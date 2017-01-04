//FILE: overlay/overlay.c
//
//Description: this file implements a ON process 
//A ON process first connects to all the neighbors and then starts listen_to_neighbor threads each of which keeps receiving the incoming packets from a neighbor and forwarding the received packets to the SNP process. Then ON process waits for the connection from SNP process. After a SNP process is connected, the ON process keeps receiving sendpkt_arg_t structures from the SNP process and sending the received packets out to the overlay network. 
//
//Date: April 28,2008


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "overlay.h"
#include "../topology/topology.h"
#include "neighbortable.h"

//you should start the ON processes on all the overlay hosts within this period of time
#define OVERLAY_START_DELAY 60

/**************************************************************/
//declare global variables
/**************************************************************/

//declare the neighbor table as global variable 
nbr_entry_t* nt; 
//declare the TCP connection to SNP process as global variable
int network_conn; 
int EXIT_SIG = 0;
int keeplistenning = 1;

/**************************************************************/
//implementation overlay functions
/**************************************************************/

// This thread opens a TCP port on CONNECTION_PORT and waits for the incoming connection from all the neighbors that have a larger node ID than my nodeID,
// After all the incoming connections are established, this thread terminates 
void* waitNbrs(void* arg)
{
    int largerIdNum = 0; int nodeId;
    int myNodeId = topology_getMyNodeID();
    int nbrNum = topology_getNbrNum();
    int srvconn = server_socket_setup(CONNECTION_PORT);
    if(srvconn == -1)
    {
		fprintf(stderr, "err in file %s func %s line %d: server_socket_setup err.\n"
                , __FILE__, __func__, __LINE__);
        return NULL;
	}
    for (int i = 0; i < nbrNum; ++i)
    {
        if (nt[i].nodeID > myNodeId)
        {
            largerIdNum++;
        }
    }
    while ((largerIdNum--) > 0)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int clientconn = accept(srvconn, (struct sockaddr*)&client_addr, &len);
        if (clientconn < 0){
			fprintf(stderr, "err in file %s func %s line %d: accept err.\n"
                    , __FILE__, __func__, __LINE__);
		}
        nodeId = topology_getNodeIDfromip(&client_addr.sin_addr);
        if(nodeId == -1)
        {
			fprintf(stderr, "err in file %s func %s line %d: topology_getNodeIDfromip err.\n"
                    , __FILE__, __func__, __LINE__);
		}
        printf("%s: get nbrid %d sock %d\n", __func__, nodeId, clientconn);
        if (nodeId > myNodeId)
        {
            if (nt_addconn(nt, nodeId, clientconn) == -1)
            {
                fprintf(stderr, "err in file %s func %s line %d: nt_addconn err.\n"
                        , __FILE__, __func__, __LINE__);
            }
        }
    }
    printf("%s: finish\n", __func__);
    return 0;
}

int server_socket_setup(int port)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("Socket create failed.\n");
		return -1;
    }
    else
    {
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
    struct sockaddr_in server;
    server.sin_addr.s_addr = htons(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*) &server, sizeof(server)))
    {
		printf("Bind port %d failed.\n", port);
		return -1;
	}

    if (listen(sock, LISTEN_QUEUE_LENGTH))
    {
		printf("Server Listen Failed!");
		return -1;
	}

    return sock;
}

// This function connects to all the neighbors that have a smaller node ID than my nodeID
// After all the outgoing connections are established, return 1, otherwise return -1
int connectNbrs()
{
    int nbrNum = topology_getNbrNum();
    int myNodeId = topology_getMyNodeID();
    for (int i = 0; i < nbrNum; ++i)
    {
        if (nt[i].nodeID > myNodeId)
        {
            continue;
        }
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            printf("Could not create socket\n");
        }
        struct sockaddr_in server = config_server(nt[i].nodeIP);
        printf("%s: adding nodeid %d's socket\n", __func__, nt[i].nodeID);
		if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
        {
            perror("connect failed. Error");
			exit(-1);
		}
        if (nt_addconn(nt, nt[i].nodeID, sock) == -1)
        {
            fprintf(stderr, "err in file %s func %s line %d: nt_addconn err.\n"
                    , __FILE__, __func__, __LINE__);
            exit(-1);
        }
    }
    printf("%s: finish\n", __func__);
    return 1;
}

struct sockaddr_in config_server(in_addr_t nodeIP)
{
    struct sockaddr_in server;
    server.sin_addr.s_addr = nodeIP;
    server.sin_family = AF_INET;
    server.sin_port = htons(CONNECTION_PORT);
    return server;
}

//Each listen_to_neighbor thread keeps receiving packets from a neighbor. It handles the received packets by forwarding the packets to the SNP process.
//all listen_to_neighbor threads are started after all the TCP connections to the neighbors are established 
void* listen_to_neighbor(void* arg)
{
    int i = *(int *)arg;
    snp_pkt_t* pkt = (snp_pkt_t*)malloc(sizeof(snp_pkt_t));
    while (!EXIT_SIG)
    {
        if (!keeplistenning)
        {
            sleep(10000);
            printf("%s: stop listening.\n", __func__);
            //keeplistenning = 1;  //add!!! keeplistenning is changed at other function
            continue;
        }
        if (recvpkt(pkt, nt[i].conn) < 0)
        {
            printf("%s-%d: snp process is down on the other side.\n", __func__, i);
            keeplistenning = 0;
        }
        else
        {
            if (network_conn != -1)
            {
                forwardpktToSNP(pkt, network_conn);
                printf("%s-%d: get a packet.\n", __func__, i);
            }
            else
            {
                printf("%s: snp process is not connected, maybe try latter\n", __func__);
            }
        }
    }
    free(arg);
    return 0;
}

//This function opens a TCP port on OVERLAY_PORT, and waits for the incoming connection from local SNP process. After the local SNP process is connected, this function keeps getting sendpkt_arg_ts from SNP process, and sends the packets to the next hop in the overlay network. If the next hop's nodeID is BROADCAST_NODEID, the packet should be sent to all the neighboring nodes.
void waitNetwork()
{
    int nodeNum = topology_getNbrNum();
    int *nodeArray = topology_getNbrArray();
    int *nextNode = (int *)malloc(sizeof(int));  //free attention!!!
    snp_pkt_t *pkt = (snp_pkt_t*)malloc(sizeof(snp_pkt_t));
	int srvconn = server_socket_setup(OVERLAY_PORT);
    struct sockaddr* client_addr;
    socklen_t len = sizeof(client_addr);
    network_conn = accept(srvconn, client_addr, &len);
    if (network_conn < 0) {
		fprintf(stderr, "err in file %s func %s line %d: accept err.\n"
                , __FILE__, __func__, __LINE__);
        exit(1);
	}
    
    int keepwaiting = 1;
    while (!EXIT_SIG)
    {
        if (!keepwaiting)
        {
            sleep(10000);
            printf("%s: stop waiting.\n", __func__);
            continue;
        }
        if (getpktToSend(pkt, nextNode, network_conn) < 0)
        {
            printf("%s: snp is down\n", __func__);
            keepwaiting = 0;
        }
        else
        {
            if (*nextNode == BROADCAST_NODEID)
            {
                for (int i = 0; i < nodeNum; ++i)
                {
                    sendToNeighbor(pkt, nodeArray[i]);
                }
            }
            else
            {
                sendToNeighbor(pkt, *nextNode);
            }
        }
    }
}

void sendToNeighbor(snp_pkt_t* pkt, int nodeId)
{
    int nodeNum = topology_getNbrNum();
    for (int i = 0; i < nodeNum; ++i)
    {
        if (nodeId == nt[i].nodeID)
        {
            if(sendpkt(pkt, nt[i].conn) == -1)
            {
                printf("%s: snp process is not connected on the other side.\n", __func__);
            }
            else
            {
                return;
            }
        }
    }
}

//this function stops the overlay
//it closes all the connections and frees all the dynamically allocated memory
//it is called when receiving a signal SIGINT
void overlay_stop()
{
    int nbrNum = topology_getNbrNum();
    for (int i = 0; i < nbrNum; ++i)
    {
        close(nt[i].conn);
    }
    close(network_conn);
    nt_destroy(nt);
    EXIT_SIG = 1;
}

int main() {
	//start overlay initialization
	printf("Overlay: Node %d initializing...\n",topology_getMyNodeID());	

	//create a neighbor table
	nt = nt_create();
	//initialize network_conn to -1, means no SNP process is connected yet
	network_conn = -1;
	
	//register a signal handler which is sued to terminate the process
	signal(SIGINT, overlay_stop);

	//print out all the neighbors
	int nbrNum = topology_getNbrNum();
	int i;
	for(i = 0; i < nbrNum; i++)
    {
		printf("Overlay: neighbor %d:%d\n",i + 1, nt[i].nodeID);
	}

	//start the waitNbrs thread to wait for incoming connections from neighbors with larger node IDs
	pthread_t waitNbrs_thread;
	pthread_create(&waitNbrs_thread, NULL, waitNbrs, (void*)0);

	//wait for other nodes to start
	sleep(OVERLAY_START_DELAY);
	
	//connect to neighbors with smaller node IDs
	connectNbrs();

	//wait for waitNbrs thread to return
	pthread_join(waitNbrs_thread, NULL);

	//at this point, all connections to the neighbors are created
	
	//create threads listening to all the neighbors
	for(i = 0;i < nbrNum; i++)
    {
		int* idx = (int*)malloc(sizeof(int));
		*idx = i;
		pthread_t nbr_listen_thread;
		pthread_create(&nbr_listen_thread, NULL, listen_to_neighbor, (void*)idx);
	}
	printf("Overlay: node initialized...\n");
	printf("Overlay: waiting for connection from SNP process...\n");

	//waiting for connection from  SNP process
	waitNetwork();
}

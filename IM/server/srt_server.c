#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "srt_server.h"
#include "../common/constants.h"
#include "string.h"
#define VNAME(x) #x
#define MAX_THREAD_NUM 11

typedef struct svr_tcb svr_tcb_t;
typedef struct port_sockfd_pair{
  int port;
  int sock;
}port_sock;

int overlay_conn, thread_count = 0;;
const int TCB_TABLE_SIZE = MAX_TRANSPORT_CONNECTIONS;
const int CHK_STAT_INTERVAL_NS = 50;
pthread_t threads[MAX_THREAD_NUM];
svr_tcb_t **tcb_table;
port_sock **p2s_hash_t;

/*interfaces to application layer*/

//
//
//  SRT socket API for the server side application.
//  ===================================
//
//  In what follows, we provide the prototype definition for each call and limited pseudo code representation
//  of the function. This is not meant to be comprehensive - more a guideline.
//
//  You are free to design the code as you wish.
//
//  NOTE: When designing all functions you should consider all possible states of the FSM using
//  a switch statement (see the Lab4 assignment for an example). Typically, the FSM has to be
// in a certain state determined by the design of the FSM to carry out a certain action.
//
//  GOAL: Your job is to design and implement the prototypes below - fill in the code.
//

// srt server initialization
//
// This function initializes the TCB table marking all entries NULL. It also initializes
// a global variable for the overlay TCP socket descriptor ‘‘conn’’ used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to
// handle the incoming segments. There is only one seghandler for the server side which
// handles call connections for the client.
//

void srt_server_init(int conn) {
  overlay_conn = conn;

  // init tcb table and port/sock mapping
  int i;
  tcb_table = (svr_tcb_t**) malloc(TCB_TABLE_SIZE * sizeof(svr_tcb_t*));
  for (i = 0; i < TCB_TABLE_SIZE; i++){
    *(tcb_table + i) = NULL;
  }
  p2s_hash_t = (port_sock**) malloc(TCB_TABLE_SIZE * sizeof(port_sock*));
  for (i = 0; i < TCB_TABLE_SIZE; i++){
    *(p2s_hash_t + i) = NULL;
  }

  // handling new coming request
  bzero(&threads, sizeof(pthread_t) * MAX_THREAD_NUM);
  // creating new thread
  int pthread_err = pthread_create(threads + (thread_count++), NULL,
    (void *) seghandler, (void *) NULL);
  if (pthread_err != 0) {
    printf("Create thread Failed!\n");
    return;
  }

  return;
}

// Create a server sock
//
// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized
// e.g., TCB state is set to CLOSED and the server port set to the function call parameter
// server port.  The TCB table entry index should be returned as the new socket ID to the server
// and be used to identify the connection on the server side. If no entry in the TCB table
// is available the function returns -1.

int srt_server_sock(unsigned int port) {
  int idx;
  // find the first NULL, and create a tcb entry
  for (idx = 0; idx < TCB_TABLE_SIZE; idx++){
    if (tcb_table[idx] == NULL) {
      // creat a tcb entry
      tcb_table[idx] = (svr_tcb_t*) malloc(sizeof(svr_tcb_t));
      if(init_tcb(idx, port) == -1) 
        printf("hash table insert failed!\n");
      // printf("sock on port %d created\n", port);
      return idx;
    }
  }
  return -1;
}

// Accept connection from srt client
//
// This function gets the TCB pointer using the sockfd and changes the state of the connetion to
// LISTENING. It then starts a timer to ‘‘busy wait’’ until the TCB’s state changes to CONNECTED
// (seghandler does this when a SYN is received). It waits in an infinite loop for the state
// transition before proceeding and to return 1 when the state change happens, dropping out of
// the busy wait loop. You can implement this blocking wait in different ways, if you wish.
//

int srt_server_accept(int sockfd) {
  // set the state of corresponding tcb entry
  if (state_transfer(sockfd, LISTENING) == -1)
    printf("%s: state tranfer err!\n", __func__);

  return keep_try(sockfd, LISTENING, -1, -1);
}

// Receive data from a srt client
//
// Receive data to a srt client. Recall this is a unidirectional transport
// where DATA flows from the client to the server. Signaling/control messages
// such as SYN, SYNACK, etc.flow in both directions. You do not need to implement
// this for Lab4. We will use this in Lab5 when we implement a Go-Back-N sliding window.
//
int srt_server_recv(int sockfd, void* buf, unsigned int length) {
	return 1;
}

// Close the srt server
//
// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1
// if fails (i.e., in the wrong state).
//

int srt_server_close(int sockfd) {
  // printf("%s: current state %d, sockfd is %d\n", __func__, tcb_table[sockfd]->state, sockfd);
  if(tcb_table[sockfd] == NULL)
    printf("%s: tcb not found!\n", __func__);
  if(tcb_table[sockfd]->state != CLOSED)
    printf("%s: force to close server\n", __func__);

  // delete entry in hash table
  int port = tcb_table[sockfd]->svr_portNum;
  int idx = p2s_hash_get_idx(port);
  free(p2s_hash_t[idx]);
  p2s_hash_t[idx] = NULL;
  printf("hash table entry %d -> %d deleted\n", port, sockfd);

  // delete entry in tcb table
  free(tcb_table[sockfd]);
  tcb_table[sockfd] = NULL;

  return 1;
}

// Thread handles incoming segments
//
// This is a thread  started by srt_server_init(). It handles all the incoming
// segments from the client. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the overlay connection is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
//

void *seghandler(void* arg) {  
  seg_t* segPtr = (seg_t*) malloc(sizeof(seg_t));
  while(1) {
    if(recvseg(overlay_conn, segPtr) == 1){
      int sockfd = p2s_hash_get_sock(segPtr->header.dest_port);
      if(segPtr->header.type == SYN){
        if (state_transfer(sockfd, CONNECTED) == -1)
          printf("%s: transfer err!\n", __func__);
        tcb_table[sockfd]->client_portNum = segPtr->header.src_port; // client port, GET!
        send_control_msg(sockfd, SYNACK);
      }
      else if(segPtr->header.type == FIN){
        send_control_msg(sockfd, FINACK);
        // printf("FIN received for sockfd %d, port %d\n", sockfd, segPtr->header.dest_port);
        if (state_transfer(sockfd, CLOSEWAIT) == -1){
          printf("FIN duplicate!\n");
        }
        else{
          // creating new thread
          int pthread_err = pthread_create(threads + (thread_count++), NULL,
            (void *) close_wait, (void *) sockfd);
          if (pthread_err != 0) {
            printf("Create thread Failed!\n");
            return;
          }
        }
      }
      else{
        printf("%s: unkown msg type!\n", __func__);
        return;
      }
    }else{
      return;
    }
  }
}

void *close_wait(int sockfd) { 
  sleep(CLOSEWAIT_TIME);
  // printf("%s: current state %d, sockfd is %d\n", __func__, tcb_table[sockfd]->state, sockfd);
  if (state_transfer(sockfd, CLOSED) == -1)
    printf("%s: state tranfer err!\n", __func__);
  // printf("%s: after close_wait current state %d, sockfd is %d\n", __func__, tcb_table[sockfd]->state, sockfd);
}

int init_tcb(int sockfd, int port) {
  svr_tcb_t* tcb_t = tcb_table[sockfd];
  tcb_t->svr_nodeID = 0;  // currently unused
  tcb_t->svr_portNum = port;   
  tcb_t->client_nodeID = 0;  // currently unused
  tcb_t->client_portNum = 0; // @TODO: where to get it??, will be initialized latter
  tcb_t->state = CLOSED; 
  tcb_t->expect_seqNum = 0; 
  tcb_t->recvBuf = NULL; 
  tcb_t->usedBufLen = 0; 
  tcb_t->bufMutex = NULL; 

  int i;
  for(i = 0; i < TCB_TABLE_SIZE; i++) {
    int hash_idx = (port + i) % TCB_TABLE_SIZE; // hash function
    if(p2s_hash_t[hash_idx] == NULL) {
      port_sock* p2s = (port_sock*) malloc(sizeof(port_sock));
      p2s->port = port;
      p2s->sock = sockfd;
      p2s_hash_t[hash_idx] = p2s;
      printf("hash table entry %d -> %d added\n", port, sockfd);
      return 0;
    }
  }
  return -1;
}

int is_timeout(struct timespec tstart, struct timespec tend, long timeout_ns) {
  if(tend.tv_sec - tstart.tv_sec > 0)
    return 1;
  else if(tend.tv_nsec - tstart.tv_nsec > timeout_ns)
    return 1;
  else
    return 0;
}

void send_control_msg(int sockfd, int type) {
  if(tcb_table[sockfd] == NULL)
    printf("%s: tcb not found!\n", __func__);

  seg_t* segPtr = (seg_t*) malloc(sizeof(seg_t));
  segPtr->header.src_port = tcb_table[sockfd]->svr_portNum;
  segPtr->header.dest_port = tcb_table[sockfd]->client_portNum;
  segPtr->header.type = type;

  sendseg(overlay_conn, segPtr);
}

int keep_try(int sockfd, int action, int maxtry, long timeout) {
  if(tcb_table[sockfd] == NULL)
    printf("%s: tcb not found!\n", __func__);

  int try_cnt = 1;
  while(maxtry == -1 || try_cnt++ <= FIN_MAX_RETRY) {
    struct timespec tstart={0,0}, tend={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    clock_gettime(CLOCK_MONOTONIC, &tend);
    while(timeout == -1 || !is_timeout(tstart, tend, timeout)) {
      if(action == LISTENING 
        && tcb_table[sockfd]->state == CONNECTED)
        return 1;
      clock_gettime(CLOCK_MONOTONIC, &tend);
    }
  }
  if (state_transfer(sockfd, CLOSED) == -1)
    printf("%s: state tranfer err!\n", __func__);
  return -1;
}

int state_transfer(int sockfd, int new_state) {
  if(tcb_table[sockfd] == NULL)
    printf("%s: tcb not found!\n", __func__);

  if(new_state == CLOSED) {
    if (tcb_table[sockfd]->state == CLOSEWAIT) {
      tcb_table[sockfd]->state = CLOSED;
      printf("State of sockfd %d transfer to %s\n", sockfd, "CLOSED");
      return 0;
    }
  } else if (new_state == CLOSEWAIT) {
    if (tcb_table[sockfd]->state == CLOSEWAIT
      || tcb_table[sockfd]->state == CONNECTED) {
      tcb_table[sockfd]->state = CLOSEWAIT;
      printf("State of sockfd %d transfer to %s\n", sockfd, "CLOSED");
      return 0;
    }
  } else if (new_state == CONNECTED) {
    if (tcb_table[sockfd]->state == CONNECTED
      || tcb_table[sockfd]->state == LISTENING) {
      tcb_table[sockfd]->state = CONNECTED;
      printf("State of sockfd %d transfer to %s\n", sockfd, "CONNECTED");
      return 0;
    }
  } else if (new_state == LISTENING) {
    if (tcb_table[sockfd]->state == CLOSED) {
      tcb_table[sockfd]->state = LISTENING;
      printf("State of sockfd %d transfer to %s\n", sockfd, "LISTENING");
      return 0;
    }
  }
  return -1;
}

int p2s_hash_get_sock(int port) {
  return p2s_hash_t[p2s_hash_get_idx(port)]->sock;
}

int p2s_hash_get_idx(int port) {
  int i;
  for(i = 0; i < TCB_TABLE_SIZE; i++) {
    int hash_idx = (port + i) % TCB_TABLE_SIZE; // hash function
    if(p2s_hash_t[hash_idx] != NULL 
      && p2s_hash_t[hash_idx]->port == port) {
      return hash_idx;
    }
  }
  printf("%s: err\n", __func__);
  return -1;  
}
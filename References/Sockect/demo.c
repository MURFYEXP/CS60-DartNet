
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

int network_conn;

int server_socket_setup(int port) {
    // build server Socket
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		printf("Socket create failed.\n");
		return -1;
	} else {
		int opt = 1;
		setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	}
    
	// set server configuration
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
    
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(port);
    
	// bind the port
	if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr))) {
		printf("Bind port %d failed.\n", port);
		return -1;
	}
    
	// listening
	if (listen(server_socket, 10)) {
		printf("Server Listen Failed!");
		return -1;
	}
    
	return server_socket;
}


void waitnetwork()
{
    int srvconn = server_socket_setup(3569);
    struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);
    
    network_conn = accept(srvconn, (struct sockaddr*) &client_addr, &length);
	if (network_conn < 0) {
		fprintf(stderr, "err in file %s func %s line %d: accept err.\n"
                , __FILE__, __func__, __LINE__);
        exit(1);
	}
    else
    {
        printf("wait network!!\n");
    }
}

struct sockaddr_in config_server(int port) {
	struct sockaddr_in server;
	struct hostent* host =NULL;
    
	if((host = gethostbyname("localhost")) == NULL) {
		fprintf(stderr, "err in file %s func %s line %d: gethostbyname err.\n"
                , __FILE__, __func__, __LINE__);
		exit(1);
	}
    
    memcpy(&server.sin_addr.s_addr, host->h_addr_list[0], sizeof(struct in_addr));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
    
	return server;
}

int connectToOverlay() {
	int sock;
	struct sockaddr_in server;
    
	// Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Could not create socket\n");
		return -1;
	}
	
	// Configure server info
	server = config_server(3569);
    
	// Connect to remote server
	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("connect failed. Error");
		return -1;
	}
    else
    {
        printf("connect success!!\n");
    }
	return sock;
}


int main()
{
    waitnetwork();
}

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

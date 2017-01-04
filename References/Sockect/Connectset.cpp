struct sockaddr_in config_server(in_addr_t nodeIP)
{
    struct sockaddr_in server;
    server.sin_addr.s_addr = nodeIP;
    server.sin_family = AF_INET;
    server.sin_port = htons(CONNECTION_PORT);
    return server;
}

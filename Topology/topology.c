//FILE: topology/topology.c
//
//Description: this file implements some helper functions used to parse 
//the topology file 
//
//Date: May 3,2010

#include "topology.h"

char filename[] = "/Users/mac/Desktop/DartNet/lab6handout/topology/topology.dat";

//this function returns node ID of the given hostname
//the node ID is an integer of the last 8 digit of the node's IP address
//for example, a node with IP address 202.120.92.3 will have node ID 3
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromname(char* hostname) 
{
    int nodeId = 0;
    struct hostent* hostInfo = NULL;
    hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: gethostbyname err.\n"
                , __FILE__, __func__, __LINE__);
        return -1;
    }
    struct in_addr* ip = (struct in_addr*)hostInfo->h_addr_list[0];
    nodeId = topology_getNodeIDfromip(ip);
    if (nodeId == -1)
    {
        fprintf(stderr, "err in file %s func %s line %d: inet_ntoa err.\n"
                , __FILE__, __func__, __LINE__);
        return -1;
    }
    return nodeId;
}

//this function returns node ID from the given IP address
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromip(struct in_addr* addr)
{
    int nodeId = 0;
    char ip_adress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, addr, ip_adress, INET_ADDRSTRLEN);
    if (ip_adress == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: inet_ntoa err.\n",
                __FILE__, __func__, __LINE__);
        return -1;
    }
    char *ptr = rindex(ip_adress, '.');
    if (ptr == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: rindex err.\n",
                __FILE__, __func__, __LINE__);
        return -1;
    }
    nodeId = atoi(ptr + 1);
    return nodeId;
}

//this function returns my node ID
//if my node ID can't be retrieved, return -1
int topology_getMyNodeID()
{
    int nodeId = 0;
    char hostname[2048];
    if (gethostname(hostname, 2048) == -1)
    {
        fprintf(stderr, "err in file %s func %s line %d: gethostbyname err.\n"
                , __FILE__, __func__, __LINE__);
        return -1;
    }
    nodeId = topology_getNodeIDfromname(hostname);
    if (nodeId == -1)
    {
        fprintf(stderr, "err in file %s func %s line %d: topology_getNodeIDfromname err.\n",
                __FILE__, __func__, __LINE__);
        return -1;
    }
    return nodeId;
}

//this functions parses the topology information stored in topology.dat
//returns the number of neighbors
int topology_getNbrNum()
{
    char hostname[2048];
    if (gethostname(hostname, 2048) == -1)
    {
        fprintf(stderr, "err in file %s func %s line %d: gethostbyname err.\n"
                , __FILE__, __func__, __LINE__);
        return -1;
    }
    getNbrNumfromHostname(hostname);
    return 0;
}

int getNbrNumfromHostname(char* hostname)
{
    FILE *pFile;
    int nbrNum = 0; int cost;
    char host1[100], host2[100]; //array's length temporary set 100
    pFile = fopen(filename, "r");
    if (pFile == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: fopen err.\n",
                __FILE__, __func__, __LINE__);
        return -1;
    }

    while (fscanf(pFile, "%s %s %d", host1, host2, &cost) != EOF)
    {
        if (strcmp(host1, hostname) * strcmp(host2, hostname) == 0)
        {
            nbrNum++;
        }
    }
    fclose(pFile);
    return nbrNum;
}

//this functions parses the topology information stored in topology.dat
//returns the number of total nodes in the overlay 
int topology_getNodeNum()
{ 
  return 0;
}

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the nodes' IDs in the overlay network  
int* topology_getNodeArray()
{
  return 0;
}

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the neighbors'IDs  
int* topology_getNbrArray()
{
    char hostname[2048];
    if (gethostname(hostname, 2048) == -1)
    {
        fprintf(stderr, "err in file %s func %s line %d: gethostbyname err.\n"
                , __FILE__, __func__, __LINE__);
        return NULL;
    }
    return getNbrArray(hostname);
}

int* getNbrArray(char* hostname)
{
    int nodeId = 0;
    int nbrArray[MAX_NODE_NUM];
    bzero(nbrArray, MAX_NODE_NUM * sizeof(int));
    FILE *pFile;
    int nbrNum = 0; int cost;
    char host1[100], host2[100]; //array's length temporary set 100
    pFile = fopen(filename, "r");
    if (pFile == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: fopen err.\n",
                __FILE__, __func__, __LINE__);
        return NULL;
    }
    
    while (fscanf(pFile, "%s %s %d", host1, host2, &cost) != EOF)
    {
        if (strcmp(host1, hostname) == 0)
        {
            nodeId = topology_getNodeIDfromname(host2);
        }
        else if (strcmp(host2, hostname) == 0)
        {
            nodeId = topology_getNodeIDfromname(host1);
        }
        else
        {
            continue;
        }
        if(nodeId == -1)
        {
            fprintf(stderr, "err in file %s func %s line %d: topology_getNodeIDfromname err.\n",
                    __FILE__, __func__, __LINE__);
            return NULL;
        }
        nbrArray[nbrNum] = nodeId;
        nbrNum++;
    }
    fclose(pFile);
    return nbrArray;
}

struct in_addr getIpFromNodeId(int nodeId)
{
    struct hostent* hostInfo = NULL;
    char *hostname = getHostnameFromNodeId(nodeId);
    if (hostname == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: getHostnameFromNodeId err.\n"
                , __FILE__, __func__, __LINE__);
        exit(-1);
        
    }
    hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: gethostbyname err.\n"
                , __FILE__, __func__, __LINE__);
        exit(-1);
    }
    struct in_addr* ip = (struct in_addr*)hostInfo->h_addr_list[0];
    return *ip;
}

char* getHostnameFromNodeId(int nodeId)
{
    int nid = 0;
    int nbrArray[MAX_NODE_NUM];
    bzero(nbrArray, MAX_NODE_NUM * sizeof(int));
    FILE *pFile;
    int cost;
    char host1[100], host2[100]; //array's length temporary set 100
    pFile = fopen(filename, "r");
    if (pFile == NULL)
    {
        fprintf(stderr, "err in file %s func %s line %d: fopen err.\n",
                __FILE__, __func__, __LINE__);
        return NULL;
    }
    
    while (fscanf(pFile, "%s %s %d", host1, host2, &cost) != EOF)
    {
        nid = topology_getNodeIDfromname(host1);
        if (nbrArray[nid] == 0) //fliter the same unmate nid
        {
            nbrArray[nid] = 1;
            if (nodeId == nid)
            {
                return host1;
            }
        }
        nid = topology_getNodeIDfromname(host2);
        if (nbrArray[nid] == 0)
        {
            nbrArray[nid] = 1;
            if (nodeId == nid)
            {
                return host2;
            }
        }
    }
    fclose(pFile);
    return 0;
}

//this functions parses the topology information stored in topology.dat
//returns the cost of the direct link between the two given nodes 
//if no direct link between the two given nodes, INFINITE_COST is returned
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
  return 0;
}
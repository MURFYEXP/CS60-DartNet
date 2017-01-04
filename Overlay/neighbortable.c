//FILE: overlay/neighbortable.c
//
//Description: this file the API for the neighbor table
//
//Date: May 03, 2010

#include "neighbortable.h"

//This function first creates a neighbor table dynamically. It then parses the topology/topology.dat file and fill the nodeID and nodeIP fields in all the entries, initialize conn field as -1 .
//return the created neighbor table
nbr_entry_t* nt_create()
{
    
    int nbrNum = topology_getNbrNum();
    nbr_entry_t* nbr_Table;
    nbr_Table = (nbr_entry_t*)malloc(sizeof(nbr_entry_t) * nbrNum);
    int* nbrArray = topology_getNbrArray(); //no dynamically
    for (int i = 0; i < nbrNum; ++i)
    {
        nbr_Table[i].nodeID = nbrArray[i];
        nbr_Table[i].nodeIP = getIpFromNodeId(nbrArray[i]).s_addr;
        nbr_Table[i].conn = -1;
    }
    return nbr_Table;
}

//This function destroys a neighbortable. It closes all the connections and frees all the dynamically allocated memory.
void nt_destroy(nbr_entry_t* nt)
{
    if (nt != NULL)
    {
        free(nt);
        nt = 0;
        return;
    }
    fprintf(stderr, "err in file %s func %s line %d: nt is null.\n"
            , __FILE__, __func__, __LINE__);
}

//This function is used to assign a TCP connection to a neighbor table entry for a neighboring node. If the TCP connection is successfully assigned, return 1, otherwise return -1
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn)
{
    int nbrNum = topology_getNbrNum();
    printf("%s: going to add sock to nodeid %d with sock %d\n", __func__, nodeID, conn);
    for (int i = 0; i < nbrNum; ++i)
    {
        if (nt[i].nodeID == nodeID)
        {
            nt[i].conn = conn;
            printf("%s: add sock to nodeid %d with sock %d\n", __func__, nodeID, conn);
            return 1;
        }
    }
    return -1;
}

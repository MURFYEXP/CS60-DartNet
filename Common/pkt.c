// File pkt.c
// May 03, 2010

#include "pkt.h"

// overlay_sendpkt() is called by the SNP process to request the ON 
// process to send a packet out to the overlay network. The 
// ON process and SNP process are connected with a local TCP connection. 
// In overlay_sendpkt(), the packet and its next hop's nodeID are encapsulated 
// in a sendpkt_arg_t data structure and sent over this TCP connection to the ON process. 
// The parameter overlay_conn is the TCP connection's socket descriptior 
// between the SNP process and the ON process.
// When sending the sendpkt_arg_t data structure over the TCP connection between the SNP 
// process and the ON process, use '!&'  and '!#' as delimiters. 
// Send !& sendpkt_arg_t structure !# over the TCP connection.
// Return 1 if sendpkt_arg_t data structure is sent successfully, otherwise return -1.
int overlay_sendpkt(int nextNodeID, snp_pkt_t* pkt, int overlay_conn)
{
  return 0;
}


// overlay_recvpkt() function is called by the SNP process to receive a packet 
// from the ON process. The parameter overlay_conn is the TCP connection's socket 
// descriptior between the SNP process and the ON process. The packet is sent over 
// the TCP connection between the SNP process and the ON process, and delimiters 
// !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if a packet is received successfully, otherwise return -1.
int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn)
{
  return 0;
}



// This function is called by the ON process to receive a sendpkt_arg_t data structure.
// A packet and the next hop's nodeID is encapsulated  in the sendpkt_arg_t structure.
// The parameter network_conn is the TCP connection's socket descriptior between the
// SNP process and the ON process. The sendpkt_arg_t structure is sent over the TCP 
// connection between the SNP process and the ON process, and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data
// Return 1 if a sendpkt_arg_t structure is received successfully, otherwise return -1.
int getpktToSend(snp_pkt_t* pkt, int* nextNode,int network_conn)
{
    char buf[sizeof(sendpkt_arg_t) + 2];
    int idx = 0; char c;
    int state = 0;
    while (recv(network_conn, &c, 1, 0) > 0)
    {
        if (state == 0)
        {
            if (c == '!')
            {
                state = 1;
            }
        }
        else if (state == 1)
        {
            if (c == '&')
            {
                state = 2;
            }
            else
            {
                state = 0;
            }
        }
        else if (state == 2)
        {
            if (c == '!')
            {
                state = 3;
                buf[idx] = c;
                idx++;
            }
            else
            {
                buf[idx] = c;
                idx++;
            }
        }
        else if (state == 3)
        {
            if (c == '#')
            {
                state = 0;
                buf[idx] = c;
                idx = 0;
                //gdb debug!!!
                memcpy(pkt, &(((sendpkt_arg_t*)buf)->pkt), sizeof(snp_pkt_t)); //!!!!changed 
                memcpy(nextNode, &(((sendpkt_arg_t*)buf)->nextNodeID), sizeof(int));
                return 1;
            }
            else if (c == '!')
            {
                buf[idx] = c;
                idx++;
            }
            else
            {
                state = 2;
                buf[idx] = c;
                idx++;
            }
        }
    }
    return -1;
}




// forwardpktToSNP() function is called after the ON process receives a packet from 
// a neighbor in the overlay network. The ON process calls this function 
// to forward the packet to SNP process.
// The parameter network_conn is the TCP connection's socket descriptior between the SNP 
// process and ON process. The packet is sent over the TCP connection between the SNP process
// and ON process, and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection.
// Return 1 if the packet is sent successfully, otherwise return -1.
int forwardpktToSNP(snp_pkt_t* pkt, int network_conn)
{
    char bufstart[2] = {'!','&'};
    char bufend[2] = {'!', '#'};
    if (send(network_conn, bufstart, 2, 0) < 0)
    {
        return -1;
    }
    if (send(network_conn, pkt, sizeof(sendpkt_arg_t), 0) < 0) //gdb debug!!!!
    {
        return -1;
    }
    if (send(network_conn, bufend, 2, 0) < 0)
    {
        return -1;
    }
    return 1;
}




// sendpkt() function is called by the ON process to send a packet 
// received from the SNP process to the next hop.
// Parameter conn is the TCP connection's socket descritpor to the next hop node.
// The packet is sent over the TCP connection between the ON process and a neighboring node,
// and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection
// Return 1 if the packet is sent successfully, otherwise return -1.
int sendpkt(snp_pkt_t* pkt, int conn)
{
    char bufstart[2] = {'!','&'};
    char bufend[2] = {'!', '#'};
    if (send(conn, bufstart, 2, 0) < 0)
    {
        return -1;
    }
    if (send(conn, pkt, sizeof(snp_pkt_t), 0) < 0)
    {
        return -1;
    }
    if (send(conn, bufend, 2, 0) < 0)
    {
        return -1;
    }
    return 1;
}



// recvpkt() function is called by the ON process to receive 
// a packet from a neighbor in the overlay network.
// Parameter conn is the TCP connection's socket descritpor to a neighbor.
// The packet is sent over the TCP connection  between the ON process and the neighbor,
// and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if the packet is received successfully, otherwise return -1.
int recvpkt(snp_pkt_t* pkt, int conn)
{
    char buf[sizeof(snp_pkt_t) + 2];
    int idx = 0; char c;
    int state = 0;
    while (recv(conn, &c, 1, 0) > 0)
    {
        if (state == 0)
        {
            if (c == '!')
            {
                state = 1;
            }
        }
        else if (state == 1)
        {
            if (c == '&')
            {
                state = 2;
            }
            else
            {
                state = 0;
            }
        }
        else if (state == 2)
        {
            if (c == '!')
            {
                state = 3;
                buf[idx] = c;
                idx++;
            }
            else
            {
                buf[idx] = c;
                idx++;
            }
        }
        else if (state == 3)
        {
            if (c == '#')
            {
                state = 0;
                buf[idx] = c;
                idx = 0;
                memcpy(pkt, (snp_pkt_t*)buf, sizeof(snp_pkt_t));
                return 1;
            }
            else if (c == '!')
            {
                buf[idx] = c;
                idx++;
            }
            else
            {
                state = 2;
                buf[idx] = c;
                idx++;
            }
        }
    }
    return -1;
}

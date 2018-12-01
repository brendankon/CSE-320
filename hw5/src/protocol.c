#include "protocol.h"
#include<stdio.h>
#include <unistd.h>
#include<stdlib.h>
#include "csapp.h"

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data){

    pkt->size = htonl(pkt->size);
    pkt->timestamp_sec = htonl(pkt->timestamp_sec);
    pkt->timestamp_nsec = htonl(pkt->timestamp_nsec);
    int size = write(fd, pkt, sizeof(XACTO_PACKET));
    if(size == -1)
        return -1;
    if(pkt->size != 0){
        size = write(fd, data, pkt->size);
        if(size == -1)
            return -1;
    }
    return 0;
}

int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap){
    int value = 0;
    value = rio_readn(fd, pkt, sizeof(XACTO_PACKET));
    if(value == -1 || value == 0)
        return -1;
    pkt->size = ntohl(pkt->size);
    pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
    pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);
    *datap = malloc(pkt->size);
    if(pkt->size != 0){
        value = rio_readn(fd, *datap, pkt->size);
        if(value == -1 || value == 0)
            return -1;
    }
    return 0;
}
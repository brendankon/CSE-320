#include "server.h"
#include <pthread.h>
#include "csapp.h"
#include "client_registry.h"
#include "transaction.h"
#include "protocol.h"
#include "data.h"
#include "store.h"
#include "debug.h"

CLIENT_REGISTRY *client_registry;
void *xacto_client_service(void *arg){

    int fd = *((int*)arg);
    free(arg);
    Pthread_detach(pthread_self());
    creg_register(client_registry, fd);
    TRANSACTION *transaction = trans_create();
    XACTO_PACKET *pktHead = malloc(sizeof(XACTO_PACKET));
    void **data = malloc(200);
    void **startData = data;
    while(transaction->status != TRANS_ABORTED || transaction->status != TRANS_COMMITTED){
        if(proto_recv_packet(fd, pktHead, data) == 0){
            if(pktHead->type == XACTO_PUT_PKT){
                data++;
                XACTO_PACKET *pktKey = malloc(sizeof(XACTO_PACKET));
                proto_recv_packet(fd, pktKey, data);
                BLOB *blobKey = blob_create(*data, pktKey->size);
                XACTO_PACKET *pktValue = malloc(sizeof(XACTO_PACKET));
                data++;
                proto_recv_packet(fd, pktValue, data);
                BLOB *blobValue = blob_create(*data, pktValue->size);
                KEY *key = key_create(blobKey);
                TRANS_STATUS stat = store_put(transaction, key, blobValue);
                if(stat == TRANS_PENDING){
                    XACTO_PACKET *sendPacket = malloc(sizeof(XACTO_PACKET));
                    sendPacket->type = XACTO_REPLY_PKT;
                    sendPacket->status = TRANS_PENDING;
                    sendPacket->null = 0;
                    sendPacket->size = 0;
                    sendPacket->timestamp_sec = pktValue->timestamp_sec;
                    sendPacket->timestamp_nsec = pktValue->timestamp_nsec;
                    proto_send_packet(fd, sendPacket, NULL);
                    store_show();
                    trans_show_all();
                }
            }

            if(pktHead->type == XACTO_GET_PKT){
                data++;
                XACTO_PACKET *pktKey = malloc(sizeof(XACTO_PACKET));
                proto_recv_packet(fd, pktKey, data);
                BLOB *blobKey = blob_create(*data, pktKey->size);
                KEY *key = key_create(blobKey);
                BLOB **keyValue = malloc(8);
                TRANS_STATUS stat = store_get(transaction, key, keyValue);
                if(stat == TRANS_PENDING){
                    XACTO_PACKET *sendPacket = malloc(sizeof(XACTO_PACKET));
                    sendPacket->type = XACTO_REPLY_PKT;
                    sendPacket->status = TRANS_PENDING;
                    sendPacket->null = 0;
                    sendPacket->size = 0;
                    sendPacket->timestamp_sec = pktKey->timestamp_sec;
                    sendPacket->timestamp_nsec = pktKey->timestamp_nsec;
                    proto_send_packet(fd, sendPacket, NULL);
                    XACTO_PACKET *dataPacket = malloc(sizeof(XACTO_PACKET));
                    dataPacket->type = XACTO_DATA_PKT;
                    dataPacket->status = TRANS_PENDING;
                    if((*keyValue) == NULL)
                        dataPacket->null = 1;
                    else
                        dataPacket->null = 0;
                    if(dataPacket->null)
                        dataPacket->size = 0;
                    else
                        dataPacket->size = (*keyValue)->size;
                    dataPacket->timestamp_sec = pktKey->timestamp_sec;
                    dataPacket->timestamp_nsec = pktKey->timestamp_nsec;
                    if(dataPacket->null)
                        proto_send_packet(fd, dataPacket, NULL);
                    else
                        proto_send_packet(fd, dataPacket, (*keyValue)->content);
                    store_show();
                    trans_show_all();
                }

            }

            if(pktHead->type == XACTO_COMMIT_PKT){
                trans_commit(transaction);
                store_show();
                XACTO_PACKET *commitPkt = malloc(sizeof(XACTO_PACKET));
                commitPkt->type = XACTO_REPLY_PKT;
                commitPkt->status = 1;
                commitPkt->timestamp_sec = pktHead->timestamp_sec;
                commitPkt->timestamp_nsec = pktHead->timestamp_nsec;
                proto_send_packet(fd, commitPkt, NULL);
                trans_show_all();
                creg_unregister(client_registry, fd);
                shutdown(fd, SHUT_RDWR);
                close(fd);
                Pthread_exit(NULL);
                free(pktHead);
                return NULL;
            }

        }

        else{
            trans_abort(transaction);
            store_show();
            trans_show_all();
            creg_unregister(client_registry, fd);
            free(pktHead);
            free(startData);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            Pthread_exit(NULL);
            return NULL;
        }
    }
    return NULL;
}
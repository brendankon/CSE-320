#include "client_registry.h"
#include "csapp.h"

typedef struct client_registry{
    int *buf;
    int n;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
    sem_t empty;
}CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init(){
    CLIENT_REGISTRY reg;
    reg.buf = calloc(200, sizeof(int));
    reg.n = 200;
    reg.rear = 0;
    Sem_init(&(reg.mutex), 0, 1);
    Sem_init(&(reg.empty), 0, 0);
    Sem_init(&(reg.slots), 0, 200);
    Sem_init(&(reg.items), 0, 0);
    /*int sem;
    sem_getvalue(&reg.slots,&sem);
    printf("%i\n", sem);*/
    CLIENT_REGISTRY *r = &reg;
    printf("%p\n", r);
    return r;
}

void creg_fini(CLIENT_REGISTRY *cr){

}

void creg_register(CLIENT_REGISTRY *cr, int fd){
    printf("%p\n", cr);
    P(&(cr->slots));
    P(&(cr->mutex));
    cr->buf[cr->rear] = fd;
    cr->rear = (cr->rear) + 1;
    V(&(cr->mutex));
    V(&(cr->items));
}

void creg_unregister(CLIENT_REGISTRY *cr, int fd){
    P(&(cr->items));
    P(&(cr->mutex));
    int i;
    for(i = 0; i < cr->n; i++){
        if(cr->buf[i] == fd)
            break;
    }
    if(i != cr->n){
        for(int x = i; x < cr->rear; x++){
            cr->buf[x] = cr->buf[x+1];
        }
        cr->buf[cr->rear] = -1;
        cr->rear = cr->rear-1;
    }
    if(cr->buf[cr->rear] == 0)
        V(&(cr->empty));
    V(&(cr->mutex));
    V(&(cr->slots));
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
    P(&(cr->empty));
    V(&(cr->empty));
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){
    P(&(cr->mutex));
    for(int i = 0; i <= cr->rear; i++){
        shutdown(cr->buf[i], SHUT_RD);
    }
    V(&(cr->mutex));
}
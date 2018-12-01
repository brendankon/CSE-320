#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include <string.h>
#include <sys/socket.h>
#include "csapp.h"
#include "server.h"

static void terminate(int status);

CLIENT_REGISTRY *client_registry;

static void hUpHandler(int sig){
    terminate(0);
}

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    // Perform required initializations of the client_registry,
    // transaction manager, and object store.
    client_registry = creg_init();
    trans_init();
    store_init();

    char* portNum = NULL;
    for(int i = 0; i < argc; i++){
        if(strcmp(*argv, "-p") == 0){
            argv++;
            if(*argv != NULL){
                portNum = *argv;
                break;
            }
        }
        argv++;
    }

    if(portNum != NULL){
        int listenfd, *connfdp;
        socklen_t clientlen;
        struct sockaddr clientaddr;
        pthread_t tid;
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = hUpHandler;
        sigaction(SIGHUP, &sa, NULL);

        listenfd = Open_listenfd(portNum);
        while (1) {
            clientlen= sizeof(struct sockaddr_storage);
            connfdp = malloc(sizeof(int));
            *connfdp = accept(listenfd, &clientaddr, &clientlen);
            Pthread_create(&tid, NULL, xacto_client_service, connfdp);
        }
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the Xacto server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    debug("Xacto server terminating");
    exit(status);
}

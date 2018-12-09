#include "transaction.h"
#include "csapp.h"

typedef struct transCount{
    int count;
    pthread_mutex_t mutex;

}TRANSCOUNT;

TRANSCOUNT counter;

void trans_init(){
    trans_list.next = &trans_list;
    trans_list.prev = &trans_list;
    trans_list.id = 0;
    counter.count = 0;
    pthread_mutex_init(&counter.mutex, NULL);
}

void trans_fini(){

}

TRANSACTION *trans_create(){
    pthread_mutex_lock(&(counter.mutex));
    TRANSACTION *trans = malloc(sizeof(TRANSACTION));
    trans->id = counter.count;
    counter.count = counter.count + 1;
    trans->refcnt = 1;
    trans->status = TRANS_PENDING;
    trans->depends = malloc(sizeof(DEPENDENCY));
    trans->waitcnt = 0;
    sem_init(&(trans->sem),0,0);
    pthread_mutex_init(&trans->mutex, NULL);
    trans->next = &trans_list;
    trans->prev = trans_list.prev;
    trans->prev->next = trans;
    trans->next->prev = trans;
    pthread_mutex_unlock(&(counter.mutex));
    return trans;
}

TRANSACTION *trans_ref(TRANSACTION *tp, char *why){
    if(tp == NULL)
        return tp;
    pthread_mutex_lock(&(tp->mutex));
    tp->refcnt = tp->refcnt + 1;
    pthread_mutex_unlock(&(tp->mutex));
    return tp;
}

void trans_unref(TRANSACTION *tp, char *why){
    if(tp != NULL){
        pthread_mutex_lock(&(tp->mutex));
        tp->refcnt = tp->refcnt - 1;
        if(tp->refcnt == 0){
            tp->prev->next = tp->next;
            tp->next->prev = tp->prev;
            free(tp->depends);
            free(tp);
            return;
        }
        pthread_mutex_unlock(&(tp->mutex));
    }
}

void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp){
    trans_ref(dtp, "Adding Dependency");
    DEPENDENCY *dep = tp->depends;
    if(dep->trans == NULL){
        dep->trans = dtp;
        return;
    }
    while(dep->next != NULL){
        if(dep->trans->id == dtp->id){
            return;
        }
        dep = dep->next;
    }
    DEPENDENCY *newD = malloc(sizeof(DEPENDENCY));
    newD->trans = dtp;
    dep->next = newD;
}

TRANS_STATUS trans_commit(TRANSACTION *tp){

    if(tp->depends->trans == NULL){
        trans_unref(tp, "Attempting to Commit Transaction");
        pthread_mutex_lock(&(tp->mutex));
        for(int i = 0; i < tp->waitcnt; i++){
            sem_post(&tp->sem);
        }
        tp->status = TRANS_COMMITTED;
        pthread_mutex_unlock(&(tp->mutex));
        return tp->status;
    }

    DEPENDENCY *dep = tp->depends;
    if(dep->next == NULL){
        pthread_mutex_lock(&(dep->trans->mutex));
        dep->trans->waitcnt = dep->trans->waitcnt + 1;
        sem_wait(&(dep->trans->sem));
        pthread_mutex_unlock(&(dep->trans->mutex));
    }
    else{
        while(dep->next != NULL){
            pthread_mutex_lock(&(dep->trans->mutex));
            dep->trans->waitcnt = dep->trans->waitcnt + 1;
            sem_wait(&(dep->trans->sem));
            pthread_mutex_unlock(&(dep->trans->mutex));
            dep = dep->next;
        }
    }

    dep = tp->depends;
    if(dep->trans->status == TRANS_ABORTED){
        trans_unref(tp, "Attempting to Commit Transaction");
        pthread_mutex_lock(&(tp->mutex));
        tp->status = TRANS_ABORTED;
        for(int i = 0; i < tp->waitcnt; i++){
            sem_post(&(tp->sem));
        }
        pthread_mutex_unlock(&(tp->mutex));
        return tp->status;
    }

    dep = dep->next;
    while(dep != NULL){
        if(dep->trans->status == TRANS_ABORTED){
            trans_unref(tp, "Attempting to Commit Transaction");
            pthread_mutex_lock(&(tp->mutex));
            tp->status = TRANS_ABORTED;
            for(int i = 0; i < tp->waitcnt; i++){
                sem_post(&(tp->sem));
            }
            pthread_mutex_unlock(&(tp->mutex));
            return tp->status;
        }
        dep = dep->next;
    }
    trans_unref(tp, "Attempting to Commit Transaction");
    pthread_mutex_lock(&(tp->mutex));
    tp->status = TRANS_COMMITTED;
    for(int i = 0; i < tp->waitcnt; i++){
        sem_post(&(tp->sem));
    }
    pthread_mutex_unlock(&(tp->mutex));
    return tp->status;
}

TRANS_STATUS trans_abort(TRANSACTION *tp){
    if(tp->status == TRANS_COMMITTED){
        exit(-1);
    }
    if(tp->status == TRANS_PENDING){
        tp->status = TRANS_ABORTED;
        trans_unref(tp, "Aborting Transaction");
        return TRANS_ABORTED;
    }
    return tp->status;
}

TRANS_STATUS trans_get_status(TRANSACTION *tp){
    return tp->status;
}

void trans_show(TRANSACTION *tp){
    if(tp != NULL)
        fprintf(stderr, "[id=%i, status=%i, refcnt=%i]", tp->id, tp->status, tp->refcnt);
}

void trans_show_all(void){
    fprintf(stderr, "TRANSACTIONS:\n");
    TRANSACTION *trans = trans_list.next;
    while(trans != &trans_list){
        trans_show(trans);
        trans = trans->next;
    }
    fprintf(stderr, "\n");
}
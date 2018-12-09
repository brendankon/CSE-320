#include "data.h"
#include "csapp.h"
#include <string.h>
#include "transaction.h"


BLOB *blob_create(char *content, size_t size){
    BLOB *bl = malloc(sizeof(BLOB));
    pthread_mutex_init(&bl->mutex, NULL);
    bl->refcnt = 1;
    bl->size = size;
    bl->content = malloc(size);
    strcpy(bl->content, content);
    bl->prefix = malloc(size);
    strcpy(bl->prefix, content);
    return bl;
}

BLOB *blob_ref(BLOB *bp, char *why){
    if(bp == NULL)
        return bp;
    pthread_mutex_lock(&(bp->mutex));
    bp->refcnt = (bp->refcnt) + 1;
    pthread_mutex_unlock(&(bp->mutex));
    return bp;
}

void blob_unref(BLOB *bp, char *why){
    if(bp != NULL){
        pthread_mutex_lock(&(bp->mutex));
        bp->refcnt = (bp->refcnt) - 1;
        if(bp->refcnt == 0)
            free(bp);
        pthread_mutex_unlock(&(bp->mutex));
    }
}

int blob_compare(BLOB *bp1, BLOB *bp2){
    if(strcmp(bp1->content, bp2->content) == 0)
        return 0;
    return -1;
}

int blob_hash(BLOB *bp){

    int hash = 7;
    for(int i = 0; i < strlen(bp->content); i++){
        hash = hash*11 + (int)(*bp->content);
        (bp->content)++;
    }
    bp->content = bp->content - strlen(bp->content);
    return hash;
}

KEY *key_create(BLOB *bp){
    KEY *k = malloc(sizeof(KEY));
    blob_ref(bp, "Creating Key");
    k->hash = blob_hash(bp);
    k->blob = bp;
    return k;
}

void key_dispose(KEY *kp){
    blob_unref(kp->blob, "Disposing Key");
    if(kp != NULL)
        free(kp);
}

int key_compare(KEY *kp1, KEY *kp2){
    if(kp1->hash == kp2->hash)
        return 0;
    return -1;
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp){
    VERSION *v = malloc(sizeof(VERSION));
    blob_ref(bp, "Creating Version");
    v->blob = bp;
    trans_ref(tp, "Creating Version");
    v->creator = tp;
    v->next = NULL;
    v->prev = NULL;
    return v;
}

void version_dispose(VERSION *vp){
    blob_unref(vp->blob, "Disposing Version");
    trans_unref(vp->creator, "Disposing Version");
    if(vp != NULL)
        free(vp);
}
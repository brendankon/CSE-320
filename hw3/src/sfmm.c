/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"

static int start = 0;
void *sf_malloc(size_t size) {
    if(size == 0)
        return NULL;
    if(start == 0){
        void *p = sf_mem_grow();
        sf_prologue *p1 = (sf_prologue *)p;
        p1->padding = 0;
        p1->header.info.allocated = 1;
        p1->header.info.prev_allocated = 0;
        p1->header.info.two_zeroes = 0;
        p1->header.info.block_size = 0;
        p1->header.info.requested_size = 0;
        p1->footer.info.allocated = 1;
        p1->footer.info.prev_allocated = 0;
        p1->footer.info.two_zeroes = 0;
        p1->footer.info.block_size = 0;
        p1->footer.info.requested_size = 0;

        char *e = (char *)sf_mem_end();
        e = e -8;
        sf_epilogue *p2 = (sf_epilogue *)e;
        p2->footer.info.allocated = 1;
        p2->footer.info.prev_allocated = 0;
        p2->footer.info.two_zeroes = 0;
        p2->footer.info.block_size = 0;
        p2->footer.info.requested_size = 0;

        size_t pageSize = PAGE_SZ - 48;
        sf_free_list_node *headPointer;
        headPointer = &sf_free_list_head;
        sf_free_list_node *newNode = sf_add_free_list(pageSize, headPointer);
        sf_header *newHead = &(newNode->head);
        char *memStart = (char *)sf_mem_start();
        memStart = memStart + 40;
        sf_header *nextHead = (sf_header *)memStart;
        nextHead->info.allocated = 0;
        nextHead->info.prev_allocated = 1;
        nextHead->info.two_zeroes = 0;
        nextHead->info.block_size = pageSize >> 4;
        nextHead->info.requested_size = 0;
        nextHead->links.next = newHead;
        nextHead->links.prev = newHead;
        newHead->links.next = nextHead;
        newHead->links.prev = nextHead;
        char *footerByte = (char *)nextHead;
        footerByte = footerByte + pageSize - 8;
        sf_footer *nextFooter =  (sf_footer *)footerByte;
        nextFooter->info.allocated = 0;
        nextFooter->info.prev_allocated = 1;
        nextFooter->info.two_zeroes = 0;
        nextFooter->info.block_size = pageSize >> 4;
        nextFooter->info.requested_size = 0;
        start = 1;
    }

    int totalSize;
    if(size <= 16){
        totalSize = 32;
    }
    else{
        totalSize = size + 8;
        while(totalSize % 16 != 0){
            totalSize++;
        }
    }

    sf_free_list_node *headPoint = &sf_free_list_head;
    sf_free_list_node *currentNode = headPoint->next;
    while(currentNode != headPoint){
        if(currentNode->size == totalSize || currentNode->size - totalSize < 32){
            sf_header *currHead;
            currHead = &(currentNode->head);
            sf_header *currHeader = currHead->links.next;
            if(currHeader != currHead){
                char *currHeaderChar;
                currHeaderChar = (char *)currHeader;
                currHeaderChar = currHeaderChar + (currHeader->info.block_size << 4);
                sf_header *nextH;
                nextH = (sf_header *)currHeaderChar;
                nextH->info.prev_allocated = 1;
                currHeader->info.allocated = 1;
                currHeader->info.requested_size = size;
                currHeader->links.prev->links.next = currHeader->links.next;
                currHeader->links.next->links.prev = currHeader->links.prev;
                currHeader->payload = 0;
                void *pay;
                pay = &(currHeader->payload);
                return pay;
            }
        }
        if(currentNode->size > totalSize){
            sf_header *currentHead;
            currentHead = &(currentNode->head);
            sf_header *currentHeader = currentHead->links.next;
            while(currentHeader != currentHead){
                if(currentHeader->info.allocated == 0){
                    currentHeader->info.allocated = 1;
                    sf_header *splitBlock;
                    char *chChar = (char *)currentHeader;
                    chChar = chChar + totalSize;
                    splitBlock = (sf_header *)chChar;
                    splitBlock->info.allocated = 0;
                    splitBlock->info.prev_allocated = 1;
                    splitBlock->info.two_zeroes = 0;
                    splitBlock->info.block_size = ((currentHeader->info.block_size << 4) - totalSize)>>4;
                    splitBlock->info.requested_size = 0;
                    chChar = chChar - totalSize;
                    chChar = chChar + (currentHeader->info.block_size << 4) - 8;
                    sf_footer *splitFooter;
                    splitFooter = (sf_footer *)chChar;
                    splitFooter->info.allocated = 0;
                    splitFooter->info.prev_allocated = 1;
                    splitFooter->info.two_zeroes = 0;
                    splitFooter->info.requested_size = 0;
                    splitFooter->info.block_size = splitBlock->info.block_size;
                    splitBlock->info.requested_size = 0;
                    //currentHeader->info.prev_allocated = 1;
                    currentHeader->info.requested_size = size;
                    currentHeader->info.block_size = totalSize >> 4;
                    currentHeader->links.prev->links.next = currentHeader->links.next;
                    currentHeader->links.next->links.prev = currentHeader->links.prev;

                    sf_free_list_node *newHP = &sf_free_list_head;
                    sf_free_list_node *currNode = newHP->next;
                    int foundList = 0;
                    sf_free_list_node *nextNode;
                    while(currNode != newHP){
                        if(currNode->size == (splitBlock->info.block_size << 4)){
                            currNode->head.links.next->links.prev = splitBlock;
                            splitBlock->links.next = currNode->head.links.next;
                            currNode->head.links.next = splitBlock;
                            splitBlock->links.prev = &(currNode->head);
                            foundList = 1;
                            break;
                        }
                        if(currNode->size > (splitBlock->info.block_size << 4)){
                            nextNode = currNode;
                            break;
                        }
                        currNode = currNode->next;
                    }

                    if(foundList == 0){
                        sf_free_list_node *newList = sf_add_free_list(splitBlock->info.block_size << 4, nextNode);
                        sf_header *sentinelHead = &(newList->head);
                        sentinelHead->links.next = splitBlock;
                        sentinelHead->links.prev = splitBlock;
                        splitBlock->links.next = sentinelHead;
                        splitBlock->links.prev = sentinelHead;
                    }

                    currentHeader->payload = 0;
                    void *firstWord;
                    firstWord = &(currentHeader->payload);
                    return firstWord;
                }
                currentHeader = currentHeader->links.next;
            }
        }
        currentNode = currentNode->next;
    }

    char *endMem = (char *)sf_mem_grow();
    if(endMem == NULL){
        sf_errno = ENOMEM;
        return NULL;
    }
    endMem = endMem - 8;
    sf_epilogue *epiFooter;
    epiFooter = (sf_epilogue *)endMem;
    char *newEpiChar = (char *)sf_mem_end();
    newEpiChar = newEpiChar - 8;
    sf_epilogue *newEpi;
    newEpi = (sf_epilogue *)newEpiChar;
    newEpi->footer.info.allocated = 1;
    newEpi->footer.info.prev_allocated = 0;
    newEpi->footer.info.two_zeroes = 0;
    newEpi->footer.info.block_size = 0;
    newEpi->footer.info.requested_size = 0;

    if(epiFooter->footer.info.prev_allocated == 1){
        epiFooter->footer.info.block_size = (PAGE_SZ) >> 4;
        endMem = endMem + 8;
        void *freedEpi = (void *)endMem;
        sf_free(freedEpi);
        return sf_malloc(size);
    }

    else{
        endMem = endMem - 8;
        sf_footer *lastBlockFoot;
        lastBlockFoot = (sf_footer *)endMem;
        int lastBSize = lastBlockFoot->info.block_size << 4;
        endMem = endMem - (lastBSize - 8);
        sf_header *lastBlockHead;
        lastBlockHead = (sf_header *)endMem;
        lastBlockHead->links.next->links.prev = lastBlockHead->links.prev;
        lastBlockHead->links.prev->links.next = lastBlockHead->links.next;
        lastBlockHead->info.block_size = (lastBSize + PAGE_SZ) >> 4;
        lastBlockHead->info.allocated = 1;
        endMem = endMem + 8;
        void *newFreeBlock = (void *)endMem;
        sf_free(newFreeBlock);
        return sf_malloc(size);
    }

    return NULL;
}

void sf_free(void *pp) {

    if(pp == NULL)
        abort();
    if((pp - 8) < (sf_mem_start() + 40))
        abort();
    if((pp - 8) > (sf_mem_end() - 8))
        abort();

    sf_header *ppHeader;
    ppHeader = (sf_header *)(pp - 8);
    if(ppHeader->info.allocated == 0)
        abort();
    int blockSize = ppHeader->info.block_size << 4;
    if(blockSize < 32 || blockSize % 16 != 0)
        abort();
    int reqSize = ppHeader->info.requested_size;
    if(reqSize + 8 > blockSize)
        abort();
    if(ppHeader->info.prev_allocated == 0){
        char *ppChar;
        ppChar = (char *)ppHeader;
        ppChar = ppChar - 8;
        sf_footer *prevFooter;
        prevFooter = (sf_footer *)ppChar;
        if(prevFooter->info.allocated != 0)
            abort();
        int prevBlockSize = prevFooter->info.block_size << 4;
        ppChar = ppChar - (prevBlockSize - 8);
        sf_header *prevHeader;
        prevHeader = (sf_header *)ppChar;
        if(prevHeader->info.allocated != 0)
            abort();
    }

    int prevIsAlloc = ppHeader->info.prev_allocated;
    int ppBlockSize = ppHeader->info.block_size << 4;
    char *ppHeaderChar = (char *)ppHeader;
    ppHeaderChar = ppHeaderChar + ppBlockSize;
    sf_header *nextHeader;
    nextHeader = (sf_header *)ppHeaderChar;
    int nextIsAlloc = nextHeader->info.allocated;

    if(prevIsAlloc == 1 && nextIsAlloc == 1){
        sf_free_list_node *newHP = &sf_free_list_head;
        sf_free_list_node *currNode = newHP->next;
        int foundList = 0;
        sf_free_list_node *nextNode;
        while(currNode != newHP){
            if(currNode->size == ppBlockSize){
                currNode->head.links.next->links.prev = ppHeader;
                ppHeader->links.next = currNode->head.links.next;
                currNode->head.links.next = ppHeader;
                ppHeader->links.prev = &(currNode->head);
                foundList = 1;
                break;
            }

            if(currNode->size > ppBlockSize){
                nextNode = currNode;
                break;
            }
            currNode = currNode->next;
        }

        if(foundList == 0){
            nextNode = currNode;
            sf_free_list_node *newList = sf_add_free_list(ppBlockSize, nextNode);
            sf_header *sentinelHead = &(newList->head);
            sentinelHead->links.next = ppHeader;
            sentinelHead->links.prev = ppHeader;
            ppHeader->links.next = sentinelHead;
            ppHeader->links.prev = sentinelHead;
        }

        sf_footer *newFooter;
        newFooter = (sf_footer *)(ppHeaderChar - 8);
        newFooter->info.allocated = 0;
        newFooter->info.prev_allocated = ppHeader->info.prev_allocated;
        newFooter->info.two_zeroes = 0;
        newFooter->info.block_size = ppHeader->info.block_size;
        newFooter->info.requested_size = ppHeader->info.requested_size;
        ppHeader->info.allocated = 0;
        nextHeader->info.prev_allocated = 0;
        if(nextHeader->info.allocated == 0){
            char *thisHeadChar;
            thisHeadChar = (char *)nextHeader;
            thisHeadChar = thisHeadChar + (nextHeader->info.block_size << 4) - 8;
            sf_footer *thisFooter;
            thisFooter = (sf_footer *)thisHeadChar;
            thisFooter->info.prev_allocated = 0;
        }
    }

    if(prevIsAlloc == 0 && nextIsAlloc == 1){
        ppHeaderChar = (char *)ppHeader;
        ppHeaderChar = ppHeaderChar - 8;
        sf_footer* prevFoot = (sf_footer *)ppHeaderChar;
        int previousSize = prevFoot->info.block_size << 4;
        ppHeaderChar = ppHeaderChar - (previousSize-8);
        sf_header *pHead;
        pHead = (sf_header *)ppHeaderChar;
        pHead->info.block_size = ((ppBlockSize) + (previousSize)) >> 4;
        pHead->info.requested_size = 0;
        pHead->links.next->links.prev = pHead->links.prev;
        pHead->links.prev->links.next = pHead->links.next;
        ppHeaderChar = ppHeaderChar + ppBlockSize + previousSize - 8;
        sf_footer *newF;
        newF = (sf_footer *)ppHeaderChar;
        newF->info.allocated = 0;
        newF->info.prev_allocated = pHead->info.prev_allocated;
        newF->info.two_zeroes = 0;
        newF->info.block_size = pHead->info.block_size;
        newF->info.requested_size = 0;

        sf_free_list_node *newHP = &sf_free_list_head;
        sf_free_list_node *currNode = newHP->next;
        int foundList = 0;
        sf_free_list_node *nextNode;
        int newBSize = pHead->info.block_size << 4;
        while(currNode != newHP){
            if(currNode->size == newBSize){
                currNode->head.links.next->links.prev = pHead;
                pHead->links.next = currNode->head.links.next;
                currNode->head.links.next = pHead;
                pHead->links.prev = &(currNode->head);
                foundList = 1;
                break;
            }
            if(currNode->size > newBSize){
                nextNode = currNode;
                break;
            }
            currNode = currNode->next;
        }

        if(foundList == 0){
            sf_free_list_node *newList = sf_add_free_list(newBSize, nextNode);
            sf_header *sentinelHead = &(newList->head);
            sentinelHead->links.next = pHead;
            sentinelHead->links.prev = pHead;
            pHead->links.next = sentinelHead;
            pHead->links.prev = sentinelHead;
        }
        nextHeader->info.prev_allocated = 0;
        if(nextHeader->info.allocated == 0){
            char *thisHeadChar;
            thisHeadChar = (char *)nextHeader;
            thisHeadChar = thisHeadChar + (nextHeader->info.block_size << 4) - 8;
            sf_footer *thisFooter;
            thisFooter = (sf_footer *)thisHeadChar;
            thisFooter->info.prev_allocated = 0;
        }

    }

    if(prevIsAlloc == 1 && nextIsAlloc == 0){
        ppHeader->info.allocated = 0;
        char *charPP;
        charPP = (char *)ppHeader;
        charPP = charPP + ppBlockSize;
        sf_header *nextBHead;
        nextBHead = (sf_header *)charPP;
        ppBlockSize = ppBlockSize + (nextBHead->info.block_size << 4);
        ppHeader->info.block_size = ppBlockSize >> 4;
        charPP = charPP + (nextBHead->info.block_size << 4) - 8;
        sf_footer *nextBFooter;
        nextBFooter = (sf_footer *)charPP;
        nextBFooter->info.prev_allocated = ppHeader->info.prev_allocated;
        nextBFooter->info.block_size = ppHeader->info.block_size;
        ppHeader->info.requested_size = 0;
        nextBFooter->info.requested_size = 0;
        nextBHead->links.next->links.prev = nextBHead->links.prev;
        nextBHead->links.prev->links.next = nextBHead->links.next;
        charPP = charPP + 8;
        sf_header *nextNextHead;
        nextNextHead = (sf_header *)charPP;
        nextNextHead->info.prev_allocated = 0;
        if(nextNextHead->info.allocated == 0){
            char *thisHeadChar;
            thisHeadChar = (char *)nextNextHead;
            thisHeadChar = thisHeadChar + (nextNextHead->info.block_size << 4) - 8;
            sf_footer *thisFooter;
            thisFooter = (sf_footer *)thisHeadChar;
            thisFooter->info.prev_allocated = 0;
        }

        sf_free_list_node *newHP = &sf_free_list_head;
        sf_free_list_node *currNode = newHP->next;
        int foundList = 0;
        sf_free_list_node *nextNode;
        while(currNode != newHP){
            if(currNode->size == ppBlockSize){
                currNode->head.links.next->links.prev = ppHeader;
                ppHeader->links.next = currNode->head.links.next;
                currNode->head.links.next = ppHeader;
                ppHeader->links.prev = &(currNode->head);
                foundList = 1;
                break;
            }
            if(currNode->size > ppBlockSize){
                nextNode = currNode;
                break;
            }
            currNode = currNode->next;
        }

        if(foundList == 0){
            sf_free_list_node *newList = sf_add_free_list(ppBlockSize, nextNode);
            sf_header *sentinelHead = &(newList->head);
            sentinelHead->links.next = ppHeader;
            sentinelHead->links.prev = ppHeader;
            ppHeader->links.next = sentinelHead;
            ppHeader->links.prev = sentinelHead;
        }
    }

    if(prevIsAlloc == 0 && nextIsAlloc == 0){
        char *pChar;
        pChar = (char *)ppHeader;
        pChar = pChar - 8;
        sf_footer *pFooter;
        pFooter = (sf_footer *)pChar;
        int prevBSize = pFooter->info.block_size << 4;
        pChar = pChar - (prevBSize - 8);
        sf_header *pBlockHeader;
        pBlockHeader = (sf_header*)pChar;
        pChar = pChar + prevBSize + ppBlockSize;
        sf_header *nextBH;
        nextBH = (sf_header *)pChar;
        int nextBSize = nextBH->info.block_size << 4;
        pBlockHeader->info.block_size = (prevBSize + nextBSize + ppBlockSize) >> 4;
        pBlockHeader->info.requested_size = 0;
        pBlockHeader->links.next->links.prev = pBlockHeader->links.prev;
        pBlockHeader->links.prev->links.next = pBlockHeader->links.next;
        nextBH->links.next->links.prev = nextBH->links.prev;
        nextBH->links.prev->links.next = nextBH->links.next;
        pChar = pChar + nextBSize - 8;
        sf_footer *nextBF;
        nextBF = (sf_footer *)pChar;
        nextBF->info.prev_allocated = pBlockHeader->info.prev_allocated;
        nextBF->info.requested_size = 0;
        nextBF->info.block_size = pBlockHeader->info.block_size;
        pChar = pChar + 8;
        sf_header * nextNextH;
        nextNextH = (sf_header *)pChar;
        nextNextH->info.prev_allocated = 0;
        if(nextNextH->info.allocated == 0){
            char *thisHeadChar;
            thisHeadChar = (char *)nextNextH;
            thisHeadChar = thisHeadChar + (nextNextH->info.block_size << 4) - 8;
            sf_footer *thisFooter;
            thisFooter = (sf_footer *)thisHeadChar;
            thisFooter->info.prev_allocated = 0;
        }
        ppBlockSize = pBlockHeader->info.block_size << 4;

        sf_free_list_node *newHP = &sf_free_list_head;
        sf_free_list_node *currNode = newHP->next;
        int foundList = 0;
        sf_free_list_node *nextNode;
        while(currNode != newHP){
            if(currNode->size == ppBlockSize){
                currNode->head.links.next->links.prev = pBlockHeader;
                pBlockHeader->links.next = currNode->head.links.next;
                currNode->head.links.next = pBlockHeader;
                pBlockHeader->links.prev = &(currNode->head);
                foundList = 1;
                break;
            }
            if(currNode->size > ppBlockSize){
                nextNode = currNode;
                break;
            }
            currNode = currNode->next;
        }

        if(foundList == 0){
            sf_free_list_node *newList = sf_add_free_list(ppBlockSize, nextNode);
            sf_header *sentinelHead = &(newList->head);
            sentinelHead->links.next = pBlockHeader;
            sentinelHead->links.prev = pBlockHeader;
            pBlockHeader->links.next = sentinelHead;
            pBlockHeader->links.prev = sentinelHead;
        }

    }
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    if(pp == NULL)
        abort();
    if((pp - 8) < (sf_mem_start() + 40))
        abort();
    if((pp - 8) > (sf_mem_end() - 8))
        abort();

    sf_header *ppHeader;
    ppHeader = (sf_header *)(pp - 8);
    if(ppHeader->info.allocated == 0)
        abort();
    int blockSize = ppHeader->info.block_size << 4;
    if(blockSize < 32 || blockSize % 16 != 0)
        abort();
    int reqSize = ppHeader->info.requested_size;
    if(reqSize + 8 > blockSize)
        abort();
    if(ppHeader->info.prev_allocated == 0){
        char *ppChar;
        ppChar = (char *)ppHeader;
        ppChar = ppChar - 8;
        sf_footer *prevFooter;
        prevFooter = (sf_footer *)ppChar;
        if(prevFooter->info.allocated != 0)
            abort();
        int prevBlockSize = prevFooter->info.block_size << 4;
        ppChar = ppChar - (prevBlockSize - 8);
        sf_header *prevHeader;
        prevHeader = (sf_header *)ppChar;
        if(prevHeader->info.allocated != 0)
            abort();
    }

    if(rsize < 0)
        abort();

    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }

    sf_header *ppH;
    char *ppCh;
    ppCh = (char *)pp;
    ppCh = ppCh-8;
    ppH = (sf_header *)ppCh;
    int requestSize = ppH->info.requested_size;
    if(rsize > requestSize){
        void *p = sf_malloc(rsize);
        memcpy(p, pp, requestSize);
        sf_free(pp);
        return p;
    }

    if(rsize < requestSize){
        int bS = ppH->info.block_size << 4;
        int newBS = rsize + 8;
        while(newBS % 16 != 0 || newBS < 32){
            newBS++;
        }

        if(bS - newBS < 32){
            ppH->info.requested_size = rsize;
            return pp;
        }

        else{
            ppH->info.requested_size = rsize;
            ppH->info.block_size = newBS >> 4;
            ppCh = ppCh + newBS;
            sf_header *newlyFreed;
            newlyFreed = (sf_header *)ppCh;
            newlyFreed->info.allocated = 1;
            newlyFreed->info.prev_allocated = 1;
            newlyFreed->info.requested_size = 0;
            newlyFreed->info.block_size = (bS - newBS) >> 4;
            newlyFreed->info.two_zeroes = 0;
            ppCh = ppCh + 8;
            void *point;
            point = (void *)ppCh;
            sf_free(point);
            return pp;
        }
    }

    return NULL;
}

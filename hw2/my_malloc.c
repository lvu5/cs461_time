#include <stdio.h>

#include "nano/list.h"

#define HEAP_SIZE 1048576

char heapMemory[HEAP_SIZE];

typedef struct {
    ListHead listNode;
    size_t size;
    int isFree;
} BlockHeader;

ListHead freeList;

void initHeap() {
    INIT_LIST_HEAD(&freeList);
    BlockHeader* header = (BlockHeader*)heapMemory;
    header->size = HEAP_SIZE - sizeof(BlockHeader);
    header->isFree = 1;
    list_add(&header->listNode, &freeList);
}

void* my_malloc(size_t size) {
    BlockHeader *header, *newHeader;
    ListHead* pos;
    void* result = NULL;
    size_t newSize = size + sizeof(BlockHeader);
    list_for_each(pos, &freeList) {
        header = list_entry(pos, BlockHeader, listNode);
        if (header->size >= newSize) {
            list_del(pos);
            if (header->size >= newSize + sizeof(BlockHeader) + 16) {
                newHeader = (BlockHeader*)((char*)header + newSize);
                newHeader->size = header->size - newSize;
                newHeader->isFree = 1;
                header->size = newSize;
                list_add(&newHeader->listNode, &freeList);
            }
            header->isFree = 0;
            result = (void*)((char*)header + sizeof(BlockHeader));
            break;
        }
    }
    return result;
}

void my_free(void* ptr) {
    if (ptr) {
        BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
#ifdef SAFE
        if (header->isFree) {
            fprintf(stderr, "Double free error!\n");
            return;
        }
        header->isFree = 1;
        list_add(&header->listNode, &freeList);
#else
        list_add(&header->listNode, &freeList);
#endif
    }
}

int main() {
    initHeap();
    char* p1 = (char*)my_malloc(512);
    char* p2 = (char*)my_malloc(256);
    char* p3 = (char*)my_malloc(1024);
    printf("Allocated p1: %p\n", (void*)p1);
    printf("Allocated p2: %p\n", (void*)p2);
    printf("Allocated p3: %p\n", (void*)p3);
    my_free(p2);
    char* p4 = (char*)my_malloc(256);
    printf("Allocated p4: %p\n", (void*)p4);
#ifdef SAFE
    my_free(p2);
#endif
    my_free(p1);
    my_free(p3);
    my_free(p4);
    return 0;
}

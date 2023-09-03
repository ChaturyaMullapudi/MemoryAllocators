#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


struct header_t{
    size_t size;
    unsigned is_free;
    struct header_t *next;
};  

typedef char align[16];

union header{
    struct{
        size_t size;
        unsigned is_free;
        union header *next;
    }s;
    align stub;
};
typedef union header header_t;
header_t *head,*tail;
pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size)
{
	header_t *curr = head;
	while(curr) {
		if (curr->s.is_free && curr->s.size >= size)
			return curr;
		curr = curr->s.next;
	}
	return NULL;
}   

// Here comes the malloc() function

void *malloc(size_t size){
    size_t total_size;
    void *block;
    header_t *header;
    if(!size){
        return NULL;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if(header){
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header+1);
    }
    total_size = sizeof(header_t)+size;
    block = sbrk(total_size);
    if(block == (void*)-1){
        pthread_mutex_lock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.size = size;
    header->s.is_free =0;
    header->s.next = NULL;
    if(!head){
        head = header;
    }
    if(tail){
        tail->s.next = header;
    }
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
	return (void*)(header + 1);
} 

// Here comes the free() function

void free(void *block){
    header_t *header,*temp;
    void *programbreak;
    if(!block){
        return;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*)block-1;
    programbreak = sbrk(0);
    if((char*)block + header->s.size == programbreak){
        if(head == tail){
            head = tail = NULL;
        }
        else{
            temp = head;
            while(temp){
                if(temp->s.next == tail){
                    temp->s.next = NULL;
                    tail = temp;
                }
                temp = temp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
		pthread_mutex_unlock(&global_malloc_lock);
		return;
    }
    header->s.is_free = 1;
	pthread_mutex_unlock(&global_malloc_lock);
}

// Here comes the calloc() function

void *calloc(size_t num,size_t numsize){
    size_t size;
    void *block;
    if(!num || !numsize){
        return NULL;
    }
    size = num * numsize;
    if(num != size/numsize){
        return NULL;
    }
    block = malloc(size);
    if(!block){
        return NULL;
    }
    memset(block,0,size);
    return block;
}

// Here comes the realloc() function

void *realloc(void *block,size_t size){
    header_t *header;
    void *ret;
    if(!block || !size){
        return malloc(size);
    }   
    header = (header_t*)block-1;
    if(header->s.size >= size){
        return block;
    }    
    ret = malloc(size);
    if(ret){
        memcpy(ret,block,header->s.size);
        free(block);
    }
    return ret;
}
int main(){
    printf("Hello World!");
    return 0;
}


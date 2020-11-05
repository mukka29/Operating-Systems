//header files
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "queue.h"

//Initilizing an empty queue
int q_init(queue_t * q, const int size){
  //allocating space for the items in queue
  q->items = (int*) malloc(sizeof(int)*size);
  if(q->items == NULL){ //if memory allocation fails i.e, malloc fails
    perror("malloc");
    return -1;
  }
  // queue is empty
  q->size = size;
  q->len = 0;
  return 0;
}

//deinitilializing an empty queue and releasing the memory
void q_deinit(queue_t * q){
  free(q->items); // releasing memory
  q->items = NULL;
  q->len = 0;
  q->size = 0;
}

// Adding an item to the queue
int q_enq(queue_t * q, const int item){
  if(q->len < q->size){ //In case if queue is not full
    q->items[q->len++] = item; // add the item
    return 0; // and return success
  }else{
    return -1;
  }
}

// remove an item from a particular position at
int q_deq(queue_t * q, const int at){
  int i;
//in case if the position is after queue end
  if(at >= q->len){
    return -1;
  }

  const int item = q->items[at]; //get the item
  q->len = q->len - 1; // decrease the queue length

  //shifting forward the items after dequeued item
  
  for(i=at; i < q->len; ++i){
    q->items[i] = q->items[i + 1];
  }
  q->items[i] = -1; //clearing the position

  return item;
}
//Returning the first item in queue
int q_top(queue_t * q){
  if(q->len > 0){ // if we have items
    return q->items[0]; //return the first one
  }else{
    return -1; // else return error
  }
}

//Return length of the queue
int q_len(queue_t * q){
  return q->len;
}

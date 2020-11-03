#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "queue.h"

int q_init(queue_t * q, const int size){
  q->items = (int*) malloc(sizeof(int)*size);
  if(q->items == NULL){
    perror("malloc");
    return -1;
  }

  q->size = size;
  q->len = 0;
  return 0;
}

void q_deinit(queue_t * q){
  free(q->items);
  q->items = NULL;
  q->len = 0;
  q->size = 0;
}

int q_enq(queue_t * q, const int item){
  if(q->len < q->size){
    q->items[q->len++] = item;
    return 0;
  }else{
    return -1;
  }
}

int q_deq(queue_t * q, const int at){
  int i;

  if(at >= q->len){
    return -1;
  }

  const int item = q->items[at];
  q->len = q->len - 1;

  for(i=at; i < q->len; ++i){
    q->items[i] = q->items[i + 1];
  }
  q->items[i] = -1;

  return item;
}

int q_top(queue_t * q){
  if(q->len > 0){
    return q->items[0];
  }else{
    return -1;
  }
}

int q_len(queue_t * q){
  return q->len;
}

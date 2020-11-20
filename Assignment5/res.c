//header files
#include <stdlib.h>
#include <stdio.h>

#include "res.h"

void res_alloc(struct resource R[RCOUNT]){

  // 20% +/- 5% of the descriptors are shared
  int i, s = 20 + ((rand() % 10) - 5);

  s = RCOUNT / (100 / s);
  printf("OSS: %d/%d resources will be shared\n", s, RCOUNT);

  //choosing which resources will be shared randomly
  while(s > 0){
    const int i = rand() % RCOUNT;
    if(R[i].is_shared == 0){
      R[i].is_shared = 1;
      --s;
    }
  }

  //seting the resource values
  for(i=0; i < RCOUNT; i++){
    if(R[i].is_shared == 0){
      R[i].total = 1 + (rand() % R_MAX);
    }else{
      R[i].total = 1;
    }
    R[i].available = R[i].total;
  }

}

void res_alloc_total(struct resource r[RCOUNT], struct resource R[RCOUNT]){
  int i;
  for(i=0; i < RCOUNT; i++){
    if(R[i].total <= 1){
      r[i].total = 1;
    }else{
      r[i].total = 1 + (rand() % (R[i].total-1));
    }
  }
}

int res_rand_request(const struct resource R[RCOUNT]){
  int i, l = 0, buf[RCOUNT];
  for(i=0; i < RCOUNT; i++){
    if(R[i].total > 0){
      buf[l++] = i;
    }
  }

  if(l == 0){
    return -1;
  }
  const int r = rand() % l;

  return buf[r];
}

int res_rand_release(const struct resource R[RCOUNT]){
  int i, l = 0, buf[RCOUNT];
  for(i=0; i < RCOUNT; i++){
    if(R[i].available > 0){
      buf[l++] = i;
    }
  }

  if(l == 0){
    return -1;
  }
  const int r = rand() % l;

  return buf[r];
}

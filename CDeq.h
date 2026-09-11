#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//#include <assert.h>
//#include "_ctypeof.h"

typedef struct {
  size_t capacity;
  size_t itemSize;
  size_t size;
  size_t frontI;
  size_t backI;
  void* data;
} CDeq;


static inline void _CDeq_init(CDeq* q, size_t itemSize, size_t capacity) {
  q->data = malloc(itemSize * capacity);
  q->capacity = capacity; q->itemSize = itemSize;
  q->size = 0; q->frontI = 0; q->backI = 0;
}
#define CDeq_init(q, T, cap) _CDeq_init(q, sizeof(T), cap)


static inline void CDeq_resize(CDeq* q, size_t newCap) {
  void* newData = malloc(q->itemSize * newCap);
  if (q->frontI <= q->backI) { 
    memcpy(newData, (char*)q->data + q->frontI * q->itemSize, q->size * q->itemSize);
  }
  else {
    memcpy(newData, (char*)q->data + q->frontI * q->itemSize, (q->capacity - q->frontI) * q->itemSize);
    memcpy((char*)newData + (q->capacity - q->frontI) * q->itemSize, q->data, (q->backI + 1) * q->itemSize);
  }
  free(q->data);
  q->data = newData; q->capacity = newCap;
  q->frontI = 0; q->backI = q->size - 1;
}


static inline void _CDeq_pushFront(CDeq* q, void* itemPtr) {
  if (q->capacity == 0) CDeq_resize(q, 5);
  else if (q->capacity == q->size) CDeq_resize(q, ceilf(q->capacity * 1.6));
  if (q->size == 0) {
    q->frontI = q->backI;
    memcpy((char*)q->data + (q->frontI * q->itemSize), itemPtr, q->itemSize);
  }
  else if(q->frontI == 0) {
    memcpy((char*)q->data + ((q->capacity - 1) * q->itemSize), itemPtr, q->itemSize);
    q->frontI = q->capacity - 1;
  }
  else {
    memcpy((char*)q->data + ((q->frontI - 1) * q->itemSize), itemPtr, q->itemSize);
    (q->frontI)--;
  }
  (q->size)++;
}
#define CDeq_pushFront(q, item) _CDeq_pushFront(q, (void*)&item)
#define CDeq_emplFrontAs(q, T, ...) \
  _CDeq_pushFront((q), &(T){__VA_ARGS__} )

#define CDeq_emplFront(q, val) \
  do { \
    __typeof__(val) tmp = (val); \
    _CDeq_pushFront((q), &tmp); \
  } while (0)

#define _EMPLF_PICK(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define CDeq_emplaceFront(...) \
  _EMPLF_PICK(__VA_ARGS__, \
             CDeq_emplFrontAs, CDeq_emplFrontAs, CDeq_emplFrontAs, CDeq_emplFrontAs, \
             CDeq_emplFrontAs, CDeq_emplFrontAs, CDeq_emplFront, _EMPLF_ERR)(__VA_ARGS__)   


static inline void _CDeq_pushBack(CDeq* q, void* itemPtr) {
  if (q->capacity == 0) CDeq_resize(q, 5);
  else if (q->capacity == q->size) CDeq_resize(q, ceilf(q->capacity * 1.6));

  if (q->size == 0) {
    // Empty: place the first item at the current index and collapse both ends
    // onto it (see _CDeq_pushFront for why the normal step is wrong here).
    q->backI = q->frontI;
    memcpy((char*)q->data + (q->backI * q->itemSize), itemPtr, q->itemSize);
  }
  else if(q->backI == q->capacity - 1) {
    memcpy((char*)q->data, itemPtr, q->itemSize);
    q->backI = 0;
  }
  else {
    memcpy((char*)q->data + ((q->backI + 1) * q->itemSize), itemPtr, q->itemSize);
    (q->backI)++;
  }
  (q->size)++;
}
#define CDeq_pushBack(q, item) _CDeq_pushBack(q, (void*)&item)
#define CDeq_emplBackAs(q, T, ...) \
  _CDeq_pushBack((q), &(T){__VA_ARGS__} )

#define CDeq_emplBack(q, val) \
  do { \
    __typeof__(val) tmp = (val); \
    _CDeq_pushBack((q), &tmp); \
  } while (0)

#define _EMPLB_PICK(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define CDeq_emplaceBack(...) \
  _EMPLB_PICK(__VA_ARGS__, \
             CDeq_emplBackAs, CDeq_emplBackAs, CDeq_emplBackAs, CDeq_emplBackAs, \
             CDeq_emplBackAs, CDeq_emplBackAs, CDeq_emplBack, _EMPLB_ERR)(__VA_ARGS__)   



#define CDeq_back(q, T) \
  ( *(T*)((char*)((q)->data) + ((q)->itemSize * ((q)->backI))) )

#define CDeq_front(q, T) \
  ( *(T*)((char*)((q)->data) + ((q)->itemSize * ((q)->frontI))) )


static inline void* _CDeq_popFront(CDeq* q) {
  if (q->size == 0) return q->data;  // no-op: state unchanged, deref lands on valid memory
  void* itemPtr = (char*)q->data + q->frontI * (q->itemSize);
  if (q->frontI == q->capacity - 1) {q->frontI = 0;}
  else (q->frontI)++;
  (q->size)--;
  return itemPtr;
}
#define CDeq_popFront(q, T) (*(T*)_CDeq_popFront(q))


static inline void* _CDeq_popBack(CDeq* q) {
  if (q->size == 0) return q->data; 
  void* itemPtr = (char*)q->data + q->backI * (q->itemSize);
  if (q->backI == 0) {q->backI = q->capacity - 1;}
  else (q->backI)--;
  (q->size)--;
  return itemPtr;
}
#define CDeq_popBack(q, T) (*(T*)_CDeq_popBack(q))

static inline void CDeq_free(CDeq* q) {free(q->data);}


#define qInit(q, T, cap) CDeq_init(q, T, cap)
#define qPushFront(q, item) CDeq_pushFront(q, item)
#define qPushBack(q, item) CDeq_pushBack(q, item)
#define qEmplFront(...) CDeq_emplaceFront(__VA_ARGS__)
#define qEmplBack(...) CDeq_emplaceBack(__VA_ARGS__)
#define qPopFront(q, T) CDeq_popFront(q, T)
#define qPopBack(q, T) CDeq_popBack(q, T)
#define qFront(q, T) CDeq_front(q, T)
#define qBack(q, T) CDeq_back(q, T)
#define qFree(q) CDeq_free(q)
// as forward queue
#define qPush(q, item) CDeq_pushBack(q, item)
#define qEmpl(...) CDeq_emplaceBack(__VA_ARGS__)
#define qPop(q, T) CDeq_popFront(q, T)
// as stack
#define sPush(s, item) CDeq_pushBack(s, item)
#define sEmpl(...) CDeq_emplaceBack(__VA_ARGS__)
#define sPop(s, T) CDeq_popBack(s, T)
#define sFront(s, T) CDeq_back(s, T)


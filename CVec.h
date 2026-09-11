#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
//#include "_ctypeof.h"


typedef struct {
  void* data;
  size_t capacity;
  size_t itemSize;
  size_t size;
} CVec;


static inline void _CVec_init(CVec* vec, size_t itemSize, size_t capacity) {
  vec->data = malloc(itemSize * capacity);
  if (vec->data) {
    vec->itemSize = itemSize; vec->capacity = capacity;
    vec->size = 0;
  }
}
#define CVec_init(vec, T, cap) _CVec_init(vec, sizeof(T), cap)
#define CVec_data(vec, T) (T*)((vec)->data)
#define CVec_at(vec, T, idx) \
  (*(T*)((char*)((vec)->data) + ((vec)->itemSize * idx)))
#define CVec_front(vec, T) \
  (*(T*)((vec)->data))
#define CVec_back(vec, T) \
  (*(T*)((char*)((vec)->data) + ((vec)->itemSize * ((vec)->size - 1))))

static inline void CVec_resize(CVec* vec, size_t newCap) {
  void* newData = realloc(vec->data, vec->itemSize * newCap);
  if (newData) {
    vec->data = newData; vec->capacity = newCap;
  }
}

static inline void _CVec_push(CVec* vec, void* itemPtr) {
  if (vec->capacity == 0) {
    CVec_resize(vec, 4);
  }
  else if (vec->capacity == vec->size) {
    CVec_resize(vec, ceilf(vec->capacity * 1.6));
  }
  memcpy((char*)(vec->data) + (vec->size * vec->itemSize), itemPtr, vec->itemSize);
  (vec->size)++;
}
#define CVec_push(vec, item) _CVec_push(vec, (void*)&item)


#define CVec_emplAs(vec, T, ...) \
  _CVec_push((vec), &(T){__VA_ARGS__} )
#define CVec_empl(vec, val) \
  do { \
    __typeof__(val) tmp = (val); \
    _CVec_push((vec), &tmp); \
  } while (0)

#define _EMPL_PICK(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define CVec_emplace(...) \
  _EMPL_PICK(__VA_ARGS__, \
             CVec_emplAs, CVec_emplAs, CVec_emplAs, CVec_emplAs, \
             CVec_emplAs, CVec_emplAs, CVec_empl, _EMPL_ERR)(__VA_ARGS__)                   


static inline void CVec_trim(CVec* vec) {
  if (vec->size > 5 && vec->size < vec->capacity / 2) {
    void* trimmed = realloc(vec->data, vec->size * vec->itemSize);
    if (trimmed) {
      vec->data = trimmed;
      vec->capacity = vec->size; 
    }
  }
}

static inline void* _CVec_pop(CVec* vec) {
  if (vec->size == 0) return vec->data;
  (vec->size)--;
  CVec_trim(vec);
  return (void*)((char*)((vec)->data) + ((vec)->itemSize * (vec)->size));
}
#define CVec_pop(vec, T) (*(T*)_CVec_pop(vec))

static inline void CVec_free(CVec* vec) {free(vec->data);}

static inline CVec _CVec_moveFrom(size_t itemSize, void* arr, size_t size) {
  return (CVec){.itemSize = itemSize, .capacity = size, .size = size, .data = arr};
}
#define CVec_moveFrom(T, arr, size) _CVec_moveFrom(sizeof(T), (void*)arr, size)

static inline CVec _CVec_copyFrom(size_t itemSize, void* arr, size_t size) {
  CVec vec; _CVec_init(&vec, itemSize, size);
  memcpy(vec.data, arr, itemSize * size);
  vec.size = size;
  return vec;
}
#define CVec_copyFrom(T, arr, size) _CVec_copyFrom(sizeof(T), (void*)arr, size)

#define vInit(v, T, cap) CVec_init(v, T, cap)
#define vMoveFrom(T, arr, size) CVec_moveFrom(T, arr, size)
#define vCopyFrom(T, arr, size) CVec_copyFrom(T, arr, size)
#define vData(v, T) CVec_data(v, T)
#define vPush(v, item) CVec_push(v, item)
#define vEmpl(...) CVec_emplace(__VA_ARGS__)
#define vPop(v, T) CVec_pop(v, T)
#define vAt(v, T, i) CVec_at(v, T, i)
#define vFront(v, T) CVec_front(v, T)
#define vBack(v, T) CVec_back(v, T)
#define vFree(v) CVec_free(v)

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//
#include "_ctypeof.h"

// list of predefined primitive comparators
// custom comparators follow signature of qsort
#define _DEF_MAX(name, T) \
  static inline int name(const void* a, const void* b) { \
    T x = *(const T*)a, y = *(const T*)b; \
    return (x > y) - (x < y);     \
  }

#define _DEF_MIN(name, T) \
  static inline int name(const void* a, const void* b) { \
    T x = *(const T*)a, y = *(const T*)b; \
    return (x < y) - (x > y);     \
  }

_DEF_MAX(MAX_int, int)
_DEF_MAX(MAX_uint, unsigned)
_DEF_MAX(MAX_long, long)
_DEF_MAX(MAX_ulong, unsigned long)
_DEF_MAX(MAX_llong, long long)
_DEF_MAX(MAX_ullong, unsigned long long)
_DEF_MAX(MAX_float, float)
_DEF_MAX(MAX_double, double)
_DEF_MAX(MAX_char, char)
_DEF_MIN(MIN_int, int)
_DEF_MIN(MIN_uint, unsigned)
_DEF_MIN(MIN_long, long)
_DEF_MIN(MIN_ulong, unsigned long)
_DEF_MIN(MIN_llong, long long)
_DEF_MIN(MIN_ullong, unsigned long long)
_DEF_MIN(MIN_float, float)
_DEF_MIN(MIN_double, double)
_DEF_MIN(MIN_char, char)

#define _PRIM_MAX(T) _Generic((T){0}, \
  int: MAX_int, \
  unsigned: MAX_uint, \
  long: MAX_long, \
  unsigned long: MAX_ulong, \
  long long: MAX_llong, \
  unsigned long long: MAX_ullong, \
  float: MAX_float, \
  double: MAX_double, \
  char: MAX_char, \
  default: NULL)

#define _PRIM_MIN(T) _Generic((T){0}, \
  int: MIN_int, \
  unsigned: MIN_uint, \
  long: MIN_long, \
  unsigned long: MIN_ulong, \
  long long: MIN_llong, \
  unsigned long long: MIN_ullong, \
  float: MIN_float, \
  double: MIN_double, \
  char: MIN_char, \
  default: NULL)


static inline size_t _parent(size_t i) { return (i - 1) / 2; }

static inline size_t _left(size_t i) { return ((2 * i) + 1); }

static inline size_t _right(size_t i) { return ((2 * i) + 2); }

static inline void _swap(void* a, void* b, size_t itemSize) {
  char* p = (char*)a; char* q = (char*)b;
  char tmp[64];
  while (itemSize >= sizeof(tmp)) {
    memcpy(tmp, p, sizeof(tmp));
    memcpy(p, q, sizeof(tmp));
    memcpy(q, tmp, sizeof(tmp));
    p += sizeof(tmp); q += sizeof(tmp); 
    itemSize -= sizeof(tmp);
  }
  if (itemSize) {
    memcpy(tmp, p, itemSize);
    memcpy(p, q, itemSize);
    memcpy(q, tmp, itemSize);
  }
}

typedef struct {
  size_t capacity;
  size_t itemSize;
  size_t size;
  void* data;
  int (*comp)(const void*, const void*);
} CPrioQ;


static inline void _CPrioQ_init(CPrioQ* pq, size_t itemSize, size_t capacity, int (*comp)(const void*, const void*)) {
  void* data = malloc(itemSize * capacity);
  pq->data = data; pq->capacity = capacity; pq->itemSize = itemSize;
  pq->size = 0; pq->comp = comp;
}
#define CPrioQ_initPrim(pq, T, cap, ORDER)  _CPrioQ_init(pq, sizeof(T), cap, _PRIM_##ORDER(T))
#define CPrioQ_initCustom(pq, T, cap, comp) _CPrioQ_init(pq, sizeof(T), cap, comp)


static inline void CPrioQ_resize(CPrioQ* pq, size_t newCap) {
  void* newData = malloc(pq->itemSize * newCap);
  if (newData) {
    memcpy(newData, pq->data, pq->size * pq->itemSize);
    free(pq->data);
    pq->data = newData; pq->capacity = newCap;
  }
}

static inline void _shiftUp(CPrioQ* pq, size_t i) {
  while (i > 0) {
    size_t p = _parent(i);
    if (pq->comp((char*)pq->data + i * pq->itemSize, (char*)pq->data + p * pq->itemSize) > 0) {
      _swap((char*)pq->data + i * pq->itemSize, (char*)pq->data + p * pq->itemSize, pq->itemSize);
      i = p;
    }
    else break;
  }
}

static inline void _shiftDown(CPrioQ* pq, size_t i) {
  while (1) {
    size_t l = _left(i); size_t r = _right(i); size_t curr = i;
    if (l < pq->size && pq->comp((char*)pq->data + l * pq->itemSize, (char*)pq->data + curr * pq->itemSize) > 0) {
      curr = l;
    }
    if (r < pq->size && pq->comp((char*)pq->data + r * pq->itemSize, (char*)pq->data + curr * pq->itemSize) > 0) {
      curr = r;
    }

    if (curr != i) {
      _swap((char*)pq->data + i * pq->itemSize, (char*)pq->data + curr * pq->itemSize, pq->itemSize);
      i = curr;
    }
    else break;
  }
}


static inline void _CPrioQ_push(CPrioQ* pq, void* itemPtr) {
  if (pq->capacity == 0) CPrioQ_resize(pq, 4);
  else if (pq->capacity == pq->size) CPrioQ_resize(pq, ceilf(pq->capacity * 1.6));

  memcpy((char*)pq->data + pq->size * pq->itemSize, itemPtr, pq->itemSize);
  _shiftUp(pq, pq->size);
  pq->size++;
}
#define CPrioQ_push(pq, item) _CPrioQ_push(pq, (void*)&item)
#define CPrioQ_emplAs(pq, T, ...) \
  _CPrioQ_push((pq), &(T){__VA_ARGS__} )
#define CPrioQ_empl(pq, val) \
  do { \
    assert(sizeof(__typeof__(val)) == (pq)->itemSize);   \
    __typeof__(val) tmp = (val); \
    _CPrioQ_push((pq), &tmp); \
  } while (0)

#define _PQ_EMPL_PICK(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define CPrioQ_emplace(...) \
  _PQ_EMPL_PICK(__VA_ARGS__, \
    CPrioQ_emplAs, CPrioQ_emplAs, CPrioQ_emplAs, CPrioQ_emplAs, \
    CPrioQ_emplAs, CPrioQ_emplAs, CPrioQ_empl, _PQ_EMPL_ERR)(__VA_ARGS__)


static inline void* _CPrioQ_pop(CPrioQ* pq) {
  if (pq->size == 0) return pq->data;
  _swap((char*)pq->data, (char*)pq->data + (pq->size - 1) * pq->itemSize, pq->itemSize);
  pq->size--;
  _shiftDown(pq, 0);
  return (char*)pq->data + pq->size * pq->itemSize; 
}
#define CPrioQ_pop(pq, T) (*(T*)_CPrioQ_pop(pq))
#define CPrioQ_front(pq, T) (*(T*)((char*)pq->data))

static inline void CPrioQ_free(CPrioQ* pq) { free(pq->data); }

#define pqInitPrim(pq, T, cap, ORDER) CPrioQ_initPrim(pq, T, cap, ORDER)
#define pqInitCustom(pq, T, cap, comp) CPrioQ_initCustom(pq, T, cap, comp)
#define pqPush(pq, item) CPrioQ_push(pq, item)
#define pqEmpl(...) CPrioQ_emplace(__VA_ARGS__)
#define pqPop(pq, T) CPrioQ_pop(pq, T)
#define pqFront(pq, T) CPrioQ_front(pq, T)
#define pqFree(pq) CPrioQ_free(pq)
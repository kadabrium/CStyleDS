#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//#include "_ctypeof.h"

typedef struct { 
  void** data;
  size_t itemSize;
  int rows;
  int rowNumCap;
  int* rowLens; // cols
  int* rowLenCaps; // these 2 arr len == num rows
} CMat;


static inline void m_CMat_init(CMat* m, size_t itemSize, int rows, int cols) {
  m->itemSize = itemSize;
  // Start empty: allocate nothing. The first m_push/_addRow grows it.
  if (rows <= 0 || cols <= 0) {
    m->rows = 0; m->rowNumCap = 0;
    m->data = NULL; m->rowLens = NULL; m->rowLenCaps = NULL;
    return;
  }
  m->rows = rows; m->rowNumCap = rows;
  m->data = (void**)malloc(sizeof(void*) * rows);
  m->rowLens = (int*)malloc(sizeof(int) * rows);
  m->rowLenCaps = (int*)malloc(sizeof(int) * rows);
  for (int i = 0; i < rows; i++) {
    (m->data)[i] = malloc(itemSize * cols);
    (m->rowLens)[i] = cols; (m->rowLenCaps)[i] = cols;
  }
}
#define CMat_init(m, T, r, c) m_CMat_init(m, sizeof(T), r, c)
#define CMat_data(m, T, r) ((T*)((m)->data[r]))
#define CMat_at(m, T, r, c) \
  (*(T*)((char*)((m)->data[r]) + ((m)->itemSize * (c))))
#define CMat_front(m, T, r) (*(T*)((m)->data[r]))
#define CMat_back(m, T, r) \
  (*(T*)((char*)((m)->data[r]) + ((m)->itemSize * ((m)->rowLens[r] - 1))))
#define CMat_rows(m) ((m)->rows)
#define CMat_cols(m, r) ((m)->rowLens[r])


static inline void m_CMat_incRowNumCap(CMat* m, int newCap) {
  if (newCap <= m->rowNumCap) return;
  void** nd = (void**)realloc(m->data, sizeof(void*) * newCap);
  if (!nd) return;
  m->data = nd;
  int* nl = (int*)realloc(m->rowLens, sizeof(int) * newCap);
  if (!nl) return;
  m->rowLens = nl;
  int* nlc = (int*)realloc(m->rowLenCaps, sizeof(int) * newCap);
  if (!nlc) return;
  m->rowLenCaps = nlc;
  for (int i = m->rowNumCap; i < newCap; i++) { 
    m->rowLens[i] = 0; m->rowLenCaps[i] = 0;
  }
  m->rowNumCap = newCap;
}

// also returns index when appending.
static inline int CMat_addRow(CMat* m, int lenCap) {
  if (lenCap <= 0) { lenCap = 2; }
  if (m->rows == m->rowNumCap) {
    int newCap = m->rowNumCap == 0 ? 2 : (int)ceilf(m->rowNumCap * 1.5f);
    m_CMat_incRowNumCap(m, newCap);
  }
  int idx = m->rows; 
  m->data[idx] = malloc(m->itemSize * lenCap);
  m->rowLens[idx] = 0; m->rowLenCaps[idx] = lenCap;
  (m->rows)++;
  return idx;
}

static inline void m_CMat_resizeRow(CMat* m, int row, int newCap) {
  void* tmp = realloc(m->data[row], m->itemSize * newCap);
  if (tmp) { m->data[row] = tmp; m->rowLenCaps[row] = newCap; }
}

static inline void m_CMat_pushToRow(CMat* m, void* itemPtr, int row) {
  if (m->rowLenCaps[row] == 0) {
    m_CMat_resizeRow(m, row, 4); 
  } 
  else if (m->rowLens[row] == m->rowLenCaps[row]) {
    m_CMat_resizeRow(m, row, (int)ceilf(m->rowLenCaps[row] * 1.6f));
  }
  memcpy((char*)(m->data[row]) + (m->rowLens[row] * m->itemSize), itemPtr, m->itemSize);
  (m->rowLens[row])++;
}
#define CMat_push(m, r, item) m_CMat_pushToRow((m), (void*)&item, (r))


static inline void m_CMat_pushLastRow(CMat* m, void* itemPtr) {
  m_CMat_pushToRow(m, itemPtr, m->rows - 1);
}
#define CMat_pushLast(m, item) m_CMat_pushLastRow((m), (void*)&item)

// emplace struct 
#define CMat_emplAs(m, r, T, ...) \
  m_CMat_pushToRow((m), &(T){__VA_ARGS__}, (r))
// emplace primitive
#define CMat_empl(m, r, val) \
  do { \
    __typeof__(val) _tmp = (val); \
    m_CMat_pushToRow((m), &_tmp, (r)); \
  } while (0)
#define m_MEMPL_PICK(_1, _2, _3, _4, _5, _6, _7, _8, _9, NAME, ...) NAME
#define CMat_emplace(...) \
  m_MEMPL_PICK(__VA_ARGS__, \
    CMat_emplAs, CMat_emplAs, CMat_emplAs, CMat_emplAs, \
    CMat_emplAs, CMat_emplAs, CMat_empl, m_MEMPL_ERR, m_MEMPL_ERR)(__VA_ARGS__)

#define CMat_emplLastAs(m, T, ...) \
  m_CMat_pushLastRow((m), &(T){__VA_ARGS__})
#define CMat_emplaceLast(m, r, val) \
  do { \
    __typeof__(val) _tmp = (val); \
    m_CMat_pushLastRow((m), &_tmp); \
  } while (0)
#define m_MEMPLAST_PICK(_1, _2, _3, _4, _5, _6, _7, _8, _9, NAME, ...) NAME
#define CMat_emplLast(...) \
  m_MEMPLAST_PICK(__VA_ARGS__, \
    CMat_emplLastAs, CMat_emplLastAs, CMat_emplLastAs, CMat_emplLastAs, \
    CMat_emplLastAs, CMat_emplLastAs, CMat_emplaceLast, m_MEMPL_ERR, m_MEMPL_ERR)(__VA_ARGS__)

    
static inline void* m_CMat_pop(CMat* m, int row) {
  if (m->rowLens[row] == 0) return m->data[row];
  (m->rowLens[row])--;
  return (void*)((char*)(m->data[row]) + (m->rowLens[row] * m->itemSize));
}
#define CMat_pop(m, T, r) (*(T*)m_CMat_pop((m), (r)))


static inline void* m_CMat_popLastRow(CMat* m) {
  if (m->rowLens[m->rows - 1] == 0) return m->data[m->rows - 1];
  (m->rowLens[m->rows - 1])--;
  return (void*)((char*)(m->data[m->rows - 1]) + (m->rowLens[m->rows - 1] * m->itemSize));
}
#define CMat_popLast(m, T) (*(T*)m_CMat_popLastRow(m))

static inline void CMat_free(CMat* m) {
  for (int i = 0; i < m->rows; i++) free(m->data[i]); 
  free(m->data); free(m->rowLens); free(m->rowLenCaps);
}


#define mInit(m, T, r, c) CMat_init(m, T, r, c)
#define mAt(m, T, r, c) CMat_at(m, T, r, c)
#define mGetRow(m, T, r) CMat_data(m, T, r)
#define mNumRows(m) CMat_rows(m)
#define mRowLen(m, r) CMat_cols(m, r) 
#define mAddRow(m, cap) CMat_addRow(m, cap)
#define mPush(m, r, item) CMat_push(m, r, item)
#define mPushLast(m, item) CMat_pushLast(m, item)
#define mEmpl(...) CMat_emplace(__VA_ARGS__)
#define mEmplLast(...) CMat_emplLast(__VA_ARGS__)
#define mPop(m, T, r) CMat_pop(m, T, r)
#define mPopLast(m, T) CMat_popLast(m, T)
#define mRowFront(m, T, r) CMat_front(m, T, r)
#define mRowBack(m, T, r) CMat_back(m, T, r)
#define mFree(m) CMat_free(m)
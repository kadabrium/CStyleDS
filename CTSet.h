#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
//#include "_ctypeof.h"

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

typedef struct TSNode {
  struct TSNode* left;
  struct TSNode* right;
  unsigned char data[];
} TSNode;

typedef struct {
  size_t size;
  size_t prevMaxSz;
  size_t itemSize;
  int (*comp)(const void*, const void*);
  TSNode* root;
} CTSet;


void CTSet_init_(CTSet* tree, size_t itemSize, int (*comp)(const void*, const void*)) {
  tree->size = 0; tree->prevMaxSz = 0; 
  tree->itemSize = itemSize;
  tree->comp = comp;
  tree->root = NULL;
}
#define CTSet_initPrim(tree, T, ORDER)  CTSet_init_(tree, sizeof(T), _PRIM_##ORDER(T))
#define CTSet_initCustom(tree, T, comp) CTSet_init_(tree, sizeof(T), comp)

TSNode* CTSet_newNode_(CTSet* tree, void* itemPtr) {
  TSNode* newNode = (TSNode*)malloc(offsetof(TSNode, data) + (size_t)tree->itemSize);
  newNode->left = NULL; newNode->right = NULL;
  memcpy(newNode->data, itemPtr, tree->itemSize);
  return newNode;
}

void TSFlatten_(TSNode* node) {
  TSNode* prev = node; 
  TSNode* curr = node->right; if (curr == NULL) return;
  while (curr->left == NULL) { 
    prev = prev->right;
    curr = curr->right; 
    if (curr == NULL) return; 
  }
  TSNode* left = curr->left;
  TSNode* end = curr->left; 
  while (end->right != NULL) { end = end->right; }
  end->right = curr;
  prev->right = left;
  curr->left = NULL;
  TSFlatten_(prev);
  
}

TSNode* TSRebuild_(TSNode** nodeBox, size_t size) {
  if (size <= 0 || *nodeBox == NULL) { return NULL; }
  if (size == 1) { 
      (*nodeBox)->right = NULL; (*nodeBox)->left = NULL; 
      return *nodeBox; 
  }
  TSNode* mid = *nodeBox; 
  for (int i = 0; i < size / 2; i++) mid = mid->right;
  TSNode* buildLeft = TSRebuild_(nodeBox, size / 2);
  mid->left = buildLeft;
  *nodeBox = mid->right;
  mid->right = TSRebuild_(nodeBox, size - 1 - size/2 );
  return mid;
}

size_t TSSize_(TSNode* node) {
  if (node == NULL) return 0;
  int mid = 1;
  return mid + TSSize_(node->left) + TSSize_(node->right);
}

void TSRebalance_(CTSet* tree, TSNode* newNode, TSNode* path[], size_t nodeHt) {
  // traverse from newNode up
  // rebalance at lowest node where half/total starts to be > 2/3 
  size_t half = 1; 
  TSNode* child = newNode;
  for (size_t i = nodeHt; i >= 0; i--) {
    TSNode* x = path[i];
    TSNode* sib = (x->left == child)? x->right : x->left;
    size_t total = half + TSSize_(sib) + 1;         
    if (half * 3 > total * 2) { 
      TSNode dummy; dummy.right = x;
      TSFlatten_(&dummy);
      TSNode* fixed = TSRebuild_(&(dummy.right), total);
      if (i == 0) { tree->root = fixed; }
      else if (path[i-1]->left == x) { path[i-1]->left = fixed; }
      else { path[i-1]->right = fixed; }
      break;
    }
    half = total; child = x;
  }

}

static inline size_t TSHtCap_(size_t n) {
  size_t lg = 0;
  while (n > 1) { 
    n >>= 1; 
    lg++; 
  } 
  return (lg * 27) >> 4;  // alpha 1/1.6875
}

void CTSet_put_(CTSet* tree, void* itemPtr) {
  TSNode* curr = tree->root;
  if (curr == NULL) {
    tree->root = CTSet_newNode_(tree, itemPtr);
    if (tree->size == tree->prevMaxSz) { (tree->prevMaxSz)++; }
    (tree->size)++;
    return;
  }
  int nodeHt = 0;
  TSNode* path[64]; 
  while (true) {
    if (tree->comp(curr->data, itemPtr) == 0) { return; }
    else if (tree->comp(curr->data, itemPtr) > 0) {
      if (curr->left == NULL) break;
      else { 
        path[nodeHt] = curr;
        curr = curr->left; 
        nodeHt++;
      }
    }
    else {
      if (curr->right == NULL) break;
      else { 
        path[nodeHt] = curr;
        curr = curr->right; 
        nodeHt++; 
      }
    }
  }
  path[nodeHt] = curr; 
  TSNode* newNode = CTSet_newNode_(tree, itemPtr);
  if (tree->comp(curr->data, itemPtr) > 0) curr->left = newNode;
  else { curr->right = newNode; }
  if (tree->size == tree->prevMaxSz) { (tree->prevMaxSz)++; }
  (tree->size)++; 
 
  if (tree->size > 8 && nodeHt > TSHtCap_(tree->size)) {
    TSRebalance_(tree, newNode, path, nodeHt);
  }
}
#define CTSet_put(tree, item) CTSet_put_(tree, (void*)&item)

bool CTSet_hasRec_(TSNode* node, void* itemPtr, int (*comp)(const void*, const void*)) {
  if (node != NULL) {
    if (comp(node->data, itemPtr) == 0) { return true; }
    else if (comp(node->data, itemPtr) > 0) { return CTSet_hasRec_(node->left, itemPtr, comp); }
    else { return CTSet_hasRec_(node->right, itemPtr, comp); }
  }
  else return false;
}

bool CTSet_has(CTSet* tree, void* itemPtr) {
  return CTSet_hasRec_(tree->root, itemPtr, tree->comp);
}
#define CTSet_has(tree, item) CTSet_has(tree, (void*)&item)

// free this node after swapping toRem's contents with it
TSNode* TSLeafSwap_(CTSet* tree, TSNode* toRem) {
  TSNode* curr = toRem->right;
  TSNode* prev = NULL;
  while (curr != NULL && curr->left != NULL) {
    prev = curr;
    curr = curr->left;
  }
  if (prev != NULL) prev->left = NULL;
  memcpy(toRem->data, curr->data, tree->itemSize);
  return curr;

}

void CTSet_removeNode_(CTSet* tree, TSNode* toRem) {
  if (toRem->left == NULL && toRem->right == NULL) {
    free(toRem);
  }
  else if (toRem->left != NULL && toRem->right == NULL) {
    TSNode* temp = toRem->left;
    memcpy(toRem->data, temp->data, tree->itemSize);
    toRem->left = temp->left;
    toRem->right = temp->right;
    free(temp);
  }
  else if (toRem->left == NULL && toRem->right != NULL) {
    TSNode* temp = toRem->right;
    memcpy(toRem->data, temp->data, tree->itemSize);
    toRem->left = temp->left;
    toRem->right = temp->right;
    free(temp);
  }
  else {
    TSNode* leaf = TSLeafSwap_(tree, toRem);
    free(leaf);
  }
  tree->size--;

  if (tree->size > 8 && tree->size < 0.6 * tree->prevMaxSz) {
    TSNode dummy; dummy.right = tree->root;
    TSFlatten_(&dummy);
    tree->root = TSRebuild_(&(dummy.right), tree->size); 
    tree->prevMaxSz = tree->size;
  }
}

void CTSet_removeKey_(CTSet* tree, void* itemPtr) {
  TSNode* curr = tree->root;
  if (tree->root == NULL) return;
  if (tree->comp(curr->data, itemPtr) == 0) {
    CTSet_removeNode_(tree, curr);
    if(tree->size == 0) tree->root = NULL;
    return;
  }
  while (curr != NULL) {
    if (curr->left != NULL && tree->comp(curr->left->data, itemPtr) == 0) {
      CTSet_removeNode_(tree, curr->left);
    }
    else if (curr->right != NULL && tree->comp(curr->right->data, itemPtr) == 0) {
      CTSet_removeNode_(tree, curr->right);
    }
    else {
      if (curr->left != NULL && tree->comp(curr->left->data, itemPtr) > 0) {
        curr = curr->left;
      }
      else { curr = curr->right; }
    }
  }
}

void CTSet_hasNext_(CTSet* tree, void* itemPtr) {

}

void CTSet_getNext_(CTSet* tree, void* itemPtr) {

}
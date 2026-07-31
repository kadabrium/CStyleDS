#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "_ctypeof.h"

typedef struct TSNode {
  struct TSNode* left;
  struct TSNode* right;
  //void* data;
  unsigned char data[1];
} TSNode;

typedef struct {
  size_t size;
  size_t itemSize;
  TSNode* root;
} CTSet;

TSNode* _CTSet_newNode(CTSet* tree) {
  return (TSNode*)malloc(offsetof(TSNode, data) + (size_t)tree->itemSize);
}

void _CTSet_init(CTSet* tree, size_t itemSize) {
  tree->size = 0; tree->itemSize = itemSize;
  tree->root = _CTSet_newNode(tree);
}
#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "_ctypeof.h"

static inline size_t _HS_byteHash(const void* item, size_t itemSize) {
  const unsigned char* bytes = (const unsigned char*)item;
  size_t hash = (size_t)14695981039346656037ULL;   // FNV-1a 64 offset basis
  for (size_t i = 0; i < itemSize; i++) {
    hash ^= bytes[i];
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}
static inline int _HS_byteEq(const void* a, const void* b, size_t size) {
  return memcmp(a, b, size) == 0;
}

#define _HS_PRIM_HASH(T) _Generic((T){0}, \
  int: _HS_byteHash, unsigned: _HS_byteHash, long: _HS_byteHash, unsigned long: _HS_byteHash, \
  long long: _HS_byteHash, unsigned long long: _HS_byteHash, \
  float: _HS_byteHash, double: _HS_byteHash, char: _HS_byteHash, \
  default: (HS_HashFn)NULL)

#define _HS_PRIM_EQ(T) _Generic((T){0}, \
  int: _HS_byteEq, unsigned: _HS_byteEq, long: _HS_byteEq, unsigned long: _HS_byteEq, \
  long long: _HS_byteEq, unsigned long long: _HS_byteEq, \
  float: _HS_byteEq, double: _HS_byteEq, char: _HS_byteEq, \
  default: (HS_EqFn)NULL)

enum HS_IndexState {
  _CHSet_EMPTY = ((int64_t)-1),
  _CHSet_DUMMY = ((int64_t)-2),
  _CHSet_HOLE = ((size_t)SIZE_MAX) // deleted entry
};
typedef size_t (*HS_HashFn)(const void* key, size_t keySize);
typedef int (*HS_EqFn)(const void* a, const void* b, size_t size);

typedef struct {
  int64_t* indices;
  void* keys;
  size_t* hashes;    
  size_t totCap;     
  size_t entryCap;   
  size_t size;       
  size_t nEntries;     
  size_t keyItemSize;
  HS_HashFn hash;
  HS_EqFn eq;
} CHSet;

static inline size_t _CHSet_h(CHSet* map, const void* key) {
  size_t h = map->hash(key, map->keyItemSize);
  return h == _CHSet_HOLE ? _CHSet_HOLE - 1 : h; 
}


static inline void _CHSet_init(CHSet* map, size_t keySz, size_t cap, HS_HashFn hash, HS_EqFn eq) {
  size_t want = cap < 4 ? 4 : cap;
  size_t totCap = 8;
  while (totCap * 2 / 3 < want) totCap <<= 1;
  map->totCap = totCap; map->entryCap = totCap * 2 / 3;

  map->size = 0; map->nEntries = 0;
  map->keyItemSize = keySz; 
  map->hash = hash; map->eq = eq;

  map->keys = malloc(map->entryCap * keySz);
  map->hashes = (size_t*)malloc(map->entryCap * sizeof(size_t));
  map->indices = (int64_t*)malloc(totCap * sizeof(int64_t));
  for (size_t i = 0; i < totCap; i++) map->indices[i] = _CHSet_EMPTY;
}
#define CHSet_initPrim(map, K, cap) \
  _CHSet_init(map, sizeof(K), cap, _HS_PRIM_HASH(K), _HS_PRIM_EQ(K))
#define CHSet_initCustom(map, K, cap, hashFn, eqFn) \
  _CHSet_init(map, sizeof(K), cap, hashFn, eqFn)


static inline void _CHSet_placeIndex(CHSet* map, size_t hash, int64_t entry) {
  size_t mask = map->totCap - 1;
  size_t i = hash & mask; size_t perturb = hash;
  while (map->indices[i] != _CHSet_EMPTY) {
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
  map->indices[i] = entry;
}

static inline void _CHSet_resize(CHSet* map) {
  size_t newCap = 8;
  while (newCap * 2 / 3 <= map->size) newCap <<= 1;   // until > 1/3 empty
  size_t newEntryCap = newCap * 2 / 3;

  int64_t* indices = (int64_t*)malloc(newCap * sizeof(int64_t));
  for (size_t i = 0; i < newCap; i++) indices[i] = _CHSet_EMPTY;

  void* keys = malloc(newEntryCap * map->keyItemSize);
  size_t* hashes = (size_t*)malloc(newEntryCap * sizeof(size_t));
  
  int64_t* oldIndices = map->indices;
 
  size_t j = 0;
  for (size_t e = 0; e < map->nEntries; e++) {
    if (map->hashes[e] == _CHSet_HOLE) continue;
    memcpy((char*)keys + j * map->keyItemSize,
    	(char*)map->keys + e * map->keyItemSize, map->keyItemSize);
    hashes[j] = map->hashes[e];
    j++;
  }
  free(oldIndices); free(map->keys); free(map->hashes);

  map->indices = indices; map->keys = keys;
  map->hashes = hashes;
  map->totCap = newCap; map->entryCap = newEntryCap;
  map->nEntries = j;  

  for (size_t e = 0; e < map->nEntries; e++) _CHSet_placeIndex(map, map->hashes[e], (int64_t)e);
}

static inline size_t _CHSet_findSlot(CHSet* map, const void* key, size_t hash, int64_t* existing) {
  size_t mask = map->totCap - 1;
  size_t i = hash & mask; size_t perturb = hash;
  size_t freeslot = 0;
  bool haveFree = false;
  while (true) {
    int64_t ix = map->indices[i];
    if (ix == _CHSet_EMPTY) {
      *existing = -1;
      return haveFree ? freeslot : i;
    }

    if (ix == _CHSet_DUMMY && !haveFree) {
      haveFree = true; freeslot = i; 
		} 
		else if (map->hashes[ix] == hash 
		  && map->eq((char*)map->keys + ix * map->keyItemSize, key, map->keyItemSize)) {
			*existing = ix;
			return i;
    }
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
}


static inline void _CHSet_put(CHSet* map, const void* key) {
  if (map->nEntries >= map->entryCap) _CHSet_resize(map);
  size_t h = _CHSet_h(map, key);
  int64_t existing;
  size_t slot = _CHSet_findSlot(map, key, h, &existing);
  if (existing >= 0) { 
    return;
  }
  int64_t e = (int64_t)map->nEntries++;
  memcpy((char*)map->keys + e * map->keyItemSize, key, map->keyItemSize);
  map->hashes[e] = h;
  map->indices[slot] = e;
  map->size++;
}
#define CHSet_put(map, key) _CHSet_put((map), &(key))
#define CHSet_emplace(map, K, ...) _CHSet_put((map), &(K){__VA_ARGS__})


static inline bool _CHSet_has(CHSet* map, const void* key) {
  size_t h = _CHSet_h(map, key);
  size_t mask = map->totCap - 1;
  size_t i = h & mask; size_t perturb = h;
  while (true) {
    int64_t ix = map->indices[i];
    if (ix == _CHSet_EMPTY) return false;
    if (ix != _CHSet_DUMMY && map->hashes[ix] == h 
			&& map->eq((char*)map->keys + ix * map->keyItemSize, key, map->keyItemSize)) {
			return true;
		}
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
}
// See EOF of CHMap for getter usage guide
#define CHSet_has(map, keyExpr) (_CHSet_has(map, &(__typeof__(keyExpr)){(keyExpr)}) == true)
#define CHSet_hasObj(map, ...) (_CHSet_has((map), &(__VA_ARGS__)) == true)


static inline int _CHSet_remove(CHSet* map, const void* key) {
  size_t h = _CHSet_h(map, key);
  size_t mask = map->totCap - 1;
  size_t i = h & mask; size_t perturb = h;
  while (true) {
    int64_t ix = map->indices[i];
    if (ix == _CHSet_EMPTY) return 0; 
    if (ix != _CHSet_DUMMY && map->hashes[ix] == h 
			&& map->eq((char*)map->keys + ix * map->keyItemSize, key, map->keyItemSize)) {
      map->indices[i] = _CHSet_DUMMY; 
      map->hashes[ix] = _CHSet_HOLE;
      map->size--;
      return 1;
    }
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
}
#define CHSet_remove(map, keyExpr) \
  _CHSet_remove((map), &(__typeof__(keyExpr)){(keyExpr)})
#define CHSet_removeObj(map, ...) _CHSet_remove((map), &(__VA_ARGS__))


static inline int64_t CHSet_next(CHSet* map, int64_t it, void** keyOut) {
  for (int64_t e = it + 1; e < (int64_t)map->nEntries; e++) {
    if (map->hashes[e] == _CHSet_HOLE) continue;
    *keyOut = (char*)map->keys + e * map->keyItemSize;
    return e;
  }
  return -1;
}

static inline void CHSet_free(CHSet* map) {
  free(map->indices); free(map->keys);
  free(map->hashes);
}

#define hsInitPrim(m, K, cap) CHSet_initPrim(m, K, cap)
#define hsInitCustom(m, K, cap, h, e) CHSet_initCustom(m, K, cap, h, e)
#define hsPut(m, k) CHSet_put(m, k)
#define hsEmpl(m, K, ...) CHSet_emplace(m, K, __VA_ARGS__)
// See EOF of CHMap for getter usage guide
#define hsHas(m, k) CHSet_has(m, k)
#define hsRem(m, k) CHSet_remove(m, k)
#define hsHasObj(m, ...) CHSet_hasObj(m, __VA_ARGS__)
#define hsRemObj(m, ...) CHSet_removeObj(m, __VA_ARGS__)
#define hsNext(m, it, ko) CHSet_next(m, it, ko)
#define hsFree(m) CHSet_free(m)

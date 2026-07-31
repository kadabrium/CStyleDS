#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "_ctypeof.h"


// Predefined primitive bytewise hash. 
// For custom hash, see CHMCustomHash for examples.
static inline size_t _byteHash(const void* item, size_t itemSize) {
  const unsigned char* bytes = (const unsigned char*)item;
  size_t hash = (size_t)14695981039346656037ULL;   // FNV-1a 64 offset basis
  for (size_t i = 0; i < itemSize; i++) {
    hash ^= bytes[i];
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}
static inline int _byteEq(const void* a, const void* b, size_t size) {
  return memcmp(a, b, size) == 0;
}

// list of primitives
#define _PRIM_HASH(T) _Generic((T){0}, \
  int: _byteHash, unsigned: _byteHash, long: _byteHash, unsigned long: _byteHash, \
  long long: _byteHash, unsigned long long: _byteHash, \
  float: _byteHash, double: _byteHash, char: _byteHash, \
  default: (HashFn)NULL)

#define _PRIM_EQ(T) _Generic((T){0}, \
  int: _byteEq, unsigned: _byteEq, long: _byteEq, unsigned long: _byteEq, \
  long long: _byteEq, unsigned long long: _byteEq, \
  float: _byteEq, double: _byteEq, char: _byteEq, \
  default: (EqFn)NULL)

// indices state
// indices: the hash table proper. Each slot state is EMPTY, DUMMY, or an index into
// the dense entry arrays. keys/values/hashes[] are appended in insertion order.
enum IndexState {
  _CHMAP_EMPTY = ((int64_t)-1),
  _CHMAP_DUMMY = ((int64_t)-2),
  _CHMAP_HOLE = ((size_t)SIZE_MAX) // deleted entry
};
typedef size_t (*HashFn)(const void* key, size_t keySize);
typedef int (*EqFn)(const void* a, const void* b, size_t size);

typedef struct {
  int64_t* indices;
  void* keys;
  void* values;
  size_t* hashes;        // cached hash of entries
  size_t totCap;      // length of indices array in power of two
  size_t entryCap;      // length of hash and item arrays = totCap * 2/3
  size_t size;        // num existing entries
  size_t nEntries;      // num used slots including deleted 
  size_t keyItemSize;
  size_t valItemSize;
  HashFn hash;
  EqFn eq;
} CHMap;

// normalizing hash keeping hole marker free
static inline size_t _CHMap_h(CHMap* map, const void* key) {
  size_t h = map->hash(key, map->keyItemSize);
  return h == _CHMAP_HOLE ? _CHMAP_HOLE - 1 : h; 
}

static inline void _CHMap_init(CHMap* map, size_t keySz, size_t valSz, size_t cap,
                               HashFn hash, EqFn eq) {
  size_t want = cap < 4 ? 4 : cap;
  size_t totCap = 8;
  while (totCap * 2 / 3 < want) totCap <<= 1;
  map->totCap = totCap; map->entryCap = totCap * 2 / 3;

  map->size = 0; map->nEntries = 0;
  map->keyItemSize = keySz; map->valItemSize = valSz;
  map->hash = hash; map->eq = eq;

  map->keys = malloc(map->entryCap * keySz);
  map->values = malloc(map->entryCap * valSz);
  map->hashes = (size_t*)malloc(map->entryCap * sizeof(size_t));
  map->indices = (int64_t*)malloc(totCap * sizeof(int64_t));
  for (size_t i = 0; i < totCap; i++) map->indices[i] = _CHMAP_EMPTY;
}
#define CHMap_initPrim(map, K, V, cap) \
  _CHMap_init(map, sizeof(K), sizeof(V), cap, _PRIM_HASH(K), _PRIM_EQ(K))
#define CHMap_initCustom(map, K, V, cap, hashFn, eqFn) \
  _CHMap_init(map, sizeof(K), sizeof(V), cap, hashFn, eqFn)


// place a dense entry into a fresh EMPTY table (eg when resizing)
static inline void _CHMap_placeIndex(CHMap* map, size_t hash, int64_t entry) {
  size_t mask = map->totCap - 1;
  size_t i = hash & mask; size_t perturb = hash;
  while (map->indices[i] != _CHMAP_EMPTY) {
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
  map->indices[i] = entry;
}

static inline void _CHMap_resize(CHMap* map) {
  size_t newCap = 8;
  while (newCap * 2 / 3 <= map->size) newCap <<= 1;   // until > 1/3 empty
  size_t newEntryCap = newCap * 2 / 3;

  int64_t* indices = (int64_t*)malloc(newCap * sizeof(int64_t));
  void* keys = malloc(newEntryCap * map->keyItemSize);
  void* values = malloc(newEntryCap * map->valItemSize);
  size_t* hashes = (size_t*)malloc(newEntryCap * sizeof(size_t));
  assert(indices && keys && values && hashes);
  for (size_t i = 0; i < newCap; i++) indices[i] = _CHMAP_EMPTY;

  int64_t* oldIndices = map->indices;
  // compact live entries densely, dropping holes
  size_t j = 0;
  for (size_t e = 0; e < map->nEntries; e++) {
    if (map->hashes[e] == _CHMAP_HOLE) continue;
    memcpy((char*)keys + j * map->keyItemSize,
    	(char*)map->keys + e * map->keyItemSize, map->keyItemSize);
    memcpy((char*)values + j * map->valItemSize,
      (char*)map->values + e * map->valItemSize, map->valItemSize);
    hashes[j] = map->hashes[e];
    j++;
  }
  free(oldIndices); free(map->keys); free(map->values); free(map->hashes);

  map->indices = indices; map->keys = keys;
  map->values = values; map->hashes = hashes;
  map->totCap = newCap; map->entryCap = newEntryCap;
  map->nEntries = j;   // == size, no holes when new

  for (size_t e = 0; e < map->nEntries; e++) _CHMap_placeIndex(map, map->hashes[e], (int64_t)e);
}

// find indices slot to insert key; *existing = matching dense entry or -1
static inline size_t _CHMap_findSlot(CHMap* map, const void* key, size_t hash, int64_t* existing) {
  size_t mask = map->totCap - 1;
  size_t i = hash & mask; size_t perturb = hash;
  size_t freeslot = 0;
  bool haveFree = false;
  while (true) {
    int64_t ix = map->indices[i];
    if (ix == _CHMAP_EMPTY) {
      *existing = -1;
      return haveFree ? freeslot : i;    // reclaim an earlier tombstone if seen
    }

    if (ix == _CHMAP_DUMMY && !haveFree) {
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


static inline void _CHMap_put(CHMap* map, const void* key, const void* val) {
  if (map->nEntries >= map->entryCap) _CHMap_resize(map);
  size_t h = _CHMap_h(map, key);
  int64_t existing;
  size_t slot = _CHMap_findSlot(map, key, h, &existing);
  if (existing >= 0) { // update value in place
    memcpy((char*)map->values + existing * map->valItemSize, val, map->valItemSize);
    return;
  }
  int64_t e = (int64_t)map->nEntries++;
  memcpy((char*)map->keys + e * map->keyItemSize, key, map->keyItemSize);
  memcpy((char*)map->values + e * map->valItemSize, val, map->valItemSize);
  map->hashes[e] = h;
  map->indices[slot] = e;
  map->size++;
}
#define CHMap_put(map, key, val) _CHMap_put((map), &(key), &(val))
#define CHMap_emplace(map, K, key, V, ...) _CHMap_put((map), &(K){key}, &(V){__VA_ARGS__})


static inline void* _CHMap_get(CHMap* map, const void* key) {
  size_t h = _CHMap_h(map, key);
  size_t mask = map->totCap - 1;
  size_t i = h & mask; size_t perturb = h;
  while (true) {
    int64_t ix = map->indices[i];
    if (ix == _CHMAP_EMPTY) return NULL;
    if (ix != _CHMAP_DUMMY && map->hashes[ix] == h 
			&& map->eq((char*)map->keys + ix * map->keyItemSize, key, map->keyItemSize)) {
			return (char*)map->values + ix * map->valItemSize;
		}
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
}
// See EOF for getter usage guide
// Scalar / primitive literal keys getters
#define CHMap_getp(map, keyExpr) _CHMap_get((map), &(__typeof__(keyExpr)){(keyExpr)})
#define CHMap_get(map, V, keyExpr) (*(V*)CHMap_getp(map, keyExpr))
#define CHMap_has(map, keyExpr) (CHMap_getp(map, keyExpr) != NULL)
// Struct / lvalue key getters: take the address directly 
#define CHMap_getpObj(map, ...) _CHMap_get((map), &(__VA_ARGS__))
#define CHMap_getObj(map, V, ...) (*(V*)CHMap_getpObj(map, __VA_ARGS__))
#define CHMap_hasObj(map, ...) (CHMap_getpObj(map, __VA_ARGS__) != NULL)


static inline int _CHMap_remove(CHMap* map, const void* key) {
  size_t h = _CHMap_h(map, key);
  size_t mask = map->totCap - 1;
  size_t i = h & mask; size_t perturb = h;
  while (true) {
    int64_t ix = map->indices[i];
    if (ix == _CHMAP_EMPTY) return 0; 
    if (ix != _CHMAP_DUMMY && map->hashes[ix] == h 
			&& map->eq((char*)map->keys + ix * map->keyItemSize, key, map->keyItemSize)) {
      map->indices[i] = _CHMAP_DUMMY; // tombstone keeps the chain intact
      map->hashes[ix] = _CHMAP_HOLE; // hole is skipped on iteration/compaction
      map->size--;
      return 1;
    }
    i = (i * 5 + perturb + 1) & mask;
    perturb >>= 5;
  }
}
#define CHMap_remove(map, keyExpr) \
  _CHMap_remove((map), &(__typeof__(keyExpr)){(keyExpr)})
#define CHMap_removeObj(map, ...) _CHMap_remove((map), &(__VA_ARGS__))


// ordered iteration: start with it = -1, stop when the return value is < 0
static inline int64_t CHMap_next(CHMap* map, int64_t it, void** keyOut, void** valOut) {
  for (int64_t e = it + 1; e < (int64_t)map->nEntries; e++) {
    if (map->hashes[e] == _CHMAP_HOLE) continue;
    *keyOut = (char*)map->keys + e * map->keyItemSize;
    *valOut = (char*)map->values + e * map->valItemSize;
    return e;
  }
  return -1;
}

static inline void CHMap_free(CHMap* map) {
  free(map->indices); free(map->keys);
  free(map->values); free(map->hashes);
}

#define hmInitPrim(m, K, V, cap) CHMap_initPrim(m, K, V, cap)
#define hmInitCustom(m, K, V, cap, h, e) CHMap_initCustom(m, K, V, cap, h, e)
#define hmPut(m, k, v) CHMap_put(m, k, v)
#define hmEmpl(m, K, k, V, ...) CHMap_emplace(m, K, k, V, __VA_ARGS__)
#define hmGet(m, V, k) CHMap_get(m, V, k)
#define hmGetp(m, k) CHMap_getp(m, k)
#define hmHas(m, k) CHMap_has(m, k)
#define hmRem(m, k) CHMap_remove(m, k)
/**
 * Usage note: the plain getters should be primary choice for any primitive key,
 * regardless of value class (named, array/ptr access, function return, literal).
 * Struct keys must use the Obj getters, which take the key's address. As a rule, 
 * bind an rvalue struct or an initializer list to a named lvalue. This is necessary for
 * function returns, whose result has no address. Although initializer lists are 
 * technically lvalues and as such coincidentally accepted as-is, prefer binding to a 
 * named variable in practice for consistency.
 */
#define hmGetObj(m, V, ...) CHMap_getObj(m, V, __VA_ARGS__)
#define hmGetpObj(m, ...) CHMap_getpObj(m, __VA_ARGS__)
#define hmHasObj(m, ...) CHMap_hasObj(m, __VA_ARGS__)
#define hmRemObj(m, ...) CHMap_removeObj(m, __VA_ARGS__)
#define hmNext(m, it, ko, vo) CHMap_next(m, it, ko, vo)
#define hmFree(m) CHMap_free(m)

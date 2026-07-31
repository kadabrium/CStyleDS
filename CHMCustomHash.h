#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "CHMap.h"
#include "_ctypeof.h"
/*
 * _byteHash / _byteEq operate on an object's representation, not its
 * logical value. CHMap_initPrim intentionally enables them only for the
 * primitive types in _PRIM_HASH / _PRIM_EQ; it does not support arbitrary
 * structs. A struct would be able to use these through CHMap_initCustom only
 * when every byte is deterministically initialized and bytewise equality is
 * exactly the desired equality. Padding bytes commonly make that untrue.
 *
 * Use field/content-based callbacks instead for, for example:
 *
 *   // const char *name;              -> CString_hash / CString_eq
 *   // struct UserKey { int id; char *name; };
 *   //                               -> UserKey_hash / UserKey_eq
 *   // struct PriceKey { int sku; double price; };
 *   //                               -> PriceKey_hash / PriceKey_eq
 *   // struct MessageKey { size_t len; unsigned char data[]; };
 *   //                               -> MessageKey_hash / MessageKey_eq
 *
 * CString_* should hash and compare the pointed-to characters (not the
 * pointer address).  The struct callbacks should combine hashes of their
 * meaningful fields and compare those fields.  PriceKey_* should normalize
 * values such as -0.0 and define an explicit NaN policy.  A flexible-array
 * key also needs a map representation with its full runtime size; sizeof
 * alone cannot store it by value in this fixed-size map.
 */

/* Example: string

	 CHMap names;
	 CHMap_initCustom(&names, const char*, int, 32, CString_hash, CString_eq);

	 const char* k1 = "pea";
	 const char* k2 = "pea";
	 int value = 1;

	 CHMap_put(&names, k1, value);
	 // k2 compares equal by string contents, not pointer address.
*/

static inline size_t CString_hash(const void* key, size_t keySize) {
	(void)keySize;
	const char* s = *(const char* const*)key;
  if (!s) return 0;
	return _byteHash(s, strlen(s));
}

static inline int CString_eq(const void* a, const void* b, size_t keySize) {
	(void)keySize;
  //convert void*: "ptr to item" --> char**: "ptr to string; ptr to char array"
	const char* _a = *(const char* const*)a; 
	const char* _b = *(const char* const*)b;
	if (!_a || !_b) return 0;
  if (_a == _b) return 1;
	return strcmp(_a, _b) == 0;
}

/* Example: struct key, 1 to 5 fields
   Define CHMap-compatible hash/eq callbacks for a named struct by listing
   its key field names. Primitive fields are hashed/compared bytewise and
   C-string fields by content; the right callback is selected per field
   via _Generic. Register other custom field types by extending _FIELD_HASH /
   _FIELD_EQ below.

	 typedef struct {
		 int id;
		 const char* name;
	 } UserKey;

	 HASH_DEFINE(
    UserKey, // Name prefixed to Name##_hash
    UserKey, // struct typename
    id, name) // fields

	 CHMap users;
	 CHMap_initCustom(&users, UserKey, int, 32, UserKey_hash, UserKey_eq);

	 UserKey alice = { .id = 7, .name = "alice" };
	 UserKey alias = { .id = 7, .name = "alice" };
	 int score = 100;

	 CHMap_put(&users, alice, score);
	 // alias compares equal because both fields match by value/content.
*/

static inline size_t _hashCombine(size_t acc, size_t key) {
	acc ^= key + (size_t)0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
	return acc;
}

/* Per-primitive field hashes share the map's _byteHash/_byteEq directly, and
 * raw string fields are provided CString_hash/CString_eq. The macro picks
 * up the field's type via _Generic. To support a custom field type, add a
 * case (its type -> its Type_hash / Type_eq) to the lists below. */
#define _FIELD_HASH(x) _Generic((x), \
  char*: CString_hash, const char*: CString_hash, \
  default: _byteHash)
#define _FIELD_EQ(x) _Generic((x), \
  char*: CString_eq, const char*: CString_eq, \
  default: _byteEq)

/* apply a one-arg macro to each field name; supports up to 5 fields.
 * _EXPAND indirection keeps the arg count correct on MSVC's preprocessor. */
#define _EXPAND(x) x
#define _FE_1(m, a)      m(a)
#define _FE_2(m, a, ...) m(a) _EXPAND(_FE_1(m, __VA_ARGS__))
#define _FE_3(m, a, ...) m(a) _EXPAND(_FE_2(m, __VA_ARGS__))
#define _FE_4(m, a, ...) m(a) _EXPAND(_FE_3(m, __VA_ARGS__))
#define _FE_5(m, a, ...) m(a) _EXPAND(_FE_4(m, __VA_ARGS__))
#define _FE_PICK(_1, _2, _3, _4, _5, N, ...) N
#define _FOR_EACH_FIELD(m, ...) \
  _EXPAND(_FE_PICK(__VA_ARGS__, _FE_5, _FE_4, _FE_3, _FE_2, _FE_1)(m, __VA_ARGS__))

#define _HASH_ONE(f) h = _hashCombine(h, _FIELD_HASH(item->f)(&item->f, sizeof item->f));
#define _EQ_ONE(f)   && _FIELD_EQ(lhs->f)(&lhs->f, &rhs->f, sizeof lhs->f)

/* HASH_DEFINE(Name, Type, field1 [, field2, ... up to field5]) */
#define HASH_DEFINE(Name, Type, ...) \
	static inline size_t Name##_hash(const void* key, size_t keySize) { \
		(void)keySize; \
		const Type* item = (const Type*)key; \
		size_t h = (size_t)14695981039346656037ULL; \
		_FOR_EACH_FIELD(_HASH_ONE, __VA_ARGS__) \
		return h; \
	} \
	static inline int Name##_eq(const void* a, const void* b, size_t keySize) { \
		(void)keySize; \
		const Type* lhs = (const Type*)a; \
		const Type* rhs = (const Type*)b; \
		return 1 _FOR_EACH_FIELD(_EQ_ONE, __VA_ARGS__); \
	}





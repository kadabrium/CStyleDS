//#pragma once
#include "CVec.h"
#include "CDeq.h"
#include "CHMap.h"
#include "CPrioQ.h"
#include "CHMCustomHash.h"

typedef struct {
  int n;
  char c;
} Pair;

void vecTest() {
  CVec v; CVec_init(&v, int, 2);

  int a = 1; int b = 2;
  CVec_push(&v, a); CVec_push(&v, b);
  CVec_emplace(&v, 100); CVec_emplace(&v, 50);
  printf("int vec\n");
  for (int i = 0; i < v.size; i++) {
    printf("%d ", (CVec_data(&v, int))[i]);
  }
  CVec_free(&v);
  printf("\n");

  CVec v2;
  CVec_init(&v2, Pair, 2);
  Pair x = {10, 'z'};
  CVec_push(&v2, x);
  CVec_emplace(&v2, Pair, -11, 'y');
  printf("struct vec\n");
  for (int i = 0; i < v2.size; i++) {
    printf("num: %d ", (CVec_data(&v2, Pair))[i].n);
    printf("char: %c ", (CVec_data(&v2, Pair))[i].c);
  }
  printf("\nvec ops\n");
  CVec_at(&v2, Pair, 1) = (Pair){123, 'a'};
  printf("%d ", (CVec_data(&v2, Pair))[1].n);
  printf("%c ", (CVec_data(&v2, Pair))[1].c);
  printf("\n");
  printf("%c ", CVec_back(&v2, Pair).c);
  Pair p = CVec_pop(&v2, Pair);
  printf("%c ", p.c);
  printf("%c ", CVec_back(&v2, Pair).c);
  CVec_free(&v2);

  printf("\nvec from arr\n");
  int arr[] = {1,3,5,7};
  CVec v3 = CVec_copyFrom(int, arr, 4);
  for (int i = 0; i < v3.size; i++) {
    printf("%d ", (CVec_data(&v3, int))[i]);
  }
  CVec_free(&v3);
}

void dequeTest() {
  CDeq q; CDeq_init(&q, int, 2);
  int a = 2, b = 3, c = 4;
  CDeq_pushFront(&q, a); CDeq_pushFront(&q, b); CDeq_pushBack(&q, c);
  CDeq_emplaceBack(&q, 100); CDeq_emplaceFront(&q, 99); CDeq_emplaceBack(&q, 98);
  printf("\ndeque:\n");
  printf("front %d ", CDeq_front(&q, int));
  printf("back %d ", CDeq_back(&q, int));
  printf("\npopping both ends\n");
  int i = 0;
  while (q.size != 0) {
    if (i % 2 == 0) printf("%d ", CDeq_popFront(&q, int));
    else printf("%d ", CDeq_popBack(&q, int));
    i++;
  }
  CDeq_free(&q);

}

typedef struct {
  int prio;
  const char* name;
} Task;

static int taskMax(const void* a, const void* b) {
  const Task* x = a; const Task* y = b;
  return (x->prio > y->prio) - (x->prio < y->prio);
}

void PQTest() {
  CPrioQ pq; CPrioQ_initPrim(&pq, int, 2, MAX);
  int arr[10] = {1, 3, 5, 8, 2, 4, 4, 6, 7, 2} ;
  for (int i = 0; i < 10; i++) {
    CPrioQ_push(&pq, arr[i]);
  }
  printf("\npq push\n");
  for (int i = 0; i < pq.size; i++) {
    printf("%d ", ((int*)pq.data)[i]);
  }
  printf("\npq pop\n");
  while(pq.size != 0) {
    printf("%d ", CPrioQ_pop(&pq, int));
  }
  CPrioQ_free(&pq);

  CPrioQ mn; CPrioQ_initPrim(&mn, int, 0, MIN);
  printf("\npq emplace\n");
  for (int i = 0; i < 10; i++) { int v = arr[i]; CPrioQ_emplace(&mn, v); }
  while (mn.size != 0) printf("%d ", CPrioQ_pop(&mn, int));
  CPrioQ_free(&mn);

  printf("\npq struct\n");
  CPrioQ tq; CPrioQ_initCustom(&tq, Task, 4, taskMax);
  CPrioQ_emplace(&tq, Task, .prio = 5, .name = "mid");
  CPrioQ_emplace(&tq, Task, .prio = 9, .name = "hi");
  CPrioQ_emplace(&tq, Task, .prio = 1, .name = "lo");
  while (tq.size != 0) {
    Task t = CPrioQ_pop(&tq, Task);
    printf("%s(%d) ", t.name, t.prio);
  }
  CPrioQ_free(&tq);
  printf("\n");
}

// Single-primitive map
void mapTest() {
  CHMap map;
  CHMap_initPrim(&map, int, const char*, 2);   // int key
  for (int i = 0; i < 20; i++) {
    const char* name = i % 2 ? "odd" : "even";
    CHMap_put(&map, i, name);
  }
  int replace = 7;
  CHMap_remove(&map, replace);
  CHMap_emplace(&map, int, replace, const char*, "7777"); 

  printf("\nhashmap: %zu entries", map.size);
  for (int i = 0; i < 10; i++) {
    const char** value = CHMap_getp(&map, i);
    printf("\nhashmap: %s", *value);
  }
  for (int i = 10; i < 20; i++) {
    printf("\nhashmap: %s", CHMap_get(&map, const char*, i));
  }
  
  CHMap_free(&map);
  printf("\n");
}

// Struct maps
// Hash example of struct with prim / string fields (these autopick with _Generic)
typedef struct { int id; const char* name; } UserKey;
HASH_DEFINE(UserKey, UserKey, id, name)  

// Primitive only examples
typedef struct { int a; } OneKey;
HASH_DEFINE(OneKey, OneKey, a)           
typedef struct { int a; double b; char* c; long d; unsigned e; } FiveKey;
HASH_DEFINE(FiveKey, FiveKey, a, b, c, d, e)  

void structMapTest() {
  printf("\nstruct map test\n");
  // UserKey: Struct keys use the _Obj getters; see EOF of CHMap.h for explanation
  CHMap users; CHMap_initCustom(&users, UserKey, int, 8, UserKey_hash, UserKey_eq);
  char alias[8]; snprintf(alias, sizeof alias, "%s", "alice"); // distinct storage
  UserKey a1 = { 7, "alice" };
  UserKey a2 = { 7, alias }; // content equal to a1: should update, not add
  UserKey a3 = { 8, "alice" }; // different id, new entry
  int v1 = 100, v2 = 200, v3 = 300;
  CHMap_put(&users, a1, v1);
  CHMap_put(&users, a2, v2); // matches a1 by value/content, update
  CHMap_put(&users, a3, v3);
  printf("size (expect 2): %zu\n", users.size);
  printf("getRef named eq: %d\n", CHMap_getObj(&users, int, a2));  // 200
  printf("getRef compound lit: %d\n",
    CHMap_getObj(&users, int, (UserKey){ .id = 8, .name = "alice" })); // 300
  printf("hasRef miss: %d\n", CHMap_hasObj(&users, ((UserKey){ 9, "x" }))); // 0
  CHMap_removeObj(&users, a1); // removes the {7,...} entry
  printf("after removeRef size (expect 1): %zu\n", users.size);
  CHMap_free(&users);

  // Hash fn only tests for OneKey/FiveKey
  OneKey o1 = {5}, o2 = {5}, o3 = {6};
  printf("1-field eq/ne: %d %d\n", OneKey_eq(&o1,&o2,0), OneKey_eq(&o1,&o3,0));

  char xb[2] = "x";
  FiveKey f1 = {1, 2.5, "x", 3, 4};
  FiveKey f2 = {1, 2.5, xb,  3, 4};
  FiveKey f3 = {1, 2.5, "x", 3, 9};
  printf("5-field eq/ne: %d %d, hash match: %d\n",
    FiveKey_eq(&f1,&f2,0), FiveKey_eq(&f1,&f3,0),
    FiveKey_hash(&f1,0) == FiveKey_hash(&f2,0));
}

int main() {
  vecTest();
  dequeTest();
  PQTest();
  mapTest();
  structMapTest();

  return 0;
}
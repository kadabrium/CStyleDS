# CStyleDS

Minimalist header-only semi-generic C data structures with a macro-based API.

The library is aimed at small projects and coding-challenge style code where a single-header include and low setup cost matter more than full production-style abstraction.

## Included

- `CVec`: dynamic array / vector
- `CMat`: 2D dynamic array / matrix
- `CDeq`: circular-array deque, doubles as queue or stack
- `CPrioQ`: binary-heap priority queue
- `CHMap` / `CHSet`: open-addressing hash map / set
- `CHMCustomHash`: helpers for custom struct hashing/equality

## Upcoming

Planned in order of priority:
- `CTSet` / `CTMap`: binary search tree maps
- `CTrie`
- `CSTable`: sparse table / segment tree
- `CVStr`: dynamic string
- `CLList`: linked list

## Requirements

- C23 
- CMake 3.20+ recommended for building the example and benchmark targets

This project relies on `typeof` support through the local [_ctypeof.h](_ctypeof.h) shim and is configured for C23 in [CMakeLists.txt](CMakeLists.txt).

## Build

Build the example program:

```bash
cmake -S . -B build
cmake --build build
```

This produces the example executable from [main.c](main.c).

The repository also includes a separate benchmark target comparing the C containers against STL baselines in [bench/CMakeLists.txt](bench/CMakeLists.txt).

## Quick Start

Include the header for the container you want to use:

```c
#include "CVec.h"
#include "CDeq.h"
#include "CHMap.h"
```

The API is macro-heavy by design. 
For a list of exposed methods, refer to the short macros at the end of each file.
The usual calling pattern is:

- init: `(container, Type, capacity)`
- push (existing value): `(container, value)`
- emplace (literal / compound value): macro variant with forwarded arguments
- access: `(container, Type, position)`


## Usage Examples

### Vector

```c
#include "CVec.h"

int main(void) {
	CVec numbers;
	CVec_init(&numbers, int, 4);

	int x = 10;
	CVec_push(&numbers, x);
	CVec_emplace(&numbers, 20);
	CVec_emplace(&numbers, 30);

	for (size_t i = 0; i < numbers.size; i++) {
		printf("%d\n", CVec_at(&numbers, int, i));
	}

	CVec_free(&numbers);
	return 0;
}
```

### Deque as Queue

```c
#include "CDeq.h"

int main(void) {
	CDeq q;
	CDeq_init(&q, int, 2);

	CDeq_emplaceBack(&q, 1);
	CDeq_emplaceBack(&q, 2);
	CDeq_emplaceBack(&q, 3);

	while (q.size) {
		printf("%d\n", CDeq_popFront(&q, int));
	}

	CDeq_free(&q);
	return 0;
}
```

### Hash Map

```c
#include "CHMap.h"

int main(void) {
	CHMap counts;
	CHMap_initPrim(&counts, int, const char*, 8);

	CHMap_put(&counts, 1, "one");
	CHMap_put(&counts, 2, "two");
	CHMap_emplace(&counts, int, 3, const char*, "three");

	if (CHMap_has(&counts, 2)) {
		printf("%s\n", CHMap_get(&counts, const char*, 2));
	}

	CHMap_remove(&counts, 1);
	CHMap_free(&counts);
	return 0;
}
```

### Priority Queue

```c
#include "CPrioQ.h"

int main(void) {
	CPrioQ pq;
	CPrioQ_initPrim(&pq, int, 0, MAX);

	CPrioQ_emplace(&pq, 4);
	CPrioQ_emplace(&pq, 9);
	CPrioQ_emplace(&pq, 2);

	while (pq.size) {
		printf("%d\n", CPrioQ_pop(&pq, int));
	}

	CPrioQ_free(&pq);
	return 0;
}
```

## Design Notes

- Header-only implementation for simple drop-in use
- Generic behavior is provided through macros and `sizeof(T)`-based storage
- Containers store values inline (currently all complete ones are void*-array based)

## Caveats

- This is not a fully type-safe API; misuse of macros or wrong access types is on the caller
- Returned references and pointers can be invalidated by container growth or mutation
- Primitive hash-map setup is intended for primitive keys; struct keys should use custom hash/equality helpers
- Error handling is intentionally minimal and currently does not provide a full failure-reporting layer

## Repository Layout

- [main.c](main.c): example/demo usage
- [bench](bench): benchmark target against C++ STL containers
- [CVec.h](CVec.h), [CDeq.h](CDeq.h), [CPrioQ.h](CPrioQ.h), [CHMap.h](CHMap.h): primary container headers

## Notes

The examples in this README are intentionally small. For a broader usage sample, see [main.c](main.c).
AI usage disclosure: documentation and tests mainly AI generated. For source files, AI used for inline debugging only.
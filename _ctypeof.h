#pragma once
// cpptools/EDG IntelliSense (in its MSVC C23 emulation) doesn't recognize the
// GNU `__typeof__` spelling, even though real cl.exe and every gcc/clang mode
// accept it. Under __INTELLISENSE__ only, alias it to the standard C23 keyword
// `typeof` so the container macros stop showing false "expected a ';'" errors.
// __INTELLISENSE__ is defined solely by cpptools, so no real build is affected.
#ifdef __INTELLISENSE__
  #define __typeof__ typeof
#endif

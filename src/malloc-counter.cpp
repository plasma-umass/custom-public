// clang++ -compatibility_version 1 -current_version 1 -dynamiclib malloc-counter.cpp printf.cpp -o malloc-counter.dylib


// -*- C++ -*-

#ifndef HL_MACINTERPOSE_H
#define HL_MACINTERPOSE_H

// The interposition data structure (just pairs of function pointers),
// used an interposition table like the following:
//

typedef struct interpose_s {
  void *new_func;
  void *orig_func;
} interpose_t;

#define MAC_INTERPOSE(newf,oldf) __attribute__((used)) \
  static const interpose_t macinterpose##newf##oldf \
  __attribute__ ((section("__DATA, __interpose"))) = \
    { (void *) newf, (void *) oldf }

#endif

#include <stdlib.h>
#include <malloc/malloc.h>
#include <stdio.h>
#include <unistd.h>

#include "printf.h"

// For use by the replacement printf routines (see
// https://github.com/mpaland/printf)
extern "C" void _putchar(char ch) { ::write(1, (void *)&ch, 1); }

static int mallocs = 0;
static int frees = 0;

extern "C" void * replace_malloc(size_t sz) {
  auto ptr = ::malloc(sz);
  ++mallocs;
  printf_("malloc(%lu): %d\n", sz, mallocs);
  return ptr;
}

extern "C" void replace_free(void * ptr) {
  ++frees;
  printf_("frees: %d\n", frees);
  ::free(ptr);
}

MAC_INTERPOSE(replace_malloc, malloc);
MAC_INTERPOSE(replace_free, free);


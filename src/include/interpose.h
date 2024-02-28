#ifndef INTERPOSE_H
#define INTERPOSE_H

#ifdef __APPLE__
#define DYLD_INTERPOSE(_replacment, _replacee)                                                                         \
    __attribute__((used)) static struct {                                                                              \
        const void* replacment;                                                                                        \
        const void* replacee;                                                                                          \
    } _interpose_##_replacee __attribute__((section("__DATA,__interpose")))                                            \
    = {(const void*) (unsigned long) &_replacment, (const void*) (unsigned long) &_replacee}

#define GET_REAL_FUNCTION(NAME) ::NAME
#define INTERPOSE_FUNCTION_NAME(NAME) interpose_##NAME##__
#define INTERPOSE(NAME) DYLD_INTERPOSE(INTERPOSE_FUNCTION_NAME(NAME), NAME)
#else
#include <dlfcn.h>
#define GET_REAL_FUNCTION(NAME) (reinterpret_cast<decltype(::NAME)*>(dlsym(RTLD_NEXT, #NAME)))
#define INTERPOSE_FUNCTION_NAME(NAME) NAME
#define INTERPOSE(NAME)
#endif

#endif // INTERPOSE_H

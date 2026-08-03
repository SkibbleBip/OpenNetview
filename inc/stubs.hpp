#ifndef __STUBS_HPP__
#define __STUBS_HPP__

#include <cstddef>

extern "C" {
// Satisfies GCC 12's C++ header checks on old uClibc runtimes.
// Setting it to 1 tells libstdc++ that single-threaded optimizations are safe.
char __attribute__((weak)) __libc_single_threaded = 1;
}

void operator delete(void *p, std::size_t) { ::operator delete(p); }

void operator delete[](void *p, std::size_t) { ::operator delete[](p); }

#endif // __STUBS_HPP__

// cxxrt.cpp -- what g++ -m32 -fno-exceptions -fno-rtti code needs from a C++
// runtime, since helios has no 32-bit libstdc++: new and delete on malloc,
// the pure-virtual trap, and function-local statics' guards (one thread).
// The same set libc4cxx gives c4cxx's output (plan section 3.7).
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

static void *alloc (size_t n)
{
	void *p = malloc (n ? n : 1);
	if (!p) {
		fprintf (stderr, "cxxrt: out of memory (%lu bytes)\n", (unsigned long)n);
		abort ();
	}
	return p;
}

void *operator new (size_t n) { return alloc (n); }
void *operator new[] (size_t n) { return alloc (n); }
void operator delete (void *p) noexcept { free (p); }
void operator delete[] (void *p) noexcept { free (p); }
void operator delete (void *p, size_t) noexcept { free (p); }
void operator delete[] (void *p, size_t) noexcept { free (p); }

extern "C" {
void __cxa_pure_virtual (void)
{
	fprintf (stderr, "cxxrt: pure virtual function called\n");
	abort ();
}
int __cxa_guard_acquire (long long *g) { return !*(char *)g; }
void __cxa_guard_release (long long *g) { *(char *)g = 1; }
void __cxa_guard_abort (long long *g) { (void)g; }
}

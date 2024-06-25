#include <stdint.h>

extern "C" {

void seed(uint64_t v);
uint64_t xoshiro256plus(void);
uint64_t splitmix64();

}

// from yarandom.h
#define LRAND()         ((long) (xoshiro256plus() & 0x7fffffff))
#define MAXRAND         (2147483648.0) /* unsigned 1<<31 as a float */
#define balance_rand(v)	((LRAND()/MAXRAND*(v))-((v)/2))	/* random around 0 */

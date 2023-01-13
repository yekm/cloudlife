#include <stdint.h>

extern "C" {

void seed(uint64_t v);
uint64_t xoshiro256plus(void);
uint64_t splitmix64();

}

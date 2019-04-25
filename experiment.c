/* This code is an experiment in C++ */
// The aim is to create a class or structure, that behaves as though it had
// primitive types (POD structure), but such that reading or writing to
// these fields actually calls a method. This would be for testing C code

#include <stdint.h>
#ifdef TEST
#include "regtest.h"
#endif

#ifndef TEST            // As a compromise to help testing, we use Reg32 as
#define Reg32 uint32_t  // substitute for uint32_t in the type definition
#endif                  // below (and only in the definition).
typedef struct {
    Reg32 cr;
    Reg32 dr;
    Reg32 isr;
    Reg32 icr;
} Device;
#ifndef TEST
#undef Reg32
#endif


static volatile Device* dev = (volatile Device*) 0x20000800;

// This below is only needed as I chose to intermingle test code and normal
// code. Usually, this would not be done, and the only uses of expect_*
// would be in test code, at the end of the file, protected by an ifdef
// directive.
#ifndef TEST
static void expect_write(volatile void *a, uint32_t v) {(void) a; (void) v;}
static void expect_read(volatile void *a, uint32_t v) {(void) a; (void) v;}
static void expect_rest() {}
#endif

int main() {
    expect_write(&dev->cr, 0x01);
    expect_read(&dev->isr, 0x00);
    expect_read(&dev->isr, 0x00);
    expect_read(&dev->isr, 0x01);
    dev->cr = 0x01;
    while (dev->isr == 0) {};
    expect_rest();
    expect_write(&dev->dr, 0x20);
    expect_read(&dev->isr, 0x00);
    expect_read(&dev->isr, 0x00);
    expect_read(&dev->isr, 0x01);
    dev->dr = 0x20;
    while (dev->isr == 0) {};
    expect_rest();
    expect_write(&dev->cr, 0x00);
    dev->cr = 0;
    expect_rest();
    return 0;
}


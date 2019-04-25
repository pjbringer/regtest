/* This code is an experiment in C++ */
// The aim is to create a class or structure, that behaves as though it had
// primitive types (POD structure), but such that reading or writing to
// these fields actually calls a method.

#include <cstdint>
#include <cstdio>

#include <sys/mman.h>
#include <sys/signal.h>

using namespace std;

// We aim to emulate this
/*
typedef struct {
    uint32_t cr;
    uint32_t dr;
    uint32_t isr;
    uint32_t icr;
} Device;

static volatile Device* dev = static_cast<volatile Device*>((void*)0x40000800);
*/

// ############################################################################
// First step: let's try a single register;
//static volatile uint32_t* reg = (uint32_t*) 0x20000008;
class Reg32 {
  public:
    Reg32() = delete;
    Reg32& operator=(const Reg32& rhs) = delete;
    uint32_t operator=(uint32_t v) volatile;
    operator uint32_t() volatile;
  private:
    uint32_t v;
};

static volatile Reg32* reg = static_cast<volatile Reg32*>((void*)0x20000008);

// uint32_t Reg32::operator=(uint32_t v) volatile {
//     printf("Writing register at %p\n", (void*)&this->v);
//     return this->v = v;
// }
// 
// Reg32::operator uint32_t() volatile {
//     printf("Reading register at %p\n", (void*)&this->v);
//     return this->v;
// }

// First step's result: we can declare a register at an arbitary address
// and catch reads and writes to the "register". The register can be typed
// volatile, provided we add the required keyword to the function types, and
// that we give up on returning "*this" from the assignement operator.
// ############################################################################
//  Second step: let's try to incorporate this register in a struct.
struct Device {
    Reg32 cr;
    Reg32 dr;
    Reg32 isr;
    Reg32 icr;
};


static volatile Device* dev = static_cast<volatile Device*>((void*)0x20000800);
// Second step's results: We can declare a structure at an arbitrary address
// with the volatile keyword if needed, and intercept reads and writes to the
// registers.
// ############################################################################
// Third step (bonus): Let's try to add a queue of expected actions on the
// registers

#include <queue>

struct ROp {
    ROp(const volatile uint32_t *adr, uint32_t val, bool write) : adr(adr), val(val), write(write) {}
    const volatile uint32_t *adr;
    uint32_t val;
    bool   write;
};
std::queue<ROp> ropq;

void expect_read(const volatile Reg32 &reg, uint32_t val) {
    ropq.emplace(reinterpret_cast<const volatile uint32_t*>(&reg), val, false);
}

void expect_write(const volatile Reg32 &reg, uint32_t val) {
    ropq.emplace(reinterpret_cast<const volatile uint32_t*>(&reg), val, true);
}

void expect_rest() {
    if (!ropq.empty()) {
        printf("\nExpected register operation(s) did not occur.\n");
        raise(SIGINT);
    }
}

uint32_t Reg32::operator=(uint32_t v) volatile {
    if (ropq.empty() ||
        ropq.front().write == false ||
        ropq.front().adr != &this->v ) {
        printf("\nUnexpected write of %08x to address %p\n", v, static_cast<volatile void*>(&this->v));
        raise(SIGINT);
    } else if(ropq.front().val != v) {
        printf("\nUnexpected value %08x of write to address %p\n", v, static_cast<volatile void*>(&this->v));
        raise(SIGINT);
    } else {
        ropq.pop();
    }

    // printf("Writing register at %p\n", (void*)&this->v);
    return this->v = v;
}

Reg32::operator uint32_t() volatile {
    uint32_t ret = (uint32_t) -1;
    if (ropq.empty() ||
        ropq.front().write == true ||
        ropq.front().adr != &this->v) {
        printf("\nUnexpected read at address %p\n", static_cast<volatile void*>(&this->v));
        raise(SIGINT);
    } else { 
        ret = ropq.front().val;
        ropq.pop();
    }
    //printf("Reading register at %p\n", (void*)&this->v);
    return ret;
}


// Third step results: It is feasible to add a queue of operations and check
// that all expected register level operations do occur. Volatile is a problem.
// ############################################################################
// Fourth step: Try to avoid declaring Reg32 to avoid typing mess

int main() {
    void *retmap = mmap((void*)0x20000000, 0x1000,
                        PROT_READ|PROT_WRITE, 
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,
                        -1, 0);
    if (retmap == MAP_FAILED) {
        perror(NULL);
        return -1;
    }
    // *reg = 0x10;
    expect_write(dev->cr, 0x01);
    expect_read(dev->isr, 0x00);
    expect_read(dev->isr, 0x00);
    expect_read(dev->isr, 0x01);
    dev->cr = 0x01;
    while (dev->isr == 0) {};
    expect_rest();
    expect_write(dev->dr, 0x20);
    expect_read(dev->isr, 0x00);
    expect_read(dev->isr, 0x00);
    expect_read(dev->isr, 0x01);
    dev->dr = 0x20;
    while (dev->isr == 0) {};
    expect_rest();
    expect_write(dev->cr, 0x00);
    dev->cr = 0;
    expect_rest();
    return 0;
}


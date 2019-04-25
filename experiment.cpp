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

// First step's result: we can declare a register at an arbitary address
// and catch reads and writes to the "register". The register can be typed
// volatile, provided we add the required keyword to the function types, and
// that we give up on returning "*this" from the assignement operator.
// ############################################################################
// Second step: let's try to incorporate this register in a struct.

// Second step's results: We can declare a structure at an arbitrary address
// with the volatile keyword if needed, and intercept reads and writes to the
// registers.
// ############################################################################
// Third step (bonus): Let's try to add a queue of expected actions on the
// registers

// Third step results: It is feasible to add a queue of operations and check
// that all expected register level operations do occur. Volatile is a problem.
// ############################################################################
// Fourth step: Try to avoid declaring Reg32 to avoid typing mess

#include <queue>

struct ROp {
    ROp(const volatile uint32_t *adr, uint32_t val, bool write) : adr(adr), val(val), write(write) {}
    const volatile uint32_t *adr;
    uint32_t val;
    bool   write;
};
std::queue<ROp> ropq;

void expect_read(const volatile uint32_t* adr, uint32_t val) {
    ropq.emplace(adr, val, false);
}

void expect_write(const volatile uint32_t* adr, uint32_t val) {
    ropq.emplace(adr, val, true);
}

void expect_rest() {
    if (!ropq.empty()) {
        printf("\nExpected register operation(s) did not occur.\n");
        raise(SIGINT);
    }
}


class Reg32 {
  public:
    Reg32() = delete;
    Reg32& operator=(const Reg32& rhs) = delete;
    uint32_t operator=(uint32_t v) volatile;
    const volatile uint32_t* operator&() const volatile;
    operator uint32_t() volatile;
  private:
    uint32_t v;
};

struct Device {
    Reg32 cr;
    Reg32 dr;
    Reg32 isr;
    Reg32 icr;
};

/*
class DeviceVPtr {
    volatile Device* dev;
  public:
    DeviceVPtr(void* adr) { dev = static_cast<volatile Device*>(const_cast<volatile void*>(adr));}
    operator volatile Device*() { return dev;}
};
*/

static volatile Device* dev = (volatile Device*) 0x20000800;

uint32_t Reg32::operator=(uint32_t v) volatile {
    if (ropq.empty() ||
        ropq.front().write == false ||
        ropq.front().adr != &this->v ) {
        printf("\nUnexpected write of 0x%08x to address %p\n", v, (volatile void*)&this->v);
        raise(SIGINT);
    } else if(ropq.front().val != v) {
        printf("\nUnexpected value 0x%08x of write to address %p\n", v, (volatile void*)&this->v);
        raise(SIGINT);
    } else {
        ropq.pop();
    }

    return this->v = v;
}

const volatile uint32_t* Reg32::operator&() const volatile {
    return &this->v;
}

Reg32::operator uint32_t() volatile {
    uint32_t ret = (uint32_t) -1;
    if (ropq.empty() ||
        ropq.front().write == true ||
        ropq.front().adr != &this->v) {
        printf("\nUnexpected read at address %p\n", (volatile void*)&this->v);
        raise(SIGINT);
    } else { 
        ret = ropq.front().val;
        ropq.pop();
    }
    return ret;
}


int main() {
    void *retmap = mmap((void*)0x20000000, 0x1000,
                        PROT_READ|PROT_WRITE, 
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,
                        -1, 0);
    if (retmap == MAP_FAILED) {
        perror(NULL);
        return -1;
    }
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

// Fourth step result: No overloading can be done on a pointer to any type,
// and the dot (.) operator cannot be overloaded either. Thus, it is necessary
// to change the peripheral type definition so that uint32_t types are replaced
// by Reg32.
// Similarly, there is no way around using a static_cast in the definition
// of the global variables for the peripherals, as they are plain pointers.
// A class such as DeviceVPtr can be used, but still requires an explicit
// cast/ I have found no better for now. If however the C code were to not
// use // void* but the actually pointer type instead, there would be no
// problem.
// Reg32 can be further adjusted so that in addition to replacing assignements
// and to auto-converting to uint32_t, getting the address with the ampersand
// (&) operator yields the address of the register itself. This makes the Reg32
// feel even more like the wrapped primitive uint32_t.

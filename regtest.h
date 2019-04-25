/* Regtest.hpp - Peripheral access test library */

/*
    Documentation
    =============

    This library is a C++ header-only library meant to help test C code making
direct use of a peripheral. In C a structure corresponding to the registers of
a peripheral on the bus may be defined as follows:

typedef struct {
    uint32_t cr;
    uint32_t isr;
    uint32_t dr;
} Peripheral;

volatile Peripheral *peripheral = (Peripheral*) peripheral;


    Then the peripheral can be used directly:

void periph_start() {
    peripheral->cr = 0x1; // Start device
    while (peripheral->isr == 0) {}; // Loop until device actually started
    peripheral->dr = 0x20 ; // Send one byte using the peripheral
}


    To test the accesses to such a peripheral this library defines a queue
of memory operations, where each operation is defined by its address, whether
it is a read or a write operation, and the value, either expected from a write
or yielded to a read. The test script thus defines the sequence of expected
memory operations to the peripheral, and intercepts the operations that are
made by the tested code.


    To do this this library requires that the structure definition use the C++
type Reg32 instead of uint32_t. This may be done as follows:

#ifndef TEST            // As a compromise to help testing, we use Reg32 as
#define Reg32 uint32_t  // substitute for uint32_t in the type definition
#endif                  // below (and only in the definition).
typedef struct {
    Reg32 cr;
    Reg32 isr;
    Reg32 dr;
} Peripheral;
#ifndef TEST
#undef Reg32
#endif

    Then the test script uses the expect_write(), expect_read(), and
expect_rest() functions to specify the expected accesses.

void test_periph_start() {
    expect_write(&periph->cr, 0x01);
    expect_read(&periph->isr, 0x00);
    expect_read(&periph->isr, 0x00);
    expect_read(&periph->isr, 0x00);
    expect_read(&periph->isr, 0x01);
    expect_write(&periph->dr, 0x20);
    expect_rest(); // No expected operation remains
}

The "expect_*" Procedures
-------------------------

    void expect_read(const volatile uint32_t*, uint32_t);
    void expect_write(const volatile uint32_t*, uint32_t);
    void expect_rest();

    Both expect_read and expect_write add one expected memory operation to the
queue "ropq". The "expect_rest()" function fails by raising a signal if there
remains an expected operation in the queue.

The "Reg32" Type
----------------

    This type wraps a single uin32_t value, and has the same address as the
underlying memory. Assigning to this type is overloaded to check that the
provided value is the one in the head of the queue (and discards this head).
Converting this value to uint32_t is overloaded to look in the queue instead
of the underlying register. Finally, getting the address of this value returns
the address of the underlying register.

*/


using namespace std;

#include <cstdint>
#include <cstdio>
#include <queue>

#include <sys/signal.h>


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

    return v;
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


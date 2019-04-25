/* Regtest.h - Peripheral access test library */

/*
    Documentation
    =============

    The regtest library is a C++ header-only library meant to help test C code
making direct use of a peripheral by making mock registers. In C, a structure
corresponding to the registers of a peripheral on the bus may be defined as
follows:

typedef struct {
    uint32_t cr;
    uint32_t isr;
    uint32_t dr;
} Peripheral;

volatile Peripheral *peripheral = (Peripheral*) peripheral;


    Then the peripheral can be used directly (the example below is tailored
for clarity rather than robustness or good engineering):

void periph_start() {
    peripheral->cr = CR_START; // Start device
    while (peripheral->isr != ...) {}; // Loop until device actually started
    peripheral->dr = ... ; // Send one byte using the peripheral
}


    To test the accesses to such a peripheral this library defines a queue
of memory operations, where each operation is defined by its address, whether
it is a read or a write operation, and the value, either expected from a write
or provided to a read. The test script thus defines the sequence of expected
memory operations to the peripheral, and intercepts the operations that are
made by the tested code.


    To do this, this library requires that the structure definition use the C++
type Reg32 provided by this library instead of uint32_t. This may be done as
follows:

#ifndef TEST            // As a compromise to help testing, we use Reg32 as
#define Reg32 uint32_t  // substitute for uint32_t in the type definition
#endif                  // (and only in the definition).
typedef struct {
    Reg32 cr;   // Control register
    Reg32 isr;  // Interrupt status register
    Reg32 dr;   // Data register
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
    int expect_rest();

    Both expect_read and expect_write add one expected memory operation to the
queue "ropq". The "expect_rest()" function fails by raising a signal if there
remains an expected operation in the queue; it's return code is 0 if and only
is succeeds.


The "Reg32" Type
----------------

    This type wraps a single uin32_t value, and has the same address as its
underlying memory. Assigning to this type is overloaded to check that the
provided value is the one in the head of the queue (and discards this head).
Converting this value to uint32_t is overloaded to look in the queue instead
of the underlying register. Finally, getting the address of this value returns
the address of the underlying register.


Output
------

    This library is meant to be used in unit testing. It writes to standard
output when an error is detected, and uses signals to grab the attention of
a debugger:

    raise(SIGINT)


Security Considerations
-----------------------

    This library is meant to help test cooperative code, it doesn't provide
security. It is trivial to work around it:

    *&periph->dr = ... ; // Pirate write! Sneaky!

*/

#ifndef __cplusplus
#warning Use a C++ compiler
#endif

using namespace std;

#include <cstdint>
#include <cstdio>
#include <queue>

#include <sys/signal.h>


/* This structure stores one expected register operation: either one write of
 * a given value to a given address, or a read at a given address that is
 * should yield a certain value in the context of the test script. */
struct ROp {
    ROp(const volatile uint32_t *adr, uint32_t val, bool write) : adr(adr),
                                                     val(val), write(write) {}
    const volatile uint32_t *adr;
    uint32_t val;
    bool   write;
};

/* We queue the expected operation in this variable. Expect_read/write enqueues
 * here, the Reg32 operations dequeue from here. */
std::queue<ROp> ropq;


void expect_read(const volatile uint32_t* adr, uint32_t val) {
    ropq.emplace(adr, val, false);
}

void expect_write(const volatile uint32_t* adr, uint32_t val) {
    ropq.emplace(adr, val, true);
}

int expect_rest() {
    if (!ropq.empty()) {
        printf("\nExpected register operation(s) did not occur.\n");
        raise(SIGINT);
        return 1;
    }
    return 0;
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

/* Assignement operator overload: intercept writes */
uint32_t Reg32::operator=(uint32_t v) volatile {
    if (ropq.empty() ||
        ropq.front().write == false ||
        ropq.front().adr != &this->v ) {
        printf("\nUnexpected write of 0x%08x to address %p\n",
                                 v, (volatile void*)&this->v);
        raise(SIGINT);
    } else if(ropq.front().val != v) {
        printf("\nUnexpected value 0x%08x of write to address %p\n",
                                       v, (volatile void*)&this->v);
        raise(SIGINT);
    } else {
        ropq.pop();
    }

    return v;
}

/* Ampersand operator overload: provide the underlying register address */
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


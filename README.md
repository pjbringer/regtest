# Documentation

    The regtest library is a C++ header-only library meant to help test C code
making direct use of a peripheral by making mock registers. In C, a structure
corresponding to the registers of a peripheral on the bus may be defined as
follows:

```C
typedef struct {
    uint32_t cr;
    uint32_t isr;
    uint32_t dr;
} Peripheral;

volatile Peripheral *peripheral = (Peripheral*) peripheral;
```


    Then the peripheral can be used directly (the example below is tailored
for clarity rather than robustness or good engineering):

```C
void periph_start() {
    peripheral->cr = CR_START; // Start device
    while (peripheral->isr != ...) {}; // Loop until device actually started
    peripheral->dr = ... ; // Send one byte using the peripheral
}
```


    To test the accesses to such a peripheral this library defines a queue
of memory operations, where each operation is defined by its address, whether
it is a read or a write operation, and the value, either expected from a write
or provided to a read. The test script thus defines the sequence of expected
memory operations to the peripheral, and intercepts the operations that are
made by the tested code.


    To do this, this library requires that the structure definition use the C++
type Reg32 provided by this library instead of `uint32_t`. This may be done as
follows:

```C
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
```


    Then the test script uses the `expect_write()`, `expect_read()`, and
`expect_rest()` functions to specify the expected accesses.

```C++
void test_periph_start() {
    expect_write(&periph->cr, 0x01);
    expect_read(&periph->isr, 0x00);
    expect_read(&periph->isr, 0x00);
    expect_read(&periph->isr, 0x00);
    expect_read(&periph->isr, 0x01);
    expect_write(&periph->dr, 0x20);
    expect_rest(); // No expected operation remains
}
```


## The "expect_*" Procedures

```C++
void expect_read(const volatile uint32_t*, uint32_t);
void expect_write(const volatile uint32_t*, uint32_t);
void expect_rest();
```

    Both expect_read and expect_write add one expected memory operation to the
queue "ropq". The "expect_rest()" function fails by raising a signal if there
remains an expected operation in the queue.


## The "Reg32" Type

    This type wraps a single uin32_t value, and has the same address as its
underlying memory. Assigning to this type is overloaded to check that the
provided value is the one in the head of the queue (and discards this head).
Converting this value to `uint32_t` is overloaded to look in the queue instead
of the underlying register. Finally, getting the address of this value returns
the address of the underlying register.


## Output

    This library is meant to be used in unit testing. It writes to standard
output when an error is detected, and uses signals to grab the attention of
a debugger:

```C++
raise(SIGINT)
```


## Security Considerations

    This library is meant to help test cooperative code, it doesn't provide
security. It is trivial to work around it:

```C
*&periph->dr = ... ; // Pirate write! Sneaky!
```

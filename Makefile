CXXFLAGS=-Wall -Wextra -pedantic -Og -g -Wunused -std=c++11 -I.
CFLAGS=-c -Wall -Wextra -pedantic -Og -g -Wunused -std=c11

all: examples/test_example examples/example.o

test: examples/test_example
	./examples/test_example

clean:
	$(RM) examples/example.o examples/test_example

examples/example.o: examples/example.c regtest.h
examples/test_example: examples/example.c regtest.h
	$(CXX) $(CXXFLAGS) -DTEST -o $@ $<


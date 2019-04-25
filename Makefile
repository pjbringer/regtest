CXXFLAGS=-Wall -Wextra -pedantic -Og -g3 -Wunused -std=c++11
CFLAGS=-c -Wall -Wextra -pedantic -Og -g3 -Wunused -std=c11

all: experiment.o test_experiment
test: test_experiment
	./test_experiment

clean:
	$(RM) experiment experiment.o test_experiment

experiment.o: experiment.c regtest.h
test_experiment: experiment.c regtest.h
	$(CXX) $(CXXFLAGS) -DTEST -o $@ $<


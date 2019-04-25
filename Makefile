CXXFLAGS=-Wall -Wextra -pedantic -Og -g3 -Wunused -std=c++11

make: experiment
	./experiment

clean:
	$(RM) experiment

experiment: experiment.cpp


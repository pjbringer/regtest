CXXFLAGS=-Wall -Wextra -pedantic -Og -g3 -Wunused

make: experiment
	./experiment

clean:
	$(RM) experiment

experiment: experiment.cpp


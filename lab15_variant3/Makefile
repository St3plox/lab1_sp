CXX = g++
PYTHON ?= python3
# -O2 оптимизирует код; -pthread включает поддержку потоков при сборке.
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra -Wpedantic -pthread
THREADS ?= 4
REPEATS ?= 7
DISTANCE ?= 50

.PHONY: all check benchmark plot analysis clean
all: points

points: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o points

check: points
	$(PYTHON) check.py

benchmark: points
	mkdir -p results
	./points --benchmark $(THREADS) $(REPEATS) $(DISTANCE)

plot:
	$(PYTHON) plot.py

# Сначала замеры разных размеров, затем графики и вывод по этим замерам.
analysis: benchmark
	$(MAKE) plot

clean:
	rm -f points

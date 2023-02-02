

all:
	g++ -g icache_sim.cpp -o icache_sim

test:
	./icache_sim $(shell pwd)/test.txt $(shell pwd)/ext.txt $(shell pwd)/multi.txt

clean:
	rm icache_sim
	rm ext.txt
	rm multi.txt

.PHONY: all test clean
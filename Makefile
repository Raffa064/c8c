SOURCES = $(wildcard ./src/*.c)

all: c8c test/rom64.ch8 test/multiply.ch8

c8c: $(SOURCES)
	cc -O3 $^ -o $@

%.ch8: %.asm c8c
	./c8c ./$< -o $@

clean:
	rm c8c ./test/*.ch8

.PHONY: clean

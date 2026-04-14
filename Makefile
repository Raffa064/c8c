all: c8c

c8c: main.c
	cc -O3 $^ -o $@

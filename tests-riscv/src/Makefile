CC=/opt/riscv/bin/riscv64-unknown-elf-gcc
CXX=/opt/riscv/bin/riscv64-unknown-elf-g++
# CC=/opt/riscv/bin/riscv64-unknown-linux-gnu-gcc
# CC=/opt/rv64ima/bin/riscv64-unknown-linux-gnu-gcc
# CXX=/opt/riscv/bin/riscv64-unknown-linux-gnu-g++
# CXX=/opt/rv64ima/bin/riscv64-unknown-linux-gnu-g++

CC2=gcc
CXX2=g++
CFLAGS=-static
CXXFLAGS=-static 

all: anagram test-printf test-fmath test-math test-llong test-lswlr anagram_orig test-printf_orig test-fmath_orig test-math_orig test-llong_orig test-lswlr_orig x

anagram: anagram.c
	$(CC) $(CFLAGS)  -o anagram anagram.c
	$(CC2) $(CFLAGS) -o anagram_orig anagram.c

test-printf: test-printf.c
	$(CC) $(CFLAGS) -o test-printf test-printf.c -lm
	$(CC2) $(CFLAGS) -o test-printf_orig test-printf.c -lm

test-fmath: test-fmath.c
	$(CC) $(CFLAGS)  -o test-fmath test-fmath.c -lm
	$(CC2) $(CFLAGS) -o test-fmath_orig test-fmath.c -lm

test-math: test-math.c
	$(CC) $(CFLAGS) -o test-math test-math.c -lm
	$(CC2) $(CFLAGS) -o test-math_orig test-math.c -lm

test-llong: test-llong.c
	$(CC) $(CFLAGS) -o test-llong test-llong.c
	$(CC2) $(CFLAGS) -o test-llong_orig test-llong.c

test-lswlr: test-lswlr.c
	$(CC) $(CFLAGS) -o test-lswlr test-lswlr.c
	$(CC) $(CFLAGS) -o test-lswlr_orig test-lswlr.c
x: x.cpp
	$(CXX) $(CXXFLAGS) -o x x.cpp
	$(CXX2) $(CXXFLAGS) -o x_orig x.cpp

test:	all
	./x_orig 1> x.txt
	../../sim-safe x 1> test-x.txt 2> sim-test-x.txt
	./test-printf_orig 1> printf.txt 
	../../sim-safe test-printf 1> test-printf.txt 2> sim-test-printf.txt
	./test-fmath_orig 1> fmath.txt 
	../../sim-safe test-fmath 1> test-fmath.txt 2> sim-test-fmath.txt
	./test-math_orig 1> math.txt 
	../../sim-safe test-math 1> test-math.txt 2> sim-test-math.txt
	./test-llong_orig 1> llong.txt
	../../sim-safe test-llong 1> test-llong.txt 2> sim-test-llong.txt
	./test-lswlr_orig 1> lswlr.txt
	../../sim-safe test-lswlr 1> test-lswlr.txt 2> sim-test-lswlr.txt
	./anagram_orig words < input.txt > anagram.txt
	../../sim-safe anagram words < input.txt 1> anagram.txt 2> sim-anagram.txt

distclean:
	make clean

clean:
	rm -f anagram test-printf test-fmath test-math test-llong test-lswlr test-as *.[oia] core *~
	rm -f *_orig x


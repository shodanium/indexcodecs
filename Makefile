test:
	gcc -O3 -lstdc++ -o test test.cpp

check:
	./test postings.bin

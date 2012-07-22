test : test.cpp
	gcc -O3 -lstdc++ -o test test.cpp

check : test
	./test postings.bin

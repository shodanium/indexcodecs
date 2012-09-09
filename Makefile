test : test.cpp
	g++ -O3 -lstdc++ -o test test.cpp group_huffman.cpp

check : test
	./test postings.bin

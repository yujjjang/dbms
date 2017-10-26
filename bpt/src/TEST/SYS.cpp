#include <stdlib.h>

int main(){
	system("gcc -g -c BPT.c");
	system("ar cr libBPT.a BPT.o");
	system("gcc -g -o test test.c -L. -lBPT");
	return 0;
}


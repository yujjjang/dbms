#include "BPT.h"
#include <stdio.h>
int main(){
	open_db("aaa.txt");
	header_page* HEAD = getHeader();

	printf("%ld %ld %ld\n",HEAD->free_page,HEAD->root_page,HEAD->number_of_pages);
	
	free(HEAD);
	close_db();
	return 0;
}


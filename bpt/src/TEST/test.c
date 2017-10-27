#include "BPT.h"
#include <stdio.h>
#include <string.h>
#include <memory.h>
int main(){
	char* pathname = "aaa.txt";
	open_db(pathname);
	char a[10]="abc";
	char* ans;
	for(int i=1; i < 50000; ++i){
		insert(i,a); 
	}
	for(int i = 100000; i>=50000;--i){
		insert(i,a);
	}
	/*
	for(int i = 1; i < 33; ++i){
		if (insert(i,a)==-1){
			printf("DUP\n");
		}
	}
	*/
	
	for(int i = 1; i <= 100000; ++i){
		ans = find(i);
		if(ans == NULL){
			printf("NO EXIST\n");
			return 0;
		} else {
			printf("%d %s\n",i,ans);
			free(ans);
		}
	}

	/*for (int i = 1; i <= 3 ; ++i){
		delete(i);
	}*/
	
	close_db();
	return 0;
}

#include "BPT.h"
#include <stdio.h>
#include <string.h>
#include <memory.h>
int main(){
	char* pathname = "aaa.txt";
	open_db(pathname);
	char a[10]="abc";
	/*for(int i=1; i< 50000; ++i){
		insert(i,a); 
	}
	for(int i = 100000; i>=5000;--i){
		insert(i,a);
	}*/
	insert(100001,a);
	char * ans;
	for(int i = 1; i <= 100001 ; ++i){
		ans = find(i);
		if(ans == NULL){
			printf("NO EXIST\n");
		} else {
			printf("%d %s\n",i,ans);
			free(ans);
		}
	}
	close_db();
	return 0;
}

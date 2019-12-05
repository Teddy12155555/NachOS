#include "syscall.h"
static int n[1000];
main(){
	int i, j;
	for(i = 0; i < 5; ++i)
		for(j = 0; j < 1000; ++j){
			// PrintInt(j);
			n[j] = j;
		}
		
	PrintInt(666666666);
	PrintInt(666666666);
	PrintInt(666666666);
}

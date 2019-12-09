#include "syscall.h"
static int n[1000];
main(){
	int i, j, k;

	for(i = 0; i < 1000; i += 100)
		n[i] = i;

	for(k = 0; k < 5; ++k)
	{
		for(i = 0; i < 1000; ++i)
		{
			n[i] = i + n[i- i%100];
		}
	}
	PrintInt(666666666);
	PrintInt(666666666);
	PrintInt(666666666);
}

#include "syscall.h"
int main(){
	int n=4;
	for (;n<6;n++){
		Sleep(n);
		PrintInt(1);
	}
	return 1;
}

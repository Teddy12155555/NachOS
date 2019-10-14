#include "syscall.h"
int main(){
	int n = 3;
	for (;n<5;n++){
		Sleep(n);
		PrintInt(2);
	}
	return 1;
}

#include "syscall.h"
int main(){
	int n = 1;
	for (;n<3;n++){
		Sleep(n);
		PrintInt(3);
	}
	return 1;
}

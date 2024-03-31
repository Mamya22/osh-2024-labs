#define _GUN_SOURCE
#define SYS_NUM 548
#define LENGTH_1 45
#define LENGTH_2 20
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>

int main()
{
	char buf_1[LENGTH_1], buf_2[LENGTH_2];
	
	int res_1 = syscall(SYS_NUM, buf_1, LENGTH_1);
	
	if(res_1 == 0){
		printf("test 1: the length is %d, the return value is %d, buffer can store the whole string.\n", LENGTH_1, res_1);
		printf("Stored string is %s\n",buf_1);
	}
	else if(res_1 == -1){
		printf("test1: the length is %d, the return value is %d, the length is too short\n", LENGTH_1, res_1); 
	}
	else{
		printf("Error\n");
	}
	
	int res_2 = syscall(SYS_NUM, buf_2, LENGTH_2);
	
	if(res_2 == 0){
		printf("test 2: the length is %d, the return value is %d, buffer can store the whole string.\n", LENGTH_2, res_2);
		printf("Stored string is %s\n",buf_1);
	}
	else if(res_2 == -1){
		printf("test2: the length is %d, the return value is %d, the length is too short\n", LENGTH_2, res_2); 
	}
	else{
		printf("Error\n");
	}
	
	while(1){}
}

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>


using namespace std;
 
void child1(void) {
	sleep(30);
	//...write your code here
}

void child2(void) {
	 
	//...write your code here
}

void child3(void) {
	//sleep(30);
	while(true){}
	//...write your code here
}

int main(void) {
	cout << "parent process: "<< getpid() << "\n";

	if ( fork() == 0 ) {
		cout << "child1: " << getpid() << "\n";
		child1();
		cout << "exiting child1\n";
		_exit(0);
	}
	
	if ( fork() == 0 ) {
		cout << "child2: " << getpid() << "\n";
		child2();
		cout << "exiting child2\n";
		_exit(0);
	}

	if ( fork() == 0 ) {
		cout << "child3: " << getpid() << "\n";
		child3();
		cout << "exiting child3\n";
		_exit(0);
	}

	int count=0;
	while(true){}
	while(count!=2){
		waitpid(-1,NULL,0);
		count++;
		cout << count << endl;
	}
	//pause();
	return 0;
}

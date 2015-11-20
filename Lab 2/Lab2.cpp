#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define SHMSZ 30

using namespace std;

struct node{
	string domain;
	pid_t pid;
	vector<char*> email;
};

node* find(vector<node*> &match,string domain){
	for(int i=0;i<match.size();i++){
		if(match[i]->domain==domain)return match[i];
	}
	return 0;
}

void sig_usr(int signo){
if(signo == SIGUSR1)
printf("Signal caught!");
return;
}


int main(){

	int testCases;
	cin>>testCases;
	void* shm;
	char* s;
	vector<node*>  match;
	pid_t mainPid=getpid(),pid;
	key_t key=9999;
	int shmid = shmget(key,SHMSZ,IPC_CREAT|0666);
	if(shmid<0){
		cout<<"ERROR in shmget"<<endl;exit(1);
	}

	
	if((shm = shmat(shmid,NULL,0)) == (void *) -1) {
		cout<<"ERROR in shmat"<<endl;exit(1);
	}
	
	s = (char*)shm;
	strcpy(s, "Hello");
    struct sigaction sig;               
    sigemptyset(&sig.sa_mask);          
    sig.sa_flags = 0;                   
    sig.sa_handler = sig_usr;           

	while(testCases--){
		string base;
		cin>>base;

		if(base=="add_email"){
			string s1;
			cin>>s1;
			const char* tempc=s1.c_str();
			strcpy(s, tempc);
			int temp=find(s1.begin(),s1.end(),'@')-s1.begin();
			string domain=s1.substr(temp+1);
			if(find(match,domain)){

				

			}

			else{
								
				node* temp=new node();
				temp->domain=domain;
				pid=fork();
				pid_t temp100;
				if(pid!=0){
					temp->pid=pid;
					match.push_back(temp);
				}

				if(pid==0){
					node* child=new node();
					child=temp;
					int shmid1 = shmget(key,SHMSZ,IPC_CREAT|0666);
					if(shmid1<0){
						cout<<"ERROR in shmget1"<<endl;exit(1);
					}			
					void* shm1;   
					
					if((shm1 = shmat(shmid1,NULL,0)) == (void *) -1) {
						cout<<"ERROR in shmat1"<<endl;exit(1);
					}

					char* s1=(char *)shm1;
					child->email.push_back(s1);
					strcpy(s1,"I AM BACK");
					kill(mainPid,SIGUSR1);
					cout<<s1<<endl;
					while(1){

					}
				}	
			
			}
		}



	}


}
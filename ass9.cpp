#include <bits/stdc++.h>
#include <cstdlib>
#include <pthread.h>
#include <thread>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <unistd.h>

using namespace std;
struct pageTable;

struct pcb{
	int processNo, processSize;
	pageTable* proc_pageTable;
	int seq, nextFill; //Should remove this
  	pcb(int a, int b){
  		processNo = a;
  		processSize = b;
  		proc_pageTable = NULL;
  	}
};

struct frame {
	bool valid;
    bool inFreeList;
    bool dirty;
	int seq;
	int index;
	int pid;
	int pageNo;
    frame(int ind, bool i, int a, bool b) {
		index = ind;
		valid = i;
        seq = a;
        inFreeList = b;
        dirty = false;
        pid=-1;
        pageNo=-1;
	}
	frame(){
		valid = false;
		dirty = false;
	}
};

struct pageTableEntry {
	int pageNum;
	int frameNum;
	bool valid;
	//int seq;
	pageTableEntry(){
		valid = false;
		pageNum = -1;
		frameNum = -1;
	}
	pageTableEntry(int i, int j, bool k){
		pageNum = i;
		frameNum = j;
		valid = k;
		//seq = l;
	}
};

struct pageTable {
	int size;
	vector<pageTableEntry> rows;
	pageTable(){}
	pageTable(int s) {
		size = s;
		std::vector<pageTableEntry> v(size);
		rows = v;
	}
    void print(){
		for(int i=1;i<rows.size();i++){
			cout<<rows[i].pageNum<<" "<<rows[i].frameNum<<" "<<rows[i].valid<<" "<<endl;
		}
	}
};

struct pageIOEntry{
	int pageNum;
	int pid;
	int frameNum;
	bool isPageIn; //True for pageIn and false for pageOut
	pageIOEntry(int a, int b, int c, bool d){
		pageNum = a;
		pid = b;
		frameNum = c;
		isPageIn = d;
	}
};


frame** ram;
map<int, pcb*> pcbMap;
int lower_threshold, upper_threshold;
vector<pcb*> allPcb;
int sequenceNext, frameArraySize;
queue<pageIOEntry> pageIOTable;
int threadCount=99999, threadEndTillNow = 0;
bool pageInDone;

// my part
list<frame*> freeFrames;
mutex mtx;
condition_variable cond;
mutex freeFramesLock;
mutex pcbMapLock;
mutex pageIOTableLock;
pthread_mutex_t sizeCheck_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sizeCheck_cond = PTHREAD_COND_INITIALIZER;

pthread_cond_t pageIn_cond[1000] = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pageIn_mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t freeFrame_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t freeFrame_cond = PTHREAD_COND_INITIALIZER;

// take care that frame is not dirty before calling
void insertIntoFreeList(frame* f) {
	//assert(!(f->dirty));
	f->dirty = false;
	f->valid = false;
	f->inFreeList = true;
	f->seq = 0;
	freeFrames.push_back(f);
}

int lru() {
	int seqMin;
	bool flag = true;
	int ret = -1;
	for(int i=0; i<frameArraySize; i++) {
		if(ram[i]->inFreeList) continue;
		
		/*if(i<=5) {
			cout << i << " " << ram[i]->dirty << " " << ram[i]->seq << endl;
			//printf("ram0 - %i \n", ram[0]->seq);
		}*/
		
		if(flag) {
			seqMin = ram[i]->seq;
			ret = i;
			flag = false;
		}
		else {
			if(seqMin > ram[i]->seq) {
				//cout << i << " " << ram[i]->seq << endl;
				seqMin = ram[i]->seq;
				ret = i;
			}
		}
	}
	//cout << "seqMin " << seqMin << endl;
	return ret;
}

void *pageIOManager(void *a) {
	while(threadEndTillNow!=threadCount){
		//printf("YOYOYO \n");
		while(!pageIOTable.empty()){
			int pageIOTableSize = pageIOTable.size();
			//printf("page io table size is %i \n" , pageIOTableSize);
			pageIOEntry temp = pageIOTable.front();
			pageIOTable.pop();
			if(temp.isPageIn){ //Comes from page fault handler, Just dummy IO
				usleep(5000);
				pthread_mutex_lock(&pageIn_mtx);
				pthread_cond_signal(&pageIn_cond[temp.pid]);	
				printf("IO MANAGER: Page-in: %i: Loaded page %i into frame %i \n",
				 temp.pid,
				 temp.pageNum,
				 temp.frameNum);
				pthread_mutex_unlock(&pageIn_mtx);
				//printf("Mutex Unlocked %i \n", temp.pid);
			}
			else{ //From free list manager
				usleep(5000);
				printf("IO MANAGER: Page-out: %i: Removed page %i from frame %i \n", 
					temp.pid,
					temp.pageNum,
					temp.frameNum);
				pthread_mutex_lock(&freeFrame_mtx);
				pthread_cond_signal(&freeFrame_cond);	
				pthread_mutex_unlock(&freeFrame_mtx);
			}
			//pageIOTableLock.unlock();
		}
	}
	pthread_exit(NULL);
}

void *freeFrameManager(void *NewParameter) {
	//std::unique_lock<std::mutex> lck(mtx);
	while(threadEndTillNow != threadCount) {
		//while(n>lower_threshold) {
		/*
		bool check = true;
		while(pred() && (threadEndTillNow != threadCount)) {
			//printf("Check out");
			if(check) {
				printf("FFM: Blocking itself \n");
				check = false;
			}
			//wait(lck);
		}
		*/
		if(freeFrames.size() >= upper_threshold && (threadEndTillNow != threadCount)) {
			printf("FFM: Blocking\n");
			pthread_mutex_lock(&sizeCheck_mtx);
			pthread_cond_wait(&sizeCheck_cond, &sizeCheck_mtx);
			pthread_mutex_unlock(&sizeCheck_mtx);
		}
		if(threadEndTillNow != threadCount) {
			printf("FFM: Activated \n");
			int changeFrameNo = lru();
			//assert(changeFrameNo != -1);
			frame* changeFrame = ram[changeFrameNo];
			if(changeFrame->dirty) {
				printf("FFM: instructed the page I/O manager to write out a dirty page \n");
				/*TODO: should remove entry from page table of corresp. process
				*/

                pageIOEntry temp(changeFrame->pageNo, changeFrame->pid, changeFrameNo, false);				
				pthread_mutex_lock(&freeFrame_mtx);
				pageIOTable.push(temp);
				pthread_cond_wait(&freeFrame_cond, &freeFrame_mtx);
				pthread_mutex_unlock(&freeFrame_mtx);
                
				int pageNum = changeFrame->pageNo;
				int pid = changeFrame->pid;
				pcbMapLock.lock();
				pcbMap[pid]->proc_pageTable->rows[pageNum].valid = false;
				pcbMap[pid]->proc_pageTable->rows[pageNum].frameNum = -1;
				pcbMapLock.unlock();

				changeFrame->dirty = false;
				insertIntoFreeList(changeFrame);
			}

			else {
				if(changeFrame->valid) {
					
					int pageNum = changeFrame->pageNo;
					int pid = changeFrame->pid;
					
					pcbMapLock.lock();
					pcbMap[pid]->proc_pageTable->rows[pageNum].valid = false;
					pcbMap[pid]->proc_pageTable->rows[pageNum].frameNum = -1;
					pcbMapLock.unlock();
				}
				insertIntoFreeList(changeFrame);
			}
			printf("FFM: Added page frame %i into free list\n", changeFrameNo);
			//printf("After - freeFrames - %i\n", freeFrames.size());
		}
		//printf("Check out");
		//cout << "Take action" << endl;
		//n++;
	}
	pthread_exit(NULL);
}

mutex mmuLock; //CHANGE
int mmu(int pageNo, int pid, bool isModify){
	mmuLock.lock();
	pcbMapLock.lock();
	int processSize = pcbMap[pid]->processSize;
	pcb* proc_pcb = pcbMap[pid];
	pageTable* proc_pageTable = pcbMap[pid]->proc_pageTable;

	if(pageNo<0 || pageNo>=processSize) {mmuLock.unlock(); pcbMapLock.unlock();return 1;}
	
	pageTableEntry ptEntry = proc_pageTable->rows[pageNo];
	int frameNum = ptEntry.frameNum;
	if(ptEntry.valid) {
		ram[frameNum]->seq = sequenceNext;
		sequenceNext++;
		if(isModify) {
			ram[frameNum]->dirty = true;
		}
		printf("Accessed page frame number %i\n", frameNum);
		mmuLock.unlock();
		pcbMapLock.unlock();
		return 2;
	}
	mmuLock.unlock();
	pcbMapLock.unlock();
	return 3;
}

int page_fault_handler(int pageNum, int pid) {//CHECK, Not using anylock for freeFrameList
	pcbMapLock.lock();

	pcb* proc_pcb = pcbMap[pid];
	pageTable* proc_pageTable = proc_pcb->proc_pageTable;
	
	//freeFramesLock.lock();
	/*if(freeFrames.size() < lower_threshold) {
	}	*/
	
	if(freeFrames.size() == 0){

	} //Waiting for it to fill up
	
	frame* nextFrame = freeFrames.front();
	freeFrames.pop_front();

	if(freeFrames.size() < lower_threshold) {
		pthread_mutex_lock(&sizeCheck_mtx);
		pthread_cond_signal(&sizeCheck_cond);
		pthread_mutex_unlock(&sizeCheck_mtx);		
	}

	//freeFramesLock.unlock();

    pageIOEntry temp(pageNum, pid, nextFrame->index, true);

	pthread_mutex_lock(&pageIn_mtx);
	pageIOTable.push(temp);
	pthread_cond_wait(&pageIn_cond[temp.pid], &pageIn_mtx);
	pthread_mutex_unlock(&pageIn_mtx);


	// set up the page in ram
	nextFrame->valid = true;
	nextFrame->inFreeList = false;
	nextFrame->seq = sequenceNext;
	nextFrame->pid=pid;
	nextFrame->pageNo=pageNum;
	sequenceNext++;

	// set up an entry in pid's pt
	pageTableEntry ptEntry(pageNum,nextFrame->index,true);
	proc_pageTable->rows[pageNum] = ptEntry;
	printf("Loaded %i into frame %i\n", pageNum, nextFrame->index);
	pcbMapLock.unlock();
}

void *threadFunction(void *InData){//CHANGE
	pcb* data;
	data = (pcb *) InData;
	int pid = data->processNo;
	int size = data->processSize;
 	data->proc_pageTable = new pageTable(size+1);

	string fileName = "s";
	stringstream ss;
	ss << pid;
	fileName += ss.str();

	ifstream ifs(fileName.c_str());
	string line;
	int accessNum = 0, modifyNum = 0, pfNum = 0;
	while(getline(ifs,line)){
		//cout<<"YOla"<<endl;
		string isEnd = line.substr(0,3);
		if(isEnd == "End") {
			//cout << "end" << "\n";
			printf("Number of access operations : %i \n"
				   "Number of modify operations : %i \n"
				   "Number of page faults : %i \n",
				    accessNum,
				    modifyNum,
				    pfNum); 
            threadEndTillNow++;
			break;
		}

		//cout<<"5"<<endl;	
		string arg1 = line.substr(0,6); //Same
		//cout<<"6"<<endl;
		line.erase(0,6);
		int findComma = find(line.begin(),line.end(),',') - line.begin();
		string arg2 = line.substr(0,findComma);
		line.erase(0,findComma+1);
		string arg3 = line;
		
		istringstream ss(arg2);
		int arg2Num;
		ss >> arg2Num;
		
		istringstream ss1(arg3);
		int arg3Num;
		ss1 >> arg3Num;

		//cout << arg1 << " " << arg2Num << " " << arg3Num << " " << "\n";
		
		printf("%i : attempted to %s %i %i\n", pid, arg1.c_str(), arg2Num, arg3Num);
		
		pcb* proc_pcb = pcbMap[pid];
		pageTable* proc_pageTable = pcbMap[pid]->proc_pageTable;		

		int status = mmu(arg2Num, pid, (arg1 == "Modify"));
		if(status == 3) {
			printf("Reported a page fault \n");
			pfNum++;
			page_fault_handler(arg2Num, pid);
			
			pageTableEntry ptEntry = proc_pageTable->rows[arg2Num];
			int frameNum = ptEntry.frameNum;
			if(arg1 == "Modify") {
				ram[frameNum]->dirty = true;
			}
		}
		else if(status == 1) {
			printf("Reported a memory protection violation\n"); 
		}
		if(status == 2 || status == 3) {
			if(arg1 == "Access") {
				accessNum++;
			}
			else if(arg1 == "Modify") {
				modifyNum++;
			}
		}
	}
	pthread_exit(NULL);
}

int main(){
	pthread_t frameThread, pageIOThread;
	pthread_t tempThread[1000]; 
	sequenceNext = 1;
    ifstream ifs("init");
    string line;
    int i=0, rc;
    int tempThreadCount = 0;
    bool flag = false;
    bool showPageTable = false;
    while(getline(ifs,line)){

    	string temp = line.substr(0,11);
    	if(temp=="Memory_size"){
    		string length = line.substr(12);
    		frameArraySize = stoi(length);
    		ram = new frame*[frameArraySize];
    		for(int i=0; i<frameArraySize; i++){
				frame* f = new frame(i, false, 0, true);
				ram[i] = f;
                freeFrames.push_back(f);
		    }
		    continue;
    	}

    	temp = line.substr(0,15);
    	if(temp == "Lower_threshold"){
    		string lowerSize = line.substr(16);
    		lower_threshold = stoi(lowerSize);
    		continue;
    	}

		if(temp == "Upper_threshold"){
    		string upperSize = line.substr(16);
    		upper_threshold = stoi(upperSize);
    		continue;
    	}

    	temp = line.substr(0,6);
    	if(temp=="Create"){
            tempThreadCount++;
    		line.erase(0,7);

    		int processNo, processSize;
    		for(int i=0;i<2;i++){
    			int commaAdress = find(line.begin(), line.end(), ',') - line.begin();
    			if(i==0) {processNo = stoi(line.substr(0,commaAdress)); line.erase(0,commaAdress+1);}
    			if(i==1) {processSize = stoi(line);}
    		}

    		pcb* t = new pcb(processNo, processSize);
            pcbMap[processNo] = t;
    		allPcb.push_back(t);

    		// creating 2 threads
    		if(!flag) {
    			flag = true;
    			//rc = pthread_create(&frameThread, NULL, freeFrameManager, NULL);
    			int* a;
	    		rc = pthread_create(&frameThread, NULL, freeFrameManager, (void *)a);
	    		if (rc){
					cout << "Error:unable to create frame thread," << rc << endl;
					exit(-1);
			    }

                rc = pthread_create(&pageIOThread, NULL, pageIOManager, (void *)a);
				if (rc){
					cout << "Error:unable to create page IO thread," << rc << endl;
					exit(-1);
			    }			    
    		}

    		rc = pthread_create(&tempThread[tempThreadCount], NULL, threadFunction, (void *) t);
    		if (rc){
				cout << "Error:unable to create thread," << rc << endl;
				exit(-1);
		    }
    		//tempThread[tempThreadCount] = thread(threadFunction, t);
      		//tempThread[tempThreadCount].join();
      		continue;
    	}

		temp = line.substr(0,10);
    	if(temp=="Page_table"){
    		showPageTable = true;
    		continue;
    	}
    }
    threadCount = tempThreadCount;

    // wait till all threads end
    while(threadEndTillNow != threadCount) {}

    if(showPageTable) {
		for(int i=0;i<allPcb.size();i++){
			cout << "Process No : " << allPcb[i]->processNo << endl;
			allPcb[i]->proc_pageTable->print();
		}    	
    }


	pthread_mutex_lock(&sizeCheck_mtx);
	pthread_cond_signal(&sizeCheck_cond);
	pthread_mutex_unlock(&sizeCheck_mtx);		

	pthread_cancel(frameThread);
	pthread_cancel(pageIOThread);

	pthread_exit(NULL);
}

/* init
Memory_size 10
Lower_threshold 7
Upper_threshold 9
Create 20,10
Create 25,2

*/

/* s20
Access 1,1112
Access 2,1132
Access 3,1132
Access 4,1132
Access 5,1132
Access 6,1132
Access 7,1112
Access 8,1132
Access 9,1132
Access 10,1132
Modify 1,1112
Modify 2,1132
Modify 3,1132
Modify 4,1132
Modify 5,1132
Modify 6,1132
Modify 7,1112
Modify 8,1132
Modify 9,1132
Modify 10,1132
End
*/

/* s25
Access 1,1234
End
*/
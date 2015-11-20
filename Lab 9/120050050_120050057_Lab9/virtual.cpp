#include <bits/stdc++.h>
#include <cstdlib>
#include <pthread.h>
#include <thread>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <unistd.h>
//#include "utils.h"

using namespace std;

//int DISK_BLOCK_SIZE = 999;
int DISK_BLOCK_SIZE = 199;
int DISK_SIZE = pow(10,6);
int ROOT = 1;
string DELIMITER = "|";
string diskBlockName = "example.txt";
FILE * pFile;


//Old variables 

void initDiskBlock() {
	pFile = fopen ( diskBlockName.c_str() , "wb+" );
	
	for(int i=0; i<DISK_SIZE; i++){
	fputs ( " " , pFile );
	} 
}

void print(std::vector<string> v) {
  for (std::vector<string>::iterator i = v.begin(); i != v.end(); ++i)
  {
    printf("%s\n",&(*i)[0]);
  }
}
// My variables

int nextFreeBlock = 2;
mutex freeBlockLock;
mutex blockLock[10000];
int maxDataByte = DISK_BLOCK_SIZE-10; //Inclusice
int pointerSize = DISK_BLOCK_SIZE - maxDataByte - 1;
mutex fseekLock;

/*
Say Block size is 13. maxDataByte = 3. So 4 bytes can be written 0,1,2,3;
Size of pointer = 4-12 => 9
Pointer size = 13 - 3 -1
*/


struct fcb{
	int startBlock;
	int currentBlock;
	int bytePosition;	
	fcb(int a, int b){
		startBlock = a;
		currentBlock = a;
		bytePosition = b;
	}
};
//My functions
int getNextFreeBlockDir(){
	freeBlockLock.lock();
	int temp = nextFreeBlock;
	assert(temp!=10000); //2<=temp<=9999
	nextFreeBlock++;
	freeBlockLock.unlock();
	return temp;
}

int getNextFreeBlock(){
	freeBlockLock.lock();
	int temp = nextFreeBlock;
	assert(temp!=10000); //2<=temp<=9999
	nextFreeBlock++;
	
	
	blockLock[temp].lock();
	fseekLock.lock();
	fseek(pFile, temp*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
	char buf[2];
	buf[0] = '|';
	buf[2] = '\0';
	fputs(buf,pFile);
	fseekLock.unlock();
	blockLock[temp].unlock();
	freeBlockLock.unlock();
	return temp; 
}



// utils.cpp

std::vector<std::string> &split(std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> split(std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

//////////////////

struct DirectoryEntry {
  string name;
  int locationIndex;
  int isDirectory;
  string user; // protection infot

  DirectoryEntry(){}

  DirectoryEntry(string n, int i, int b, string u) {
    name = n;
    locationIndex = i;
    isDirectory = b;
    user = u;
  }

  DirectoryEntry(std::vector<string> v) {
    name = v[0];
    locationIndex = stoi(v[1]);
    isDirectory = stoi(v[2]);
    user = v[3];
  }

  string convert() {
    string ret;
    ret = name + DELIMITER + to_string(locationIndex) + DELIMITER;
    ret += to_string(isDirectory) + DELIMITER + user;
    return ret;
  }

  void print() {
    printf("Name: %s, Location: %i, isDirectory: %i, user: %s\n", &name[0], 
      locationIndex, isDirectory, &user[0]);
  }
};

struct Directory {
	string name;
	//int parentIndex; 
	std::vector<DirectoryEntry*> listEntries;

	string convert() {
		string ret = name;
		ret += ",";
		for (std::vector<DirectoryEntry*>::iterator i = listEntries.begin(); i != listEntries.end(); ++i)
		{
			DirectoryEntry* temp = (*i);
			ret += temp->convert();
			ret += ",";
		}
		return ret;
	}

	void print() {
		printf("Name - %s, Number of Directory Entries - %i\n", 
			&name[0], int(listEntries.size()));
		for (std::vector<DirectoryEntry*>::iterator i = listEntries.begin(); i != listEntries.end(); ++i)
		{
			(*i)->print();
		}
	}

	DirectoryEntry* find(string n) {
		for (std::vector<DirectoryEntry*>::iterator i = listEntries.begin(); i != listEntries.end(); ++i)
		{
			if((*i)->name == n) return *i;
		}
		return NULL;
	}
};


struct pageTable;

struct pcb{ 
	int processNo, processSize;
	int directoryBlock;
	string user;
	pageTable* proc_pageTable;
	int seq, nextFill; //Should remove this
  	pcb(int a, int b, int c, string d){
  		processNo = a;
  		processSize = b;
  		proc_pageTable = NULL;
  		directoryBlock = c;
  		user = d;
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
mutex ramLock;
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
	assert(!(f->dirty));
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
			pcbMapLock.lock();
			ramLock.lock();
			printf("FFM: Activated \n");
			int changeFrameNo = lru();
			assert(changeFrameNo != -1);
			frame* changeFrame = ram[changeFrameNo];
			if(changeFrame->dirty) {
				printf("FFM: instructed the page I/O manager to write out a dirty page \n");

                pageIOEntry temp(changeFrame->pageNo, changeFrame->pid, changeFrameNo, false);				
				pthread_mutex_lock(&freeFrame_mtx);
				pageIOTable.push(temp);
				pthread_cond_wait(&freeFrame_cond, &freeFrame_mtx);
				pthread_mutex_unlock(&freeFrame_mtx);
                
				int pageNum = changeFrame->pageNo;
				int pid = changeFrame->pid;
				//pcbMapLock.lock();
				pcbMap[pid]->proc_pageTable->rows[pageNum].valid = false;
				pcbMap[pid]->proc_pageTable->rows[pageNum].frameNum = -1;
				

				changeFrame->dirty = false;
				insertIntoFreeList(changeFrame);
			}

			else {
				if(changeFrame->valid) {
					
					int pageNum = changeFrame->pageNo;
					int pid = changeFrame->pid;
					
					//pcbMapLock.lock();
					pcbMap[pid]->proc_pageTable->rows[pageNum].valid = false;
					pcbMap[pid]->proc_pageTable->rows[pageNum].frameNum = -1;
					//pcbMapLock.unlock();
				}
				insertIntoFreeList(changeFrame);
			}
			printf("FFM: Added page frame %i into free list\n", changeFrameNo);
			pcbMapLock.unlock();
			ramLock.unlock();
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
	ramLock.lock();

	int processSize = pcbMap[pid]->processSize;
	pcb* proc_pcb = pcbMap[pid];
	pageTable* proc_pageTable = pcbMap[pid]->proc_pageTable;

	if(pageNo<0 || pageNo>=processSize) {
		mmuLock.unlock(); 
		pcbMapLock.unlock();
		ramLock.unlock();
		return 1;
	}
	
	pageTableEntry ptEntry = proc_pageTable->rows[pageNo];
	int frameNum = ptEntry.frameNum;
	if(ptEntry.valid) {
		ram[frameNum]->seq = sequenceNext;
		sequenceNext++;
		if(isModify) {
			ram[frameNum]->dirty = true;
		}
		printf("Accessed page frame number %i\n", frameNum);
		ramLock.unlock();
		mmuLock.unlock();
		pcbMapLock.unlock();
		return 2;
	}
	ramLock.unlock();
	mmuLock.unlock();
	pcbMapLock.unlock();
	return 3;
}

int page_fault_handler(int pageNum, int pid) {//CHECK, Not using anylock for freeFrameList
	pcbMapLock.lock();

	pcb* proc_pcb = pcbMap[pid];
	pageTable* proc_pageTable = proc_pcb->proc_pageTable;
	
	freeFramesLock.lock();
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


    pageIOEntry temp(pageNum, pid, nextFrame->index, true);

	pthread_mutex_lock(&pageIn_mtx);
	pageIOTable.push(temp);
	pthread_cond_wait(&pageIn_cond[temp.pid], &pageIn_mtx);
	pthread_mutex_unlock(&pageIn_mtx);


	// set up the page in ram
	ramLock.lock();
	nextFrame->valid = true;
	nextFrame->inFreeList = false;
	nextFrame->seq = sequenceNext;
	nextFrame->pid=pid;
	nextFrame->pageNo=pageNum;
	sequenceNext++;

	// set up an entry in pid's pt
	pageTableEntry ptEntry(pageNum,nextFrame->index,true);
	proc_pageTable->rows[pageNum] = ptEntry;
	printf("%i: Loaded %i into frame %i\n", pid, pageNum, nextFrame->index);
	ramLock.unlock();
	freeFramesLock.unlock();
	pcbMapLock.unlock();
}

void readFromDisk(int blockNum, char* buf) {
	fseekLock.lock();
	fseek(pFile, (blockNum)*DISK_BLOCK_SIZE, SEEK_SET);
	int size = fread(buf, 1, DISK_BLOCK_SIZE, pFile);
	fseekLock.unlock();
	if(size != DISK_BLOCK_SIZE) {
	fputs ("Memory error",stderr);
	exit(2);
	}
}

void writeIntoDisk(int blockNum, char* buf) {
	fseekLock.lock();
	fseek(pFile, (blockNum)*DISK_BLOCK_SIZE, SEEK_SET);
	int i=0; 
	while(buf[i] != '\0') {
	i++;
	}
	assert(i<=DISK_BLOCK_SIZE);
	fputs(buf,pFile);
	fseekLock.unlock();
}

// Mine
pair<int,int> writeMultipleBlocks(int startBlockNum, int startByteNum, string data, 
	string fileName, int pid){
	// Returns last block and lasy byte + 1, So can write freshly without any procesing
	int temporaryBlock = startBlockNum;
	int temporaryByte = startByteNum;
	if(startByteNum==maxDataByte+1){
		int lockingBlock = temporaryBlock; 
		blockLock[lockingBlock].lock();
		int temp = getNextFreeBlock();
		string tempString = to_string(temp);
		tempString += "|";	
		fseekLock.lock();	
		fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
		fputs(tempString.c_str(), pFile);
		fseekLock.unlock();
		temporaryBlock = temp;
		temporaryByte = 0;
		printf("%i: write: New disk block %i allocated to file %s", pid, temporaryBlock, fileName.c_str());
		blockLock[lockingBlock].unlock();
	}
	
	string stringRemaining = data;
	while(stringRemaining.length() > maxDataByte - temporaryByte + 1){
		int lockingBlock = temporaryBlock; 
		blockLock[lockingBlock].lock();
		fseekLock.lock();
		fseek(pFile, temporaryBlock*DISK_BLOCK_SIZE + temporaryByte, SEEK_SET);
		string stringKeeping = stringRemaining.substr(0,maxDataByte - temporaryByte + 1);
		fputs(stringKeeping.c_str(),pFile);
		//fseekLock.unlock();
		stringRemaining.erase(0,maxDataByte -temporaryByte + 1);
		int nextBlock = getNextFreeBlock();
		string pointerSet = to_string(nextBlock);
		pointerSet += "|";
		//fseekLock.lock();
		fseek(pFile, temporaryBlock*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
		fputs(pointerSet.c_str(), pFile);
		fseekLock.unlock();
		temporaryBlock = nextBlock;
		temporaryByte = 0;
		printf("%i: write: New disk block %i allocated to file %s", pid, temporaryBlock, fileName.c_str());
		blockLock[lockingBlock].unlock();
	} 

	int lockingBlock = temporaryBlock;
	blockLock[lockingBlock].lock();
	fseekLock.lock();
	fseek(pFile, temporaryBlock*DISK_BLOCK_SIZE + temporaryByte, SEEK_SET);
	fputs(stringRemaining.c_str(), pFile);
	fseekLock.unlock();
	blockLock[lockingBlock].unlock();
	temporaryByte = stringRemaining.length();
	return make_pair(temporaryBlock, temporaryByte);
}

pair<string,pair<int,int> > readMultipleBlocks(int startBlockNum, int startByteNum, 
	int size, string fileName, int pid){
	// Returns last block and lasy byte + 1, So can write freshly without any procesing

	int temporaryBlock = startBlockNum;
	int temporaryByte = startByteNum;

	if(startByteNum==maxDataByte+1){
		int lockingBlock = temporaryBlock;
		blockLock[lockingBlock].lock(); 
		fseekLock.lock();
		fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
		char* buf;
		buf = (char*) malloc(sizeof(char*)*pointerSize);
		int lengthRead = fread(buf, 1, pointerSize, pFile); 
		fseekLock.unlock();
		int temp = 0;
		for(int i=0; i<lengthRead; i++){
			if(buf[i]=='|') break;
			else temp = temp*10 + buf[i]-'0';
		}
		temporaryBlock = temp;
		temporaryByte = 0;
		blockLock[lockingBlock].unlock();
	}
	
	int remaining = size;
	string data = "";

	while(remaining > maxDataByte - temporaryByte + 1){
		//cout<<temporaryBlock<<" "<<temporaryByte<<endl;
		//cout<<"!"<<endl;
		int lockingBlock = temporaryBlock;
		blockLock[lockingBlock].lock(); 
		fseekLock.lock();
		fseek(pFile, temporaryBlock*DISK_BLOCK_SIZE + temporaryByte, SEEK_SET);
		char* buf;
		buf = (char*) malloc(sizeof(char*)*(maxDataByte - temporaryByte + 1)); ///
		string tempString;
		int lengthRead = fread(buf, 1, maxDataByte - temporaryByte + 1, pFile);
		//fseekLock.unlock();
		remaining -= maxDataByte - temporaryByte + 1;
		tempString = (string) buf;
		data += tempString;
		//cout<<tempString<<endl;
		//fseekLock.lock();
		fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
		lengthRead = fread(buf, 1, pointerSize, pFile);
		fseekLock.unlock();
		//cout<<buf<<endl; 
		int temp = 0;
		for(int i=0; i<lengthRead; i++){
			//cout<<buf<<" "<<buf[i]<<" "<<(buf[i]-'0')<<endl;
			if(buf[i]=='|') break;
			else temp = temp*10 + (buf[i]-'0');

		}
		printf("%i: read: disk block %i of file %s read \n", pid, lockingBlock, &fileName[0]);
		temporaryBlock = temp;
		temporaryByte = 0;
		blockLock[lockingBlock].unlock(); 
	} 

	int lockingBlock = temporaryBlock;
	blockLock[lockingBlock].lock();
	fseekLock.lock();
	fseek(pFile, temporaryBlock*DISK_BLOCK_SIZE + temporaryByte, SEEK_SET);
	char* buf;
	buf = (char*) malloc(sizeof(char*)*remaining);
	int lengthRead = fread(buf, 1, remaining, pFile);
	fseekLock.unlock();
	string bufString(buf, buf+lengthRead);  
	data += bufString;
	temporaryByte = remaining;
	blockLock[lockingBlock].unlock();
	printf("%i: read: disk block %i of file %s read \n", pid, lockingBlock, &fileName[0]);
	//printf("Read data ...... %s \n", data.c_str());
	return make_pair(data,make_pair(temporaryBlock, temporaryByte));
	//return make_pair(temporaryBlock, temporaryByte);
}


pair<int,int> seekMultipleBlocks(int startBlockNum, int startByteNum, int size, 
	int fileIndex, int pid){

	int temporaryBlock = startBlockNum;
	int temporaryByte = startByteNum;

	if(startByteNum==maxDataByte+1){
		int lockingBlock = temporaryBlock;
		blockLock[lockingBlock].lock(); 
		fseekLock.lock();
		fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
		char* buf;
		buf = (char*) malloc(sizeof(char*)*pointerSize);
		int lengthRead = fread(buf, 1, pointerSize, pFile); 
		fseekLock.unlock();
		int temp = 0;
		for(int i=0; i<lengthRead; i++){
			if(buf[i]=='|') break;
			else temp = temp*10 + buf[i]-'0';
		}
		if(temp != 0){
			temporaryBlock = temp;
			temporaryByte = 0;
		}
		else{

			fseekLock.lock();
			int nextBlock  = getNextFreeBlock();
			string nextBlockPointer = to_string(nextBlock);
			nextBlockPointer +=  "|";
			fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
			fputs(nextBlockPointer.c_str(),pFile);
			fseekLock.unlock();
			temporaryBlock = nextBlock;
			temporaryByte = 0;
			printf("%i: seek: Creating new block for seek on file at block %i \n",
			 pid, fileIndex);
		}
		blockLock[lockingBlock].unlock();
	}
	
	int remaining = size;

	while(remaining > maxDataByte - temporaryByte + 1){
		
		int lockingBlock = temporaryBlock;
		blockLock[lockingBlock].lock(); 

		remaining -= maxDataByte - temporaryByte + 1;
		char* buf;
		buf = (char*) malloc(sizeof(char*)*pointerSize); ///
		fseekLock.lock();
		fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
		int lengthRead = fread(buf, 1, pointerSize, pFile);
		fseekLock.unlock();
		//cout<<buf<<endl; 
		int temp = 0;
		for(int i=0; i<lengthRead; i++){
			//cout<<buf<<" "<<buf[i]<<" "<<(buf[i]-'0')<<endl;
			if(buf[i]=='|') break;
			else temp = temp*10 + (buf[i]-'0');

		}
		if(temp != 0){
			temporaryBlock = temp;
			temporaryByte = 0;
		}
		else{
			int nextBlock  = getNextFreeBlock();
			string nextBlockPointer = to_string(nextBlock) + "|";
			fseekLock.lock();
			fseek(pFile, (temporaryBlock)*DISK_BLOCK_SIZE + maxDataByte + 1, SEEK_SET);
			fputs(nextBlockPointer.c_str(),pFile);
			fseekLock.unlock();
			temporaryBlock = nextBlock;
			temporaryByte = 0;
			printf("%i: seek: Creating new block for seek on file at block %i \n",
			 pid, fileIndex);
		}
		blockLock[lockingBlock].unlock(); 
	} 

	temporaryByte += remaining; 
	return make_pair(temporaryBlock, temporaryByte);
	//return make_pair(temporaryBlock, temporaryByte);
}


bool readDirectory(int blockNum, Directory &dir) { // DPK changed
  char* input = (char*) malloc (sizeof(char)*DISK_BLOCK_SIZE);
  
  readFromDisk(blockNum, input);
  string str(input);

  std::vector<string> v = split(str,',');
  if(v.size() < 2) return false;
  
  dir.name = v[0];
  //cout << "Size " << v.size() << endl;

  for (std::vector<string>::iterator i = (v.begin()+1); i != (v.end()-1); ++i)
  {
    std::vector<string> e = split((*i),'|');
    
    //cout<<"Printing"<<endl;
    //print(e);
    //print(v);
    //cout<<"PrintingDone"<<endl;
    DirectoryEntry* entry = new DirectoryEntry(e);
    dir.listEntries.push_back(entry);
  }
  return true;
}

void writeDirectory(int blockNum, Directory d) {
	string ret = d.convert();
	//cout << ret << endl;
	writeIntoDisk(blockNum, &ret[0]);
}

int fileStartingBlock(string fileName, Directory *dir){
	std::vector<string> v = split(fileName, '/');
	//print(v);
	Directory currentDirectory = *dir;
	DirectoryEntry* entry;
	bool flag = false;
	if(v.size() > 1) {
		string errorMsg;
		for (std::vector<string>::iterator i = v.begin(); i != v.end(); ++i)
		{
			string changeDir1 = *i;
			//printf("changeDir1 %s \n" ,changeDir1.c_str());
			entry = currentDirectory.find(changeDir1);
			if(entry == NULL or entry->isDirectory!=1) {
				flag = true;
				errorMsg = changeDir1;
			}
		/*
			else {
				int temp = entry->locationIndex;
				//cout<<"!!!!!!!!"<<endl;	
				blockLock[temp].lock();
				//cout<<"2222222"<<endl;	
				Directory tempDir;
				bool ret = readDirectory(temp, tempDir);
				currentDirectory = tempDir;
				assert(ret);
				blockLock[temp].unlock();

			}
		}
		
		if(!flag) {
			currentDirectoryMap[userName] = entry->locationIndex;
		}*/
	}}
	else {
		for(int i=0; i< (dir->listEntries).size(); i++){
			if (dir->listEntries[i]->name == fileName){
				return dir->listEntries[i]->locationIndex;
			}
		}		
	}

	return -1;
}


void *threadFunction(void *InData){//CHANGE
	//pcbMapLock.lock();
	pcb* data;
	data = (pcb *) InData;
	int pid = data->processNo;
	int size = data->processSize;
	int directoryBlock = data->directoryBlock;
	string user = data->user;
 	data->proc_pageTable = new pageTable(size+1);
 	//pcbMapLock.unlock();
 	vector<fcb> fcbVector; //Adding this

 	
 	int lastFileIndex = -1;
 	//vector<mutex> allLocks;

 	string processFileName = "s";
	stringstream ss;
	ss << pid;
	processFileName += ss.str();

	ifstream ifs(processFileName.c_str());
	string line;
	int accessNum = 0, modifyNum = 0, pfNum = 0;
	
	while(getline(ifs,line)){
		
		if(line.substr(0,3)=="End"){
			printf("Number of access operations : %i \n"
				   "Number of modify operations : %i \n"
				   "Number of page faults : %i \n",
				    accessNum,
				    modifyNum,
				    pfNum); 
			printf("Exiting process %i \n", pid);
			threadEndTillNow++;
			break;
		}
		
		else if(line.substr(0,4)=="open"){
			//printf("directory block is %i \n", directoryBlock);
			string fileName = line.substr(5);
			blockLock[directoryBlock].lock();
			Directory dir;
			readDirectory(directoryBlock, dir); //Now dir has info
			
			int fileLocation = 	fileStartingBlock(fileName, &dir);
			if(fileLocation!=-1){ //Already present
				bool found = false;
				for(int i=0;i<fcbVector.size();i++){
					if(fcbVector[i].startBlock == fileLocation){
						fcbVector[i].bytePosition = 0;
						found = true;
						lastFileIndex = i;
					}
				}
				if(!found){
					fcbVector.push_back(fcb(fileLocation,0)); 
					lastFileIndex = fcbVector.size()-1;
				}
				printf("%i: open: File %s already present in directory", pid, fileName.c_str());
			}

			else{ //Creating
				fileLocation = getNextFreeBlock();
				DirectoryEntry* fileEntry = new DirectoryEntry(fileName, fileLocation, 0, user);
				bool addedSuccesfully;
				addedSuccesfully = true;//TODO Add function
				dir.listEntries.push_back(fileEntry);
				//dir.print(); 
				//printf("Before - %s \n", );
				writeDirectory(directoryBlock, dir);
				//EP
				//Directory d1;
				//readDirectory(directoryBlock, d1);
				//d1.print();
				//EP
				fcbVector.push_back(fcb(fileLocation,0));
				lastFileIndex = fcbVector.size()-1;
				printf("%i: open: File %s created, allocated diskblock %i \n", pid, fileName.c_str(), fileLocation);
			}

			blockLock[directoryBlock].unlock();
			continue;
		}

		else if(line.substr(0,4)=="read"){
			Directory dir;
			line.erase(0,5);
			int posSpace = find(line.begin(), line.end(), ' ') - line.begin();
			string fileName = line.substr(0,posSpace);
			int size = stoi(line.substr(posSpace+1));
			readDirectory(directoryBlock, dir); //Now dir has info
			int fileLocation = 	fileStartingBlock(fileName, &dir);
			if(fileLocation==-1){
				printf("%i: read: Cannot read file %s - Does not exist in directory \n", pid, fileName.c_str());
				continue;
			}
			else{
				int currentBlock = -1, nextByte = -1, index = -1;
				for(int i=0;i<fcbVector.size();i++){
					if(fcbVector[i].startBlock == fileLocation){
						currentBlock = fcbVector[i].currentBlock;
						nextByte = fcbVector[i].bytePosition;
						index = i;
						break;
					}
				}
				if(currentBlock != -1){ // File already opened
					auto returnVal = readMultipleBlocks(currentBlock, nextByte, size, fileName, pid);
					fcbVector[index].currentBlock = returnVal.second.first;
					fcbVector[index].bytePosition = returnVal.second.second;
					lastFileIndex = index;
					printf("%i: read: value %s \n", pid, returnVal.first.c_str());
				}
				else{ //Should open file and read it
					fcbVector.push_back(fcb(fileLocation,0));
					auto returnVal = readMultipleBlocks(fileLocation, 0 ,size, fileName, pid);
					fcbVector[fcbVector.size()-1].currentBlock = returnVal.second.first;
					fcbVector[fcbVector.size()-1].bytePosition = returnVal.second.second;
					lastFileIndex = fcbVector.size() - 1;
					printf("%i: read: File opened. value %s \n", pid, returnVal.first.c_str());
				}	
			}
			continue;
		}
		
		else if(line.substr(0,5)=="write"){
			Directory dir;
			line.erase(0,6);
			//cout<<line<<endl;
			auto splitVec  = split(line, ' ');
			//cout<<splitVec[0]<<" "<<splitVec[1]<<" "<<endl;
			string fileName = splitVec[0];
			int bytes = stoi(splitVec[1]);
			//cout<<"!!!!!"<<endl;
			string data = splitVec[2];
			if(bytes != data.length()){
				printf("%i: write: No. of bytes and value length does not match \n", pid);
				continue;
			} 

			readDirectory(directoryBlock, dir); //Now dir has info
			int fileLocation = 	fileStartingBlock(fileName, &dir);
			if(fileLocation==-1){
				printf("%i: write: Cannot write file %s - Does not exist in directory \n", pid, fileName.c_str());
				continue;
			}
			else{
				int currentBlock = -1, nextByte = -1, index = -1;
				for(int i=0;i<fcbVector.size();i++){
					if(fcbVector[i].startBlock == fileLocation){
						currentBlock = fcbVector[i].currentBlock;
						nextByte = fcbVector[i].bytePosition;
						index = i;
						break;
					}
				}
				if(currentBlock != -1){ // File already opened
					//auto returnVal = readMultipleBlocks(currentBlock, nextByte, size);
					auto returnVal = writeMultipleBlocks(currentBlock, nextByte,
					 data, fileName, pid);
					fcbVector[index].currentBlock = returnVal.first;
					fcbVector[index].bytePosition = returnVal.second;
					lastFileIndex = index;
					printf("%i: write: File %s already opened, Writing done \n", pid, fileName.c_str());
				}
				else{ //Should open file and write it
					fcbVector.push_back(fcb(fileLocation,0));
					auto returnVal = writeMultipleBlocks(fileLocation, 0 ,
						data, fileName, pid);
					fcbVector[fcbVector.size()-1].currentBlock = returnVal.first;
					fcbVector[fcbVector.size()-1].bytePosition = returnVal.second;
					lastFileIndex = fcbVector.size() - 1;
					printf("%i: write File %s opened and Wrote \n", pid, fileName.c_str());
				}	
			}
			continue;
		}
		
		else if(line.substr(0,4)=="seek"){
			Directory dir;
			auto splitVec = split(line, ' ');
			if(splitVec.size()==2){
				int fileStartLocation = fcbVector[lastFileIndex].startBlock;
				auto returnVal = seekMultipleBlocks(fileStartLocation, 0, stoi(splitVec[1])
					, fileStartLocation, pid);
				fcbVector[lastFileIndex].currentBlock = returnVal.first;
				fcbVector[lastFileIndex].bytePosition = returnVal.second; 
			}
			else if(splitVec.size()==3){
				int fileStartLocation = fcbVector[lastFileIndex].startBlock;
				int fileBlockLocation = fcbVector[lastFileIndex].currentBlock;
				int fileByteLocation = fcbVector[lastFileIndex].bytePosition;
				auto returnVal = seekMultipleBlocks(fileBlockLocation, fileByteLocation, 
					stoi(splitVec[2]), fileStartLocation, pid);
				fcbVector[lastFileIndex].currentBlock = returnVal.first;
				fcbVector[lastFileIndex].bytePosition = returnVal.second;
			}
			continue;
		}

		else{
			string arg1 = line.substr(0,6); //Same
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
			
			printf("%i: attempted to %s %i %i\n", pid, arg1.c_str(), arg2Num, arg3Num);
			
			pcb* proc_pcb = pcbMap[pid];
			pageTable* proc_pageTable = pcbMap[pid]->proc_pageTable;		

			int status = mmu(arg2Num, pid, (arg1 == "Modify"));
			if(status == 3) {
				printf("%i: Reported a page fault \n", pid);
				pfNum++;
				page_fault_handler(arg2Num, pid);
				
				pageTableEntry ptEntry = proc_pageTable->rows[arg2Num];
				int frameNum = ptEntry.frameNum;
				if(arg1 == "Modify") {
					ramLock.lock();
					ram[frameNum]->dirty = true;
					ramLock.unlock();
				}
			}
			else if(status == 1) {
				printf("%i: Reported a memory protection violation\n", pid); 
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
    initDiskBlock();
	
	int numDiskBlocks = DISK_SIZE/DISK_BLOCK_SIZE;
	string superBlock = to_string(numDiskBlocks) + DELIMITER + to_string(ROOT);
	superBlock += DELIMITER + to_string(0);

	//cout << &superBlock[0] << endl;
	writeIntoDisk(0,&superBlock[0]);

	std::map<string, int> currentDirectoryMap;
    while(getline(ifs,line)){
    	//printf("%s \n", line.c_str());
    	
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
		if(temp=="create"){

			int processNo, processSize;
			std::vector<string> v = split(line,' ');
			string userName = v[1];
			processNo = stoi(v[2]);
			processSize = stoi(v[3]);
			
			if(currentDirectoryMap.count(userName) == 0) {
				printf("create: user %s does not exist\n", &userName[0]);
				continue;
			}
	    	tempThreadCount++;

			pcb* t = new pcb(processNo, processSize, currentDirectoryMap[userName], v[1]);
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

		temp = line.substr(0,5);
		if(temp == "uname") {
			blockLock[ROOT].lock();
			string userName = line.substr(6);
			
			Directory rootDirectory;
			bool ret = readDirectory(ROOT, rootDirectory); //TODO: get ROOT from super block
			//printf("User: %s, ret: %i\n", &userName[0], ret);

			if(ret) {
				std::vector<DirectoryEntry*> homeDirs = rootDirectory.listEntries;
				bool flag = false;
				for (auto i = homeDirs.begin(); i != homeDirs.end(); ++i)
				{
					DirectoryEntry* temp = (*i);
					if(temp->isDirectory) {
						if(temp->name == userName) {
							flag = true;
						}
					}
				}
				if(!flag) {
					int homeDir = getNextFreeBlockDir();
					blockLock[homeDir].lock();

					Directory home;
					home.name = userName;
					DirectoryEntry parent("..", ROOT, 1, userName);
					DirectoryEntry self(".", homeDir, 1, userName);
					home.listEntries.push_back(&self);
					home.listEntries.push_back(&parent);
					writeDirectory(homeDir, home);
					blockLock[homeDir].unlock();

					DirectoryEntry entry(userName, homeDir, 1, userName);
					rootDirectory.listEntries.push_back(&entry);
					writeDirectory(ROOT, rootDirectory);

					currentDirectoryMap[userName] = homeDir;
				}
			}
			else {// create root directory also
				int homeDir = getNextFreeBlockDir();
				blockLock[homeDir].lock();

				Directory home;
				home.name = userName;
				DirectoryEntry parent("..", ROOT, 1, userName);
				DirectoryEntry self(".", homeDir, 1, userName);
				home.listEntries.push_back(&self);
				home.listEntries.push_back(&parent);
				writeDirectory(homeDir, home);
				blockLock[homeDir].unlock();

				Directory root;
				root.name = "root";
				DirectoryEntry parent1("..", -1, 1, "root");
				DirectoryEntry self1(".", ROOT, 1, "root");
				DirectoryEntry entry(userName, homeDir, 1, userName);
				root.listEntries.push_back(&self1);
				root.listEntries.push_back(&parent1);
				root.listEntries.push_back(&entry);
				writeDirectory(ROOT, root);
				blockLock[ROOT].unlock();

				currentDirectoryMap[userName] = homeDir;
			}
			blockLock[ROOT].unlock();
		}

		temp = line.substr(0,5);
		if(temp == "mkdir") {
			std::vector<string> parts = split(line, ' ');
			if(parts.size() != 3) {
				fputs("mkdir arguments not correct", stderr);
			}
			string userName = parts[1];
			string newDir = parts[2];

			//bool ret = readDirectory(currentDirectory[userName], rootDirectory); //TODO: get ROOT from super block
			bool ret = (currentDirectoryMap.count(userName) > 0);
			if(ret) {
				int currDir = currentDirectoryMap[userName];
				blockLock[currDir].lock();

				Directory currentDirectory;
				bool ret = readDirectory(currDir, currentDirectory);
				assert(ret);
				//blockLock[currDir].unlock();

				int next = getNextFreeBlockDir();
				blockLock[next].lock();
				
				DirectoryEntry entry(newDir, next, 1, userName);
				if(currentDirectory.find(newDir) != NULL) {
					printf("mkdir: Directory %s already exists\n", &newDir[0]);
				}
				else {
					currentDirectory.listEntries.push_back(&entry);
					writeDirectory(currDir, currentDirectory);

					Directory home;
					home.name = newDir;
					DirectoryEntry parent("..", currDir, 1, userName);
					DirectoryEntry self(".", next, 1, userName);
					home.listEntries.push_back(&self);
					home.listEntries.push_back(&parent);
					//home.parentIndex = currDir;
					writeDirectory(next, home);
				}
				
				blockLock[next].unlock();
				blockLock[currDir].unlock();
			}
			else {
				printf("mkdir: User %s does not exist\n", &userName[0]);
			}
		}

		temp = line.substr(0,2);
		if(temp == "cd") {
			std::vector<string> parts = split(line, ' ');
			if(parts.size() != 3) {
				fputs("mkdir arguments not correct", stderr);
			}
			string userName = parts[1];
			string changeDir = parts[2];

			bool ret = (currentDirectoryMap.count(userName) > 0);
			if(ret) {
				int currDir = currentDirectoryMap[userName];
				blockLock[currDir].lock();

				Directory currentDirectory;
				bool ret = readDirectory(currDir, currentDirectory);
				assert(ret);

				DirectoryEntry* entry = currentDirectory.find(changeDir);
				blockLock[currDir].unlock();
				if(entry == NULL) {
					string errorMsg = changeDir;
					std::vector<string> v = split(changeDir, '/');
					//print(v);
					bool flag = false;
					if(v.size() > 1) {
						for (std::vector<string>::iterator i = v.begin(); i != v.end(); ++i)
						{
							string changeDir1 = *i;
							//printf("changeDir1 %s \n" ,changeDir1.c_str());
							entry = currentDirectory.find(changeDir1);
							if(entry == NULL or entry->isDirectory!=1) {
								flag = true;
								errorMsg = changeDir1;
								break;
							}
							else {
								int temp = entry->locationIndex;
								//cout<<"!!!!!!!!"<<endl;	
								blockLock[temp].lock();
								//cout<<"2222222"<<endl;	
								Directory tempDir;
								bool ret = readDirectory(temp, tempDir);
								currentDirectory = tempDir;
								assert(ret);
								blockLock[temp].unlock();

							}
						}
						
						if(!flag) {
							currentDirectoryMap[userName] = entry->locationIndex;
						}
					}
					if(v.size() <= 1 or flag) {
						printf("cd: Directory %s not found\n", &errorMsg[0]);
					}
					/*
					}*/
				}
				else {
					 if(!entry->isDirectory) {
					 	printf("cd: %s Not a directory\n", &changeDir[0]);
					 }
					 else {
					 	currentDirectoryMap[userName] = entry->locationIndex;
					 }
				}
			}
			else {
				printf("cd: User %s does not exist\n", &userName[0]);
			}		
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
uname sgondala
uname mskd
create mskd 20 10
create sgondala 25 2
mkdir mskd Desktop
mkdir sgondala Desktop
mkdir mskd Documents
cd mskd ./Desktop/
cd mskd ../Documents/a
cd sgondala Desktop
mkdir mskd Downloads
*/

/* s20
Access 1,1112
Access 2,1132
Access 3,1132
Access 4,1132
Access 3,1132
Access 2,1132
End
*/

/* s25
Access 1,1234
End
*/
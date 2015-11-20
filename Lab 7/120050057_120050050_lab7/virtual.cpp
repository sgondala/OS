#include <bits/stdc++.h>
#include <cstdlib>
#include <pthread.h>
#include <thread>
  
using namespace std;
struct pageTable;

struct pcb{
	int processNo, processSize, pageFrameStart, pageFrameEnd, nextFill;
	pageTable* proc_pageTable;
	int seq;
  	pcb(int a, int b, int c, int d){
  		processNo = a;
  		processSize = b;
  		pageFrameStart = c;
  		pageFrameEnd = d;
  		nextFill = c;
  		proc_pageTable = NULL;
  		seq = 0;
  	}
};

struct frame {
	bool valid;
	frame(bool i) {
		valid = i;
	}
	frame(){
		valid = false;
	}
};

struct pageTableEntry {
	int pageNum;
	int frameNum;
	bool valid;
	int seq;
	pageTableEntry(){
		valid = false;
	}
	pageTableEntry(int i, int j, bool k, int l){
		pageNum = i;
		frameNum = j;
		valid = k;
		seq = l;
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
};


frame* ram[1000];
map<int, pcb*> pcbMap;							

int mmu(int pageNo, int pid){
	int processSize = pcbMap[pid]->processSize;
	pcb* proc_pcb = pcbMap[pid];
	pageTable* proc_pageTable = pcbMap[pid]->proc_pageTable;
	pageTableEntry ptEntry = proc_pageTable->rows[pageNo];

	if(pageNo<0 || pageNo>=processSize) {return 1;}
	if(ptEntry.valid) {
		proc_pageTable->rows[pageNo].seq = proc_pcb->seq;
		proc_pcb->seq++;
		cout << "Accessed page frame number " << proc_pageTable->rows[pageNo].frameNum << "\n";
		return 2;
	}
	return 3;
}

int page_fault_handler(int pageNum, int pid) {
	pcb* proc_pcb = pcbMap[pid];
	pageTable* proc_pageTable = proc_pcb->proc_pageTable;
	
	int nextFrame = proc_pcb->nextFill;

	if(nextFrame <= proc_pcb->pageFrameEnd) {// there is an empty frame which can be used for page-in
		frame* f = ram[nextFrame];
		f->valid = true;

		pageTableEntry ptentry(pageNum,nextFrame,true,proc_pcb->seq);
		proc_pcb->seq++;
		proc_pageTable->rows[pageNum]=ptentry;
		cout << "Loaded " << pageNum << " into frame " << nextFrame << "\n";
		proc_pcb->nextFill++;
	}
	else {// LRU should be used to page-out and then a page-in has to be performed
		//cout << "LRU" << endl; 
		pageTableEntry changeFrame;
		int seqMin;
		bool flag = true;
		//for (int i = proc_pcb->pageFrameStart; i <= proc_pcb->pageFrameEnd; ++i)
		for (std::vector<pageTableEntry>::iterator i = proc_pageTable->rows.begin(); 
			i != proc_pageTable->rows.end(); ++i)
		{
			if(!i->valid) continue;

			assert(ram[i->frameNum]->valid);
			
			if(flag) {
				seqMin = i->seq;
				changeFrame = *i;
				flag = false;
			}
			else {
				if(seqMin > i->seq) {
					seqMin = i->seq;
					changeFrame = *i;
				}
			}
		}

		proc_pageTable->rows[changeFrame.pageNum].valid = false;
		cout << "Removed " << changeFrame.pageNum << " from frame " << changeFrame.frameNum << "\n";
		
		pageTableEntry ptentry(pageNum,changeFrame.frameNum,true,proc_pcb->seq);
		proc_pcb->seq++;

		proc_pageTable->rows[pageNum]=ptentry;
		cout << "Loaded " << pageNum << " into frame " << changeFrame.frameNum << "\n";
	}
}


void threadFunction(pcb* data){
	int pid = data->processNo;
	int size = data->processSize;
 	data->proc_pageTable = new pageTable(size);
	pcbMap[pid] = data;

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
		if(isEnd == "end") {
			//cout << "end" << "\n";
			cout << "Number of access operations : " << accessNum << "\n"; 
			cout << "Number of modify operations : " << modifyNum << "\n"; 
			cout << "Number of page faults : " << pfNum << "\n"; 
			break;
		}

		//cout<<"5"<<endl;	
		string arg1 = line.substr(0,6);
		//cout<<"6"<<endl;
		string arg2 = line.substr(7,3);
		//cout<<"7"<<endl;
		string arg3 = line.substr(10,4);
		
		istringstream ss(arg2);
		int arg2Num;
		ss >> arg2Num;
		
		istringstream ss1(arg3);
		int arg3Num;
		ss1 >> arg3Num;

		//cout << arg1 << " " << arg2Num << " " << arg3Num << " " << "\n";
		cout << pid << ": attempted to ";
		cout << arg1 << " " << arg2Num << " " << arg3Num << "\n";
		
		int status = mmu(arg2Num, pid);
		if(status == 3) {
			cout << "Reported a page fault \n";
			pfNum++;
			page_fault_handler(arg2Num, pid);
		}
		else if(status == 1) {
			cout << "Reported a memory protection violation\n"; 
		}
		if(status == 2 || status == 3) {
			if(arg1 == "access") {
				accessNum++;
			}
			else if(arg1 == "modify") {
				modifyNum++;
			}
		}
	}
}

int main(){
    for(int i=0; i<1000; i++){
		frame* f = new frame();
		ram[i] = f;
    }
    vector<pthread_t> threads(1000);
    vector<pcb*> td(1000);
    ifstream ifs("init");
    string line;
    int i=0, rc;
    while(getline(ifs,line)){
    	//cout<<"1"<<endl;
        string temp = line.substr(7,2);
        istringstream ss(temp);
        int processNo;
        ss >> processNo;
        
        //cout<<"2"<<endl;  
        temp = line.substr(10,3);
        istringstream s2(temp);
        int processSize;
        s2 >> processSize;
        
        //cout<<"3"<<endl;  
        temp = line.substr(13,3);
        int pageFrameStart;
        istringstream s3(temp);
        s3 >> pageFrameStart;
        
        //cout<<"4"<<endl;  
        temp = line.substr(17,3);
        int pageFrameEnd;
        istringstream s4(temp);
        s4 >> pageFrameEnd;
        
        //cout<<"HI "<<processNo<<" "<<processSize<<" "<<pageFrameStart<<" "<<pageFrameEnd<<"\n";
        pcb* t = new pcb(processNo, processSize, pageFrameStart, pageFrameEnd);
      	
      	thread tempThread(threadFunction, t);
      	tempThread.join();
      	i++; 	
    }

}
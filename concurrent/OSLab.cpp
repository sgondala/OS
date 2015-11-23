#include <bits/stdc++.h>
#include <cstdlib>
#include <pthread.h>
#include <thread>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <unistd.h>
using namespace std;


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


bool flights[10][100];
mutex seats[10][100];
int numBlockedThreads = 5;

mutex numBlockedThreadsMutex;

pthread_mutex_t threadBlock_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  threadBlock_cond = PTHREAD_COND_INITIALIZER;

mutex sharedData_mtx;
vector<int> data(3,0);
list<vector<int> > sharedData;

pthread_mutex_t numBlock_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t numBlock_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t mtx = PTHREAD_COND_INITIALIZER;
pthread_cond_t mtx1 = PTHREAD_COND_INITIALIZER;

void* threadFunction(void *a){
  int* tempIntPointer = (int *) a;
  int threadNumber = *tempIntPointer;
  while(true){
    pthread_mutex_lock(&threadBlock_mtx);
    pthread_cond_wait(&threadBlock_cond, &threadBlock_mtx);

    std::vector<int> v = data;

    pthread_cond_signal(&mtx1);
    pthread_mutex_unlock(&threadBlock_mtx);

    // code
    if(v[0]==0){
            seats[v[1]][v[2]].lock(); //Locked that seat
            bool available = flights[v[1]][v[2]];
            if(available) {
                printf("Slave thread %i, Query - Status, Flight no %i, seat no %i is available \n", threadNumber, v[1], v[2]);
            }
            else{
                printf("Slave thread %i, Query - Status, Flight no %i, seat no %i is not available \n", threadNumber, v[1], v[2]);

            }
            seats[v[1]][v[2]].unlock();
        }

    else if(v[0]==1){
            seats[v[1]][v[2]].lock(); //Locked that seat
            bool available = flights[v[1]][v[2]];
            if(available) {
                flights[v[1]][v[2]] = false;
                printf("Slave thread %i, Query - Book, Flight no %i, seat no %i is booked \n", threadNumber, v[1], v[2]);
            }
            else{
                printf("Slave thread %i, Query - Book, Flight no %i, seat no %i is already booked \n", threadNumber, v[1], v[2]);

            }
            seats[v[1]][v[2]].unlock();    
        }

    else if(v[0]==2){
            seats[v[1]][v[2]].lock(); //Locked that seat
            bool available = flights[v[1]][v[2]];
            if(available) {
                printf("Slave thread %i, Query - Cancel, Flight no %i, seat no %i is not booked, Can't cancel \n", threadNumber, v[1], v[2]);
            }
            else{
                printf("Slave thread %i, Query - Cancel, Flight no %i, seat no %i is cancelled \n", threadNumber, v[1], v[2]);
                flights[v[1]][v[2]] == true;

            }
            seats[v[1]][v[2]].unlock();    
    }
    pthread_mutex_lock(&threadBlock_mtx);
    numBlockedThreads++;
    pthread_cond_signal(&mtx);
    pthread_mutex_unlock(&threadBlock_mtx);
  }
  pthread_exit(NULL);
}

int main(){
    pthread_t slave[5];
    memset(flights, true, sizeof(flights));
    ifstream ifs("instructions");
    string line;
    for(int i=0; i<5; i++){ // Initializing threads
        int* a = new int();
        *a = i;
        int rc  = pthread_create(&slave[i], NULL, threadFunction, (void *) a);
        if (rc){
          printf("error in creating thread %i", i);
        } 
    }
    
    while(getline(ifs,line)){
        vector<string> parts = split(line,' ');
        if(parts[0] == "END") {
            cout << "All queries have been processed " << endl;
            break;
        }
        vector<int> instrn;
        instrn.push_back(stoi(parts[0]));
        instrn.push_back(stoi(parts[1]));
        instrn.push_back(stoi(parts[2]));

        pthread_mutex_lock(&threadBlock_mtx);
        if(numBlockedThreads == 0){
            pthread_cond_wait(&mtx,&threadBlock_mtx);
        }

        data = instrn;
        pthread_cond_signal(&threadBlock_cond);
        numBlockedThreads--;

        pthread_cond_wait(&mtx1,&threadBlock_mtx);

        pthread_mutex_unlock(&threadBlock_mtx);

    }
    pthread_cancel(slave[0]);
    pthread_cancel(slave[1]);
    pthread_cancel(slave[2]);
    pthread_cancel(slave[3]);
    pthread_cancel(slave[4]);

    pthread_exit(NULL);
}
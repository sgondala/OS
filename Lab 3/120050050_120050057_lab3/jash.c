#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLINE 1000
#define DEBUG 0

/* Function declarations and globals */
int parent_pid ;
char ** tokenize(char*) ;
int execute_command(char** tokens,int count) ;
int current_child_pid = -1;
int shouldExit=0; // if exit is called shouldexit is set to 1 

int pidVals[100];
int countPid=0;
int isParallel = 0; //0 is false 

int BackGroundPID[100];
int BackGroundPIDCount=0;


int adjustArray(int pid){
	int i;
	int found=1;
	for(i=0;i<BackGroundPIDCount;i++){
		if(BackGroundPID[i]==pid) {found=0;break;}
	}
	while(i<BackGroundPIDCount){
		BackGroundPID[i+1]=BackGroundPID[i];
		i++;
	}
	BackGroundPIDCount--;
	return found;
}

void signal_handler(int signal){ //Signal Handler
	int status;
	int pid;
	switch (signal){
		case SIGINT:
			//printf("%s\n", "Should kill active child");
			if(current_child_pid != -1) {kill(current_child_pid,SIGKILL);}
			//else {printf("\n%s\n", "No child process running");}
			fflush(stdin);
			break;
		case SIGQUIT:
			if(current_child_pid != -1) {kill(current_child_pid,SIGKILL);}
			//else {printf("\n%s\n", "No child process running");}
			fflush(stdin);
			break;
		case SIGCHLD:
			pid = wait(&status);
			printf("Status is %d, PID is %d \n",status,pid);
			if(WIFEXITED(status)) { // get the status of returned child process
				status = WEXITSTATUS(status);
			}
			if(!adjustArray(pid)){ 
				fflush(NULL);
				printf("Status code is %d, PID is %d \n$ ",status,pid);
				fflush(NULL);
			}
			break;	
	}
}

int AfterCurrent(int hour, int min) {
		time_t rawtime;
		char buffer [80];	

		struct tm * timeinfo1;
		time ( &rawtime );
		timeinfo1 = localtime ( &rawtime );

		timeinfo1->tm_hour = hour;
		timeinfo1->tm_min = min;

		time_t t = mktime(timeinfo1);

		time_t curr;
		time(&curr);

		double diff = difftime(t, curr);

		//strftime (buffer,80,"Now it's %H %M.",timeinfo1);
	  	//puts (buffer);
		
		//printf("%f\n",diff );
		int ans = (int)diff;
		return ans;
}

int cron(int hour, int min)
{
	if(hour!=-1 && min!=-1) {
		time_t rawtime;
		char buffer [80];	

		struct tm * timeinfo1;
		time ( &rawtime );
		timeinfo1 = localtime ( &rawtime );

		timeinfo1->tm_hour = hour;
		timeinfo1->tm_min = min;

		time_t t = mktime(timeinfo1);

		time_t curr;
		time(&curr);

		double diff = difftime(t, curr);

		//strftime (buffer,80,"Now it's %H %M.",timeinfo1);
	  	//puts (buffer);
		
		//printf("%f\n",diff );
		int ans = (int)diff;
		if(ans<0) ans+=86400;
		return ans;
	}
	
	else if(hour == -1) {
		int h;
		int temp = 0;
		for(h=0; h<=23; h++) {
			if(AfterCurrent(h,min)>=0) {
				temp = 1;
				break;
			}
		}
		if(temp) {
			int diff = AfterCurrent(h,min);
			//printf("%i\n",diff );		
			return diff;
		}
		else {
			int diff = cron(0,min);
			return diff;
		}
	}
	
	else if(min == -1) {
		int m;
		int temp = 0;
		for(m=0; m<=59; m++) {
			if(AfterCurrent(hour,m)>=0) {
				temp = 1;
				break;
			} 
		}
		if(temp) {
			int diff = AfterCurrent(hour,m);
			//printf("%i\n",diff );		
			return diff;
		}
		else {
			int diff = cron(hour,0);
			return diff;
		}
	}
	
}


int main(int argc, char** argv){
	setbuf(stdout, NULL);
	parent_pid = getpid() ;
	
	struct sigaction sa;
	sa.sa_handler = &signal_handler;
	sa.sa_flags = SA_RESTART;
    //sigfillset(&sa.sa_mask);
	memset(BackGroundPID,0,sizeof(BackGroundPID));
    if(sigaction(SIGINT,&sa,NULL)==-1){ //Calling signal handlers
    	perror("Error message should be generated");
    	
    }

    if(sigaction(SIGQUIT,&sa,NULL)==-1){
    	perror("Error message should be generated");
    }

    if(sigaction(SIGCHLD,&sa,NULL)==-1){
    	perror("Error message should be generated");
    }

	char input[MAXLINE];
	char** tokens;
	
	while(1) { 
		printf("$ "); // The prompt
		fflush(stdin);

		char *in = fgets(input, MAXLINE, stdin); // Taking input one line at a time
		//Checking for EOF
		if (in == NULL){
			if (DEBUG) printf("jash: EOF found. Program will exit.\n");
			break ;
		}

		// Calling the tokenizer function on the input line    
		tokens = tokenize(input);	
		// Executing the command parsed by the tokenizer
		int status;
		status = execute_command(tokens,0) ; 
		fflush(NULL);

		//printf("Return Status - %i \n", status);
		fflush(NULL);
		
		// Freeing the allocated memory	
		int i ;
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

		if(shouldExit)break;
	}
	

	/* Kill all Children Processes and Quit Parent Process */
	return 0 ;
}

/*The tokenizer function takes a string of chars and forms tokens out of it*/
char ** tokenize(char* input){
	int i, doubleQuotes = 0, tokenIndex = 0, tokenNo = 0;
	char *token = (char *)malloc(MAXLINE*sizeof(char));
	char **tokens;

	tokens = (char **) malloc(MAXLINE*sizeof(char*));

	for(i =0; i < strlen(input); i++){
		char readChar = input[i];

		if (readChar == '"'){
			doubleQuotes = (doubleQuotes + 1) % 2;
			if (doubleQuotes == 0){
				token[tokenIndex] = '\0';
				if (tokenIndex != 0){
					tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
					strcpy(tokens[tokenNo++], token);
					tokenIndex = 0; 
				}
			}
		} else if (doubleQuotes == 1){
			token[tokenIndex++] = readChar;
		} else if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		} else {
			token[tokenIndex++] = readChar;
		}
	}

	if (doubleQuotes == 1){
		token[tokenIndex] = '\0';
		if (tokenIndex != 0){
			tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
			strcpy(tokens[tokenNo++], token);
		}
	}

	tokens[tokenNo] = NULL ;
	return tokens;
}

//Executes a given command
int execute_command(char** tokens, int count) {
	
	int IOinput=0;
	int IOoutput=0;
	int IOappend=0;
	char* inFile;
	char* outFile;

	FILE* inputFD;
	FILE* outputFD;

	int isPipe=0;
	int i;
	for(i=0;tokens[i]!=NULL;i++) {
		if(!strcmp(tokens[i],"|")) isPipe=i;
	}


	int isBackGround=0;

	if(!(strcmp(tokens[i-1], "&"))) {
		isBackGround = 1;
		tokens[i-1] = NULL;
	}

	if(isBackGround){
		int pid=fork();
		if(pid == 0){
			fflush(NULL);
			int returnMsg = execute_command(tokens,0);
			fflush(NULL);
			exit(returnMsg);
		}
		else{
			BackGroundPID[BackGroundPIDCount]=pid;
			BackGroundPIDCount++;

			/*
			struct sigaction sa;
			sa.sa_handler = &signal_handler;
			sa.sa_flags = SA_RESTART;
		    //sigfillset(&sa.sa_mask);

		    if(sigaction(SIGCHLD,&sa,NULL)==-1){ //Calling signal handlers
		    	perror("Error message should be generated");	    	
		    }*/
			return 0;
		}

	}
	
	if(!count){
		int i ;
		int countL = 0, countG = 0, countA = 0;
		for(i=0;tokens[i]!=NULL;i++) {
			if(!strcmp(tokens[i],"<")) countL++;
			if(!strcmp(tokens[i],">")) countG++;
			if(!strcmp(tokens[i],">>")) countA++;
		}

		// Checking if it has more than 1 similar token like 2 >'s 
		if(countL > 1 || countG > 1 || countA > 1 || (countA >= 1 && countG >= 1)) {
			perror("IO: Error in IO redirection");
			return(-1);
		}

		//If the input format is correct, Some initial pre procesing
		for(i=0;tokens[i]!=NULL;){
			if(!strcmp(tokens[i],"<")) {
				IOinput = 1;
				tokens[i] = "\0";
				//free(tokens[i]);
				i++;
				inFile = tokens[i];
				tokens[i] = "\0";
				//free(tokens[i]);
			}
			if(!strcmp(tokens[i],">")) {
				IOoutput = 1;
				tokens[i] = "\0";
				//free(tokens[i]);
				i++;
				outFile = tokens[i];
				tokens[i] = "\0";
				//free(tokens[i]);
			}
			if(!strcmp(tokens[i],">>")) {
				IOappend = 1;
				tokens[i] = "\0";
				//free(tokens[i]);
				i++;
				outFile = tokens[i];
				tokens[i] = "\0";
				//free(tokens[i]);
			}
			i++;
		}
		for(i=0;tokens[i]!=NULL;i++){
			if(!strcmp(tokens[i],"\0")) {
				tokens[i] = NULL;
				free(tokens[i]);
			}
		}

		fflush(NULL);
		
		int saved_stdout,saved_stdin; //used to restore STDIN and STDOUT

		//If < , then redirection of STDIN
		if(IOinput){
			saved_stdin=dup(0);
			inputFD = fopen(inFile,"r");
			if(inputFD==NULL) {perror("Input file : File not found"); return 1;}
			dup2(fileno(inputFD),0);
		}

		//If >, then redirection of STDOUT
		if(IOoutput){
			saved_stdout=dup(1);
			outputFD = fopen(outFile,"w");
			dup2(fileno(outputFD),1);	
		}

		//If >>, then redirection of STDOUT 
		if(IOappend){
			//printf("%s\n","YO");
			saved_stdout=dup(1);
			outputFD = fopen(outFile,"a");
			dup2(fileno(outputFD),1);	
		}
		
		//Executing command after redirection if Input and output as necessary
		int temp = execute_command(tokens,1);	
		

		//Restoring STDIN and STDOUT
		if(IOinput){dup2(saved_stdin,0);close(fileno(inputFD));}
		if(IOoutput || IOappend){dup2(saved_stdout,1);close(fileno(outputFD));}
		
		return temp;		
	}


	if (tokens == NULL) {
		return -1 ; 				// Null Command
	} 

	else if (tokens[0] == NULL) {
		return 0 ;					// Empty Command
	} 

   else if(isPipe) {
    	
		tokens[isPipe] = NULL;
		/*
		int i;
		for(i=0; tokens[i]!=NULL; i++) {
			printf("%s", tokens[i]);
		}
		i++;
		for(; tokens[i]!=NULL; i++) {
			printf("%s", tokens[i]);
		}
		*/
		
		int fd[2];// Create file desciptors and call pipe
		if (pipe(fd) == -1) {
		   perror("pipe");
		   exit(-1);
		}

		pid_t p1 = fork(); // First fork
		if (p1 == -1) {
		   perror("fork");
		   exit(-1);
		}
		pid_t p2;
		if(p1 == 0) {// First child - the one whose output is piped  
			if(close(fd[0]) == -1) {
				perror("file descriptor cant be closed");
				exit(-1);
			}
			dup2(fd[1],1); // use dup to redirect fd[1] to 1
			int status;
			status = execute_command(tokens, 0);
			if(close(fd[1]) == -1) {
				perror("file descriptor cant be closed");
				exit(-1);
			}
			
			exit(status);
		}
		else if((p2=fork())==0) {// Second child - the one whose input is piped 
			if(close(fd[1]) == -1) {
				perror("file descriptor cant be closed");
				exit(-1);
			}			
			dup2(fd[0],0);// use dup to redirect fd[0] to 0
			int status;
			status = execute_command(&tokens[isPipe+1], 0);
			if(close(fd[0]) == -1) {
				perror("file descriptor cant be closed");
				exit(-1);
			}
			
			exit(status);
		}
		else {
            // Close fd's as we have no use here
			if(close(fd[0]) == -1) {
				perror("file descriptor cant be closed");
				exit(-1);
			}
			if(close(fd[1]) == -1) {
				perror("file descriptor cant be closed");
				exit(-1);
			}
			int status1, status2;
            // Wait for childs to end
			wait(&status1);
			wait(&status2);
			if(WIFEXITED(status1)) { // Get the exit status
				status1 = WEXITSTATUS(status1);
			}
			if(WIFEXITED(status2)) { // Get the exit status
				status2 = WEXITSTATUS(status2);
			}
			if(status1!=0 || status2!=0) return -1;
			return 0;
		}
	}


    //If command is exit, quitting the parent also
	else if (!strcmp(tokens[0],"exit")) { 
		shouldExit = 1; //This is checked in main function and thus program is aborted
		return 0 ;
	} 
	
    //Changind directory, returns -1 if error
	else if (!strcmp(tokens[0],"cd")) { 
		char* pathValue = tokens[1];
		if(!chdir(pathValue)) {return 0;}
		else{
			//char* arrayTemp = "Directory not found";
			//fprintf(stderr, "Directory not found\n");
			perror("cd: Error ");
			fflush(NULL);
			return -1 ;
		}
	} 
	
	else if(!strcmp(tokens[0],"cron")) {
		FILE* fp = fopen(tokens[1], "r");// Get the file pointer
		if(fp==NULL) { // case when such file is there
			perror("Run: Error ");
			fflush(NULL);
			exit(1);
		}
		int AllHours[1000];
		int AllMinutes[1000];
		int CronCount = 0;
		char** AllTokens[1000];
		while(1)
		{
			char line[60];
			char* err;
			err = fgets(line, 60, fp); // Read the next line
			if(err==NULL) {
				printf("Run: EOF has reached.\n");
				fflush(NULL);
			}
			if(feof(fp))//EOF
			{ 
				break;
			}

			printf("%s", line);

			char** tokens = tokenize(line);

			int hours;
			hours = atoi(tokens[0]);
			if(!strcmp(tokens[0],"*")) hours = -1;

			int minutes;
			minutes = atoi(tokens[1]);
			if(!strcmp(tokens[1],"*")) minutes = -1;

			printf("%i, %i\n", hours, minutes);

			AllHours[CronCount] = hours;
			AllMinutes[CronCount] = minutes;
			AllTokens[CronCount] = &tokens[2];
			CronCount++;
			fflush(NULL);
		}


		return 0;
	}
    //Parallel command - Each command is executed by a new child
	else if (!strcmp(tokens[0],"parallel")) { 
		countPid=0; //No. of childrean
		isParallel=1;
		int i;
		char* buf[1000];
		memset(buf, 0, sizeof(buf));
		i = 1;
		int curr = 0;
		while(1) {
			char* token = tokens[i];
			//printf("%s \n", token);
			if((token == NULL) || !(strcmp(token,":::"))) {
				buf[curr] = NULL;
				int pos = 0;
				while(buf[pos]!=NULL) {
					//printf("%s ", buf[pos]);
					fflush(NULL);
					pos++;
				}    //Extracted the command
				
				int pidTemp=fork();
				if(pidTemp!=0){
					pidVals[countPid]=pidTemp; //Adding pid to array of pids to keep track of all children
					countPid++;
				}
				else{

					int status = execute_command(buf,0); //Calling function in child to execute it
					fflush(NULL);
					exit(status);
					//kill(getpid(),SIGKILL);
				}


				memset(buf, 0, sizeof(buf));//reset buf, curr
				curr = 0;
				i++;
				if(token == NULL) break;
				continue; 
			}
			else {
				buf[curr] = token;
				curr++;
				i++;
				continue;
			}
		}

		int iTemp=0;
		int returnVal=1;
		for(iTemp=0;iTemp<countPid;iTemp++){
			int status;
			wait(&status);
			if(WIFEXITED(status)) { // get the status of returned child process
				status = WEXITSTATUS(status);
			}
			//printf("Parallel: A child exited with status: %d \n", status);
			fflush(NULL);
			returnVal=returnVal&status;	
		}

		countPid=0;
		return returnVal; //Parallel is succesful when atleast one of its child is successful
	} 
    
    //Commands are executed sequentially and sequential is aborted whenever a command fails
	else if (!strcmp(tokens[0],"sequential")) {
		/* Analyze the command to get the jobs */
		/* Run jobs sequentially, print error on failure */
		/* Stop on failure or if all jobs are completed */
		
		int i;
		char* buf[1000];
		memset(buf, 0, sizeof(buf));
		i = 1;
		int curr = 0;
		while(1) {
			char* token = tokens[i];// tokenizing
			
			if((token == NULL) || !(strcmp(token,":::"))) {//case when we have found a 'command'
				buf[curr] = NULL;
				int pos = 0;
				while(buf[pos]!=NULL) {
					printf("%s ", buf[pos]);
					fflush(NULL);
					pos++;
				}
				printf("\n");
				fflush(NULL);
				// In buf array we have the command to be executed
				int status;
				status = execute_command(buf,0);
				fflush(NULL);
				if(status != 0) {// Status is zero implies process has ran correctly
					printf("sequential: Error Occured in %s, status is %i \n", buf[0], status);
					fflush(NULL);
					return -1;// Return -1 because one of the processes has ran incorrectly
					break;// Break immediately
				}

				memset(buf, 0, sizeof(buf));//reset buf, curr
				curr = 0;
				i++;
				if(token == NULL) break;
				continue; 
			}
			else {
				buf[curr] = token;
				curr++;
				i++;
				continue;
			}
		}

		return 0 ;// Return value accordingly
	} 
    /* Sequential-Or : Will stop if a file runs successfully
    Will return error if one of the prog runs successfully*/
	else if (!strcmp(tokens[0],"sequential_or")) {
		/* Analyze the command to get the jobs */
		/* Run jobs sequentially, print error on failure */
		/* Stop on failure or if all jobs are completed */
		
		int i;
		char* buf[1000];
		memset(buf, 0, sizeof(buf));
		i = 1;
		int curr = 0;
		while(1) {
			char* token = tokens[i];//tokenizing
            
			if((token == NULL) || !(strcmp(token,":::"))) {//case when we have found a 'command'
				buf[curr] = NULL;
				int pos = 0;
				while(buf[pos]!=NULL) {
					printf("%s ", buf[pos]);
					fflush(NULL);
					pos++;
				}
				printf("\n");
				fflush(NULL);
				// In buf array we have the command to be executed
				int status;
				status = execute_command(buf,0);
				fflush(NULL);
				if(status == 0) {// Status is zero implies process has ran correctly
					printf("sequential_or: Error has not occured in %s. Returning \n", buf[0]);
					fflush(NULL);
					return -1;// Return -1 since process has run correctly
					break;
				}

				memset(buf, 0, sizeof(buf));//reset buf, curr
				curr = 0;
				i++;
				if(token == NULL) break;
				continue; 
			}
			else {
				buf[curr] = token;
				curr++;
				i++;
				continue;
			}
		}

		return 0 ;// Return value accordingly
	}   
	else {
		/* Either file is to be executed or batch file to be run */
		/* Child process creation (needed for both cases) */
		int pid = fork() ;
		current_child_pid = pid;
		if (pid == 0) {
			if (!strcmp(tokens[0],"run")) {
                /* Batch file is to be executed */
				FILE* fp = fopen(tokens[1], "r");// Get the file pointer
				if(fp==NULL) { // case when such file is there
					perror("Run: Error ");
					fflush(NULL);
					exit(1);
				}
				while(1)
				{
					char line[60];
					char* err;
					err = fgets(line, 60, fp); // Read the next line
					if(err==NULL) {
						printf("Run: EOF has reached.\n");
						fflush(NULL);
					}
					if(feof(fp))//EOF
					{ 
						break;
					}

					printf("%s", line);
					fflush(NULL);
                    
					int status; // Execute command using execute_command
                                // Get the tokens using tokenize
					status = execute_command(tokenize(line),0);
					fflush(NULL);
					if(status != 0) {
						printf("Run: Some error in %s", line);
						fflush(NULL);
					}
				}
				/* Locate file and run commands */
				/* May require recursive call to execute_command */
				/* Exit child with or without error */
				exit (0) ;
			}
			else {
				execvp(tokens[0], tokens);// Call execvp.
                fflush(NULL);
				// comes here only if there is some error
                perror("File Execution: Error ");
				fflush(NULL);
				exit(1) ;
			}
		}
		else {// Main program
			int status;
			if(pid == -1) {// Check if no error in fork
				fprintf(stderr, "Fork has failed.\n"); 
				fflush(NULL);
				exit(1);
			}
            // Wait for the child
			int pid1 = wait(&status);
			if(WIFEXITED(status)) { // Get the exit status
				status = WEXITSTATUS(status);
			}
			//printf("Main: Child %d exited with status: %d \n", pid1, status);
			fflush(NULL);
			return status;
		}
	}

	
	return 1 ;
}

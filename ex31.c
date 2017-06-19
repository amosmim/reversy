#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>


void WaitForTwoClients(pid_t* client1Pid, pid_t* client2Pid);
void SignalClient(pid_t* client);
void WaitForClientRead(int shareId);


#define FIN_EXIT 1
#define ERR_EXIT 0
#define SHARE_SIZE 1024
#define PID_BUFFER_SIZE 10
#define FIFO_NAME "fifo_clientTOserver"


void WaitForTwoClients(pid_t* client1Pid, pid_t* client2Pid){
	int fd_read;
	char buffer1[PID_BUFFER_SIZE],buffer2[PID_BUFFER_SIZE];


	// create FIFO for client_TO_server
	if(mkfifo(FIFO_NAME,O_RDONLY|0666) < 0){
		perror("Could not create a FIFO : server\n");
		exit(ERR_EXIT);
	}

	if((fd_read = open(FIFO_NAME,O_RDONLY)) < 0){
		perror("Error in open FIFO to read pids: server\n");
        unlink(FIFO_NAME);
		exit(ERR_EXIT);
	}


	// read first client pid
	if(read(fd_read,client1Pid,sizeof(client1Pid)) < 0){
		perror("Could not read first pid : server\n");
        unlink(FIFO_NAME);
		exit(ERR_EXIT);
	}


	*client2Pid = 0;
	// read second client pid
	while (*client2Pid == 0) {
		if (read(fd_read, client2Pid, sizeof(client2Pid)) < 0) {
			perror("Could not read second pid : server\n");
			unlink(FIFO_NAME);
			exit(ERR_EXIT);
		}
	}

	printf("received Two pids: %d and %d\n",*client1Pid,*client2Pid);

	if(close(fd_read) < 0){
        unlink(FIFO_NAME);
		perror("Error in close FIFO file descriptor : server");
		exit(ERR_EXIT);
	}
	// unlink FIFO for client_TO_server
	if(unlink(FIFO_NAME) < 0){
		perror("Could not delete fifo_clientTOserver : server\n");
		exit(ERR_EXIT);
	}

}

void SignalClient(pid_t* client){
	if(kill(*client,SIGUSR1) < 0){
		perror("Could not trigger client to play : server");
		exit(ERR_EXIT);
	}
}

int readMove(char **sh_pointer){

	while(**sh_pointer == '\0'){}
	switch (**sh_pointer){
		case 'b':
		case 'w':
			*sh_pointer +=3;
			return 1;
		case 'W':
		case 'B':
		case 'T':
			return 0;
		default:
			perror("error in value of read_move\n");
			printf("error_0 : %c\n", **sh_pointer);
	}

}

void WaitForClientRead(int shareId){
	char *sharestr, *p;
	if((char*)-1 == (sharestr=(char *)shmat ( shareId ,0 ,0 ))){
		perror("error in shmat : server\n");
		exit(ERR_EXIT);
	}
    p= sharestr;
	printf("first client write: ");
	putchar(*p++);
	putchar(*p++);
	putchar(*p++);
	putchar('\n');

	//putchar(*sharestr);
	shmdt ( sharestr );
}

int main(int argc, char** argv){
	int shareId;
	key_t key;
	pid_t client1Pid, client2Pid;
	char *sharestr, *shr_p;
	int stat;

	// get pids of 2 clients
	WaitForTwoClients(&client1Pid, &client2Pid);

	// generate a key for FIFO
	if((key = ftok("ex31.c",'k')) == -1){
		perror("Could not generate a key using ftok() : server\n");
		exit(ERR_EXIT);
	}
	printf("the key is %d\n", key);
	// generate shared memory from key
	if((shareId = shmget(key,SHARE_SIZE,IPC_CREAT | 0666)) == -1){
		perror("Could not generate shared memory from key : server\n");
		exit(ERR_EXIT);
	}

	if((char*)-1 == (sharestr=(char *)shmat ( shareId ,0 ,0 ))){
		perror("error in shmat : server\n");
		exit(ERR_EXIT);
	}
	shr_p = sharestr;
	// signal for the first client to start move.
	SignalClient(&client1Pid);

	printf("Shared memory id: %d\n",shareId);

	// wait for the first client move.
	stat =  readMove(&shr_p);
	printf("first move play\n");


	// signal second client to move
	SignalClient(&client2Pid);

	// loop of moves
	while (readMove(&shr_p)==1){
		printf("move play\n");
	}
	switch (*shr_p){
		case 'T':
			printf("No Player win\n");
			break;
		case 'W':
			printf("white player win\n");
            break;
		case 'B':
			printf("black player win\n");
            break;
		default:
			perror("error in value of shr_p : server\n");
	}


	//delete share memory
	if (shmctl(shareId, IPC_RMID, NULL) == -1) {
		perror("shmctl - delete error : server\n");
		exit(ERR_EXIT);
	}


	return FIN_EXIT;
}

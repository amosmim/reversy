//
// Created by amos on 6/8/17.
//
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>

#define BLACK_MARK 2
#define WHITE_MARK 1
#define BOARD_SIZE 8

#define FIN_EXIT 1
#define ERR_EXIT 0
#define SHARE_SIZE 1024
#define PID_BUFFER_SIZE 10
#define FIFO_NAME "fifo_clientTOserver"

char color ,*shm_pointer , *sharestr;
int board[BOARD_SIZE][BOARD_SIZE];
void initBoard();
void printBoard();
void sigusr_handler(int sigNum);
int getMove();
char checkEndGame();
int checkMove(char p_color,int x, int y, int flipFlag);
void flipCoins(int my_color_num, int x_start, int x_end,int y_start, int y_end, int x_step, int y_step);
int check_direction(char player_color, int x_start, int x_end,
                    int y_start, int y_end, int x_step, int y_step, int flipFlag);
int readMove();

/**
 * initilaze board
 */
void initBoard(){
    int x,y;
    for(x=0; x < BOARD_SIZE; ++x){
        for(y=0; y < BOARD_SIZE; ++y){
            board[x][y] = 0;
        }
    }
    board[BOARD_SIZE/2 -1][BOARD_SIZE/2 -1] = BLACK_MARK;
    board[BOARD_SIZE/2][BOARD_SIZE/2] = BLACK_MARK;
    board[BOARD_SIZE/2 -1][BOARD_SIZE/2] = WHITE_MARK;
    board[BOARD_SIZE/2][BOARD_SIZE/2 -1] = WHITE_MARK;
}

void printBoard(){
    int x,y;
    printf("The board is:\n");
    for(y=0; y < BOARD_SIZE; ++y){
        for(x=0; x < BOARD_SIZE; ++x){
            putchar(board[x][y] + '0');
        }
        putchar('\n');
    }
}

/**
 * return 1 if the game is ended
 * @return
 */
char checkEndGame(){
    int x,y;
    int whites = 0 , blacks = 0;

    for(y=0; y < BOARD_SIZE; ++y){
        for(x=0; x < BOARD_SIZE; ++x){
            switch (board[x][y]){
                case 0 :
                    if(checkMove(color,x,y,0) == 1){
                        return 0; // there is a legal move!
                    }
                    break;
                case WHITE_MARK:
                    whites++;
                    break;
                case BLACK_MARK:
                    blacks++;
                    break;
                default:
                    perror("error in value in board\n");
                    printf("have value of %d\n",board[x][y]);
                    break;
            }

        }
    }
    if (whites > blacks){
        printf("white player win\n");
        return 'W';
    } else if (blacks > whites){
        printf("black player win\n");
        return 'B';
    }
    printf("No Player win\n");
    return 'T';
}

int checkMove(char p_color, int x, int y, int flipFlag){
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE){ 
        return 0;
    }
    int bool = 0;
    // check if the place is empty
    if (board[x][y] != 0){
        return 0;
    }
    // try to find at list one valid direction and flip it to my color
    bool = check_direction(p_color, x, BOARD_SIZE-1,y, BOARD_SIZE-1,  1 , 1, flipFlag) || bool;
    bool = check_direction(p_color, x, 0,           y, 0           , -1 ,-1, flipFlag) || bool; // bug
    bool = check_direction(p_color, x, 0,           y, BOARD_SIZE-1, -1 , 1, flipFlag) || bool;
    bool = check_direction(p_color, x, BOARD_SIZE-1,y, 0           ,  1 ,-1, flipFlag) || bool;
    bool = check_direction(p_color, x, x,           y, 0           ,  0 ,-1, flipFlag) || bool;
    bool = check_direction(p_color, x, x,           y, BOARD_SIZE-1,  0 , 1, flipFlag) || bool;
    bool = check_direction(p_color, x, 0,           y, y           , -1 , 0, flipFlag) || bool;
    bool = check_direction(p_color, x, BOARD_SIZE-1,y, y           ,  1 , 0, flipFlag) || bool;

    // return 0 if no direction is valid, or 1 if some is valid
    return bool;

}
/**
 * change the colors of the coins in direction
 * @param my_color_num
 * @param x_start
 * @param x_end
 * @param y_start
 * @param y_end
 * @param x_step
 * @param y_step
 */
void flipCoins(int my_color_num, int x_start, int x_end,int y_start, int y_end, int x_step, int y_step){
    int x = x_start;
    int y = y_start;
    while ((x != x_end) || (y != y_end)){
        board[x][y] = my_color_num;
        x += x_step;
        y += y_step;
    }
}

/**
 * check if direction is invalid and return 0, or flip the coin if valid
 * @param x_start
 * @param x_end
 * @param y_start
 * @param y_end
 * @param x_step
 * @param y_step
 * @param flipFlag - boolean: flip this direction if this valid?
 * @return 0 = invalid, 1 = valid
 */
int check_direction(char player_color, int x_start,
                    int x_end,int y_start, int y_end, int x_step, int y_step, int flipFlag){

    int riv_color_num,my_color_num, x = x_start,y = y_start;
    if (player_color == 'w'){
        my_color_num = WHITE_MARK;
        riv_color_num = BLACK_MARK;
    } else {
        my_color_num =BLACK_MARK;
        riv_color_num = WHITE_MARK;
    }
    if((x != x_end) || (y != y_end)){
        x += x_step;
        y += y_step;
        // check if the first coin is the rivel's coin
        if (board[x][y] == riv_color_num){
            // run until rival's coins ended or get to the end point.
            while((board[x][y] == riv_color_num) && ((x != x_end) || (y != y_end))) {
                x += x_step;
                y += y_step;
            }

            // check if the last coin we stop on is my coin.
            if ((x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) && (board[x][y] == my_color_num)){
                if (flipFlag) {
                    flipCoins(my_color_num, x_start, x, y_start, y, x_step, y_step);
                }
                return 1; // valid direction!
            }
        }
    }
    return 0; // invalid direction!

}

/**
 * get move from user -
 */
int getMove(){
    int x,y, invalid = 0;
    char endCheckOutput;
    // check if there is legal move for me before give the user to move
    if((endCheckOutput = checkEndGame())==0) {
        do {
            printf("Please choose a square\n");
            scanf("[%d,%d]", &x, &y);
            getchar(); // skip enter
            if ((invalid = checkMove(color, x, y, 1)) == 0) {
                printf("This square is invalid\nPlease choose another square\n");

            }
        } while (invalid == 0);


        printBoard();


        // write the move in share memory
        *(shm_pointer++) = color;
        *(shm_pointer++) = (char) ('0' + x);
        *(shm_pointer++) = (char) ('0' + y);
        *(shm_pointer) = '\0';
        return 1;
    } else {
        // when there isn't legal move - tell every process the result
        *(shm_pointer++) = endCheckOutput;
        *(shm_pointer) = '\0';
        return 0;
    }

}

/**
 * read move of rival player.
 * @return 0 if the game is ended, 1 if not.
 */
int readMove(){
    int  x, y;
    char riv_char = 'w';
    if (color == 'w'){
        riv_char = 'b';
    }
    // wait for write

    while(*shm_pointer == '\0'){
        printf("‫‪Waiting‬‬ ‫‪for‬‬ ‫‪the‬‬ ‫‪other‬‬ ‫‪player‬‬ ‫‪to‬‬ ‫‪make‬‬ ‫‪a‬‬ ‫‪move\n‬‬");
        sleep(1);
    }
    // check the move
    if (*shm_pointer == riv_char){
        shm_pointer++;
        x = *(shm_pointer++) - '0';
        y = *(shm_pointer++) - '0';
        checkMove(riv_char, x, y ,1);
        printBoard();
        return 1;
    } else {
        printf("no rival char enter, gets: %c\n",*shm_pointer); // TODO -DELETE
        switch (*shm_pointer) {
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
                perror("error in value of shm_pointer : client\n");
                printf("gets from shm_pointer: %c", *shm_pointer);
        }
        return 0;
    }



}


/**
 * handler for SIGUSR1
 * @param sigNum == SIGUSR1
 */
void sigusr_handler(int sigNum){
    int shareId, key;
    struct shmid_ds buf;
    char buffer[50];
    // generate a key for share memory
    if((key = ftok("ex31.c",'k')) == -1){
        perror("Could not generate a key using ftok() : client\n");
        exit(ERR_EXIT);
    }

    printf("the key is %d\n", key);
    // get shared memory from key
    if((shareId = shmget(key,SHARE_SIZE,IPC_CREAT | 0666)) == -1){
        perror("Could not generate shared memory from key : client\n");
        exit(ERR_EXIT);
    }

    // point to share memory
    if((char*)-1 == (sharestr =(char *)shmat ( shareId ,0 ,0 ))){
        perror("error in shmat : server\n");
        exit(ERR_EXIT);
    }
    shm_pointer = sharestr;
    if (-1 ==  shmctl(shareId, IPC_STAT, &buf)){
        perror("can't get struct shmid_ds : client\n");
        printf("error no.: %d",errno);
        exit(ERR_EXIT);
    }
    switch (buf.shm_nattch){
        case 2: // first client
            color = 'b';
            break;
        case 3: // second client
            color = 'w';
            readMove();
            break;
        default:
            sprintf(buffer,"error in value of shm_nattch , gets %lu : client\n", buf.shm_nattch );
            perror(buffer);
    }
    // point to the start of the share memory

    // gets legal move from client
    getMove();
}

/**
 * send pid to server via FIFO (open & close FIFO)
 */
void sendPid(){
    char pidBuff[PID_BUFFER_SIZE];
    int fd_write;
    pid_t  my_pid = getpid();

    // open FIFO
    if((fd_write = open(FIFO_NAME,O_WRONLY)) < 0){
        perror("Error in open FIFO to write pid: client\n");
        exit(ERR_EXIT);
    }
    printf("start client on %d pid",my_pid);

    if(write(fd_write,&my_pid,sizeof(pid_t)) < 0){
        perror("Could not read first pid : client\n");
        exit(ERR_EXIT);
    }
    printf("write my pid (%d) to the FIFO\n", my_pid);

    // FIFO close
    if(close(fd_write) < 0){
        perror("Could not close FIFO file descriptor : client\n");
        exit(ERR_EXIT);
    }
}

int main(int argc, char* argv[]){
    int end_game = 0;
    struct sigaction intAct, intAct1;
    sigset_t block_mask, block_mask1;
    sigfillset (&block_mask);
    intAct.sa_mask = block_mask;
    intAct.sa_handler = sigusr_handler;
    if( sigaction(SIGUSR1, &intAct, NULL) != 0 ) {
        perror( "sigaction() error SIGUSR1\n" );
        exit(-1);
    }
    initBoard();
    printBoard(); // TODO - delete this!
    // send pid to server via FIFO (open & close FIFO)
    sendPid();

    // wait for SIGUSR1
    pause();
    while (readMove() && getMove());



    // detach from the segment
    if(0 > shmdt ( sharestr )){
        perror("failed to detach from shared memory");
    }
    printf("exiting\n");


}


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#define M 100000002
int t_num;
pthread_t tids[200];
int row;
int col;
int epoch;
char** board;
char** newboard;
char** board_trans;
int* stat;
int* up_to_date;
bool transposed=0;
pthread_mutex_t block=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bready=PTHREAD_COND_INITIALIZER;
pthread_mutex_t block1=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bready1=PTHREAD_COND_INITIALIZER;
int max(int a,int b){
    if (a>b) return a;
    else return b;
}

int live_cell_count(int i,int j){
    int count=0;
    if (i-1>=0){
        if (j-1>=0) count+=((int)board[i-1][j-1]=='O');
        count+=((int)board[i-1][j]=='O');
        if (j+1<col) count+=((int)board[i-1][j+1]=='O');
    }
    if (j-1>=0) count+=((int)board[i][j-1]=='O');
    if (j+1<col) count+=((int)board[i][j+1]=='O');
    if (i+1<row){
        if (j-1>=0) count+=((int)board[i+1][j-1]=='O');
        count+=((int)board[i+1][j]=='O');
        if (j+1<col) count+=((int)board[i+1][j+1]=='O');
    }
    return count;
}

void* get_final_stat(void* arg){
    //dup2(1,2);
    int tid=(int) arg;
    //printf("thread: %d\n",tid);
    for (int i=0;i<epoch;i++){
        int t=tid;
        while (t<row){
            if (t_num>1){
                pthread_mutex_lock(&block1);
                if (t==0){
                    //printf("%d %d\n",t,up_to_date[t+1]);
                    while (up_to_date[t+1]<i){
                        pthread_cond_wait(&bready1,&block1);
                    }
                }
                else if (t==row-1){
                    //printf("%d %d\n",t,up_to_date[t-1]);
                    while (up_to_date[t-1]<i){
                        pthread_cond_wait(&bready1,&block1);
                    }
                }
                else{
                    //printf("%d %d %d\n",t,up_to_date[t-1],up_to_date[t+1]);
                    while (up_to_date[t-1]<i || up_to_date[t+1]<i){
                        pthread_cond_wait(&bready1,&block1);
                    }
                }
                pthread_mutex_unlock(&block1);
            }
            for (int j=0;j<col;j++){
                //printf("row: %d; col: %d; epoch: %d; count: %d\n",t,j,i,live_cell_count(t,j));
                if (board[t][j]=='.'){
                    if (live_cell_count(t,j)==3) newboard[t][j]='O';
                    else newboard[t][j]='.';
                }
                else{
                    int l=live_cell_count(t,j);
                    if (l==2 || l==3) newboard[t][j]='O';
                    else newboard[t][j]='.'; 
                }   
            }
            //printf("row: %d epoch: %d\n%s\n",t,i,newboard[t]);
            stat[t]+=1;
            if (t_num>1){
                pthread_cond_broadcast(&bready);
                pthread_mutex_lock(&block);
                if (t==0){
                    while (stat[t+1]<=i){
                        pthread_cond_wait(&bready,&block);
                    }
                }
                else if (t==row-1){
                    while (stat[t-1]<=i){
                        pthread_cond_wait(&bready,&block);
                    }
                }
                else{
                    while (stat[t-1]<=i || stat[t+1]<=i){
                        pthread_cond_wait(&bready,&block);
                    }
                }
            }
            char* discarded=board[t];
            board[t]=newboard[t];
            free(discarded);
            if(t_num>1) pthread_mutex_unlock(&block);
            up_to_date[t]+=1;
            //printf("update row %d (epoch: %d)\n",t,i);
            if(t_num>1) pthread_cond_broadcast(&bready1);
            newboard[t]=(char*)malloc(col*sizeof(char));
            t+=t_num;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    //dup2(1,2);
    t_num=atoi(argv[2]);
    //fprintf(stderr,"%d\n",t_num);
    int fd1=open(argv[3],O_RDONLY);
    dup2(fd1,0);
    scanf("%d %d %d",&row,&col,&epoch);
    //fprintf(stderr,"%d %d %d\n",row,col,epoch);
    if (col>row){
        transposed=1;
    }
    board=(char**)malloc((row)*sizeof(char*));
    if (transposed==0) newboard=(char**)malloc((row)*sizeof(char*));
    stat=(int*)malloc(max(row,col)*sizeof(int));
    up_to_date=(int*)malloc(max(row,col)*sizeof(int));
    for (int i=0;i<max(row,col);i++){
        stat[i]=0;
        up_to_date[i]=0;
    }
    //printf("%d %d\n",row,col);
    for (int i=0;i<row;i++){
        board[i]=(char*)malloc((col)*sizeof(char));
        if (transposed==0) newboard[i]=(char*)malloc((col)*sizeof(char));
        scanf("%s",board[i]);
    }
    if (transposed==1){
        board_trans=(char**)malloc(col*sizeof(char*));
        newboard=(char**)malloc((col)*sizeof(char*));
        for (int i=0;i<col;i++){
            board_trans[i]=(char*)malloc(row*sizeof(char));
            newboard[i]=(char*)malloc(row*sizeof(char));
        }
        for (int i=0;i<col;i++){
            for (int j=0;j<row;j++) board_trans[i][j]=board[j][i];
        }
        int tmp=row;
        row=col;
        col=tmp;
        //for (int i=0;i<row;i++) printf("%s\n",board_trans[i]);
        board=board_trans;
    }
    //for (int i=0;i<row;i++) fprintf(stderr,"%s\n",board[i]);
    //printf("%d %d\n",row,col);
    for (int i=0;i<t_num;i++){
        pthread_create(&(tids[i]),NULL,get_final_stat,(void*)i);
    }
    for (int i=0;i<t_num;i++){
        pthread_join(tids[i],NULL);
    }
    //printf("%d %d\n",row,col);
    int fd2=open(argv[4],O_RDWR);
    dup2(fd2,1);
    if (transposed==0){
        for (int i=0;i<row;i++){
            if (i<row-1) printf("%s\n",board[i]);
            else printf("%s",board[i]);
        }
    }
    else{
        for (int j=0;j<col;j++){
            for (int i=0;i<row;i++){
                printf("%c",board[i][j]);
            }
            if (j!=col-1) printf("\n");
        }
    }
}
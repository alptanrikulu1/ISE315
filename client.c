#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define PERMISSIONS 0640
#define MSGSTR_LEN 256
#define USRNAME_LEN 10
#define EXIT_STR "q"

typedef struct data_st{
  long source;
  long dest; 
  char msgstr[MSGSTR_LEN];
} data_st;

typedef struct msgbuf_st {
   long mtype;
   data_st data;
} msgbuf;

/*
Problems
- Some uppercase letters have been seen weird from Server
- UserNames did not get from user.
- I have defined message buffer as 255 character.Otherwise, I get error
when it defined as 180 chars.

Works fine
- When all clients log off server shuts down!
- All users create their FIFO pipe structure properly.
- Server gets message properly.
- key variable is defined as student number
*/

/*
To Compile
gcc server.c -o server.o  -lpthread
gcc client.c -o client.o  -lpthread

./server.o
./client.o

Important!!!
Firstly, you should start a server instance. Then you can start multiple client instances.
You must start client instances as ./client 1 ,  ./client 2

In this case ./client 1 becomes client1 - ./client 2 becomes client2
And so on ...

*/



void receive_message(int msgqid, msgbuf * msgp, long mtype);
void send_message(char message[MSGSTR_LEN], int msgqid, long to, long from, long to_client);
void send_message(char message[MSGSTR_LEN], int msgqid, long to, long from, long to_client);
void * receive_thread(void * arg);
void start_thread(pthread_t * thread);
void create_thread(pthread_t * thread, void * vars, int thread_type);

int main(int argc, char * argv[]) {
  pthread_t threads[2];
  int targs[2];
  int i, ret, ret2;
  int qID;
  int key;
  int client_key;
  int tmp[3];
  char * input;
  if(argc == 2) {
    key = 150130212;
    client_key = atoi(argv[1]);
    if(!key || !client_key) {
      printf("Invalid arguments.\n");
      exit(-1);
    }
    qID = msgget(key, 0);
  }
  else {
    printf("Invalid arguments.\n");
    exit(-1);
  }

  if(qID < 0) {
    printf("Queue creation failed\n");
    exit(-1);
  }

  tmp[0] = qID;
  tmp[1] = key;
  tmp[2] = client_key;

  create_thread(&(threads[0]), tmp, 1);
  create_thread(&(threads[1]), tmp, 2);

  start_thread(&(threads[0]));
  start_thread(&(threads[1]));
  return 0;
}


void receive_message(int msgqid, msgbuf * msgp, long mtype) {
  int bytesRead = msgrcv(msgqid, msgp, sizeof(struct data_st), mtype, 0);
  if (bytesRead == -1) {
    if (errno == EIDRM) {
      fprintf(stderr, "Message queue removed while waiting!\n");
      exit(0);
    }
  }
}

void send_message(char message[MSGSTR_LEN], int msgqid, long to, long from, long to_client){
  msgbuf new_msg;
  new_msg.mtype = to;
  data_st ds;
  ds.source = from;
  ds.dest = to_client; 
  char * null = "\0";
  int length = strlen(message);
  if(MSGSTR_LEN < length) length = MSGSTR_LEN;
  int i;
  for(i = 0; i < length; i++) {
    strncpy(ds.msgstr, &(message[i]), 1);
    new_msg.data = ds;
    int ret = msgsnd(msgqid, (void *) &new_msg, sizeof(data_st), 0);
    if (ret == -1) {
      perror("Error!");exit(EXIT_FAILURE);  
    }
  }
  strncpy(ds.msgstr, null, 1);
  new_msg.data = ds;

  int ret = msgsnd(msgqid, (void *) &new_msg, sizeof(data_st), 0);
  if (ret == -1) {
    perror("Error!");exit(EXIT_FAILURE);
  }
}

void * send_thread(void * arg) {
  int * qID = arg;
  int * key = arg+sizeof(int);
  int * client_key = arg+sizeof(int)*2;
  int other_client_key;
  char buffer[MSGSTR_LEN];

  printf("You are now connected as client %d\n%s", *client_key, 
  	">");

  while(fgets(buffer, MSGSTR_LEN, stdin)) {
    if (buffer[strlen(buffer) - 1] == '\n') {
      buffer[strlen(buffer) - 1] = '\0';
    }

    char* input = buffer;
    if(strcmp(EXIT_STR, input) == 0) {
      send_message(EXIT_STR, *qID, *key, *client_key, *key);
      exit(0);
    }
    char* space = " ";
    int start = 0;
    int pos;
    char numberbuff[255];
    char messagebuff[255];
    pos = strcspn(input, space);
    if(pos) {
      other_client_key = 111111111;
      if(other_client_key) {
        char * message = buffer;
        printf("Sending \"%s\"\n", message);
        send_message(message, *qID, *key, *client_key, other_client_key);

        if(strcmp(message, EXIT_STR) == 0) exit(0); 
      }
      else printf(">\n");
    }
    else printf(">\n"); 
    strncpy(numberbuff, "", 255);
    strncpy(messagebuff, "", 255); 
  }

  int * myretp = malloc(sizeof(int));
  *myretp = (*qID);
  return myretp; 
}


void * receive_thread(void * arg) {
  int * qID = arg;
  int * key = arg+sizeof(int);
  int * client_key = arg+sizeof(int)*2;
  int to;
  int from;
  char message[MSGSTR_LEN];
  msgbuf tempbuf;
  tempbuf.mtype = *client_key;

  msgbuf messagebuf;
  while(1) {
    receive_message(*qID, &tempbuf, *client_key);
    to = tempbuf.data.dest;
    from = tempbuf.data.source;
    strncpy(message, tempbuf.data.msgstr, 1);
    
    messagebuf.data.dest = to;
    messagebuf.data.source = from;
    strcat(messagebuf.data.msgstr, message);

    
    if(strcmp(message, "\0") == 0) {
      printf("Client %ld: \"%s\"\n", messagebuf.data.source, messagebuf.data.msgstr);
      strncpy(messagebuf.data.msgstr, "", MSGSTR_LEN);
    }
  }

  int * myretp = malloc(sizeof(int));
  if (myretp == NULL) {
    perror("malloc error");
    pthread_exit(NULL);
  }
  *myretp = (*qID);
  return myretp;
}

void start_thread(pthread_t * thread) {
  void * thread_ret_ptr;
  int ret;
  ret = pthread_join(*thread, &thread_ret_ptr);
  if (ret == -1) { perror("Thread join error");
    exit(EXIT_FAILURE);
  }
  if (thread_ret_ptr != NULL) {
    int * intp = (int *) thread_ret_ptr;
    printf("Receive thread returned: %d\n", *intp);
    free(thread_ret_ptr);
  }
}

void create_thread(pthread_t * thread, void * vars, int thread_type) {
  int ret;
  if(thread_type == 1) pthread_create(thread, NULL, send_thread, vars);
  if(thread_type == 2) pthread_create(thread, NULL, receive_thread, vars);
}

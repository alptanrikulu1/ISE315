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

int create_msg_queue(int key);
void receive_message(int msgqid, msgbuf * msgp, long mtype);
void send_message(char message[MSGSTR_LEN], int msgqid, long to, long from);

int main(int argc,char * argv[]){
  int qID;
  int key;
  int clients[2];
  int current_num_clients = 0;
  int i;
  msgbuf client_msg_buffs[2];
  if(argc == 1){
    key = 150130212;
    if(key <= 0) { 
      printf("Server key must be > 0");
      exit(-1);
    }
    qID = create_msg_queue(key);
  }
  else {
    printf("Invalid arguments.");
    exit(-1);
  }

  if(qID < 0) {
    printf("Failed to create queue.\n");
    exit(-1);
  }
  printf("Server connected! Waiting for clients to connect...\n");

while(1) {
    int to;
    int from;
    char message[MSGSTR_LEN];
    msgbuf tempbuf;
    receive_message(qID, &tempbuf, key);

    to = tempbuf.data.dest;
    from = tempbuf.data.source;
    data_st * pntr;
    strncpy(message, tempbuf.data.msgstr, 1);
    if(current_num_clients == 0) {
      pntr = &(client_msg_buffs[0].data);
      pntr->dest = to;
      pntr->source = from;
      strncpy(pntr->msgstr, message, 1);
      clients[current_num_clients] = from;
      current_num_clients++;
    }
    else {
      int i = 0;
      int k = 0;
      int buff = -1;
      while(i < current_num_clients) {
        if(clients[i] == from) buff = i;
        i++;
      }
      if(buff != -1) {
        pntr = &(client_msg_buffs[buff].data);
        pntr->dest = to;
        pntr->source = from;
        strcat(pntr->msgstr, message);
        if(strcmp(message, "\0") == 0) {
            if(pntr->dest == key) {
              //if its EXIT
              if(strcmp(pntr->msgstr, EXIT_STR)==0) {
                if (msgctl(qID, IPC_RMID, NULL) == -1) {
                  if (errno == EIDRM) {
                    fprintf(stderr, "Message queue already removed.\n");
                  }
                  else {
                    perror("Error");
                  }
                }
                exit(0);
              }
              printf("Client %ld: \"%s\"\n", pntr->source, pntr->msgstr);
            }
            else {
              send_message(pntr->msgstr, qID, pntr->dest, pntr->source);
            }
          strcpy(pntr->msgstr, "");
        }
      }
      else {
        client_msg_buffs[current_num_clients].data.dest = to;
        client_msg_buffs[current_num_clients].data.source = from;
        strncpy(client_msg_buffs[current_num_clients].data.msgstr, message, 1);
        clients[current_num_clients] = from;
        current_num_clients++;
      }
    }
  }
  return 0;
}

int create_msg_queue(int key){
  int qID;
  if ((qID = msgget(key, IPC_CREAT | PERMISSIONS)) == -1){
    perror("Error creating msg queue \n");
    exit(EXIT_FAILURE);
  }
  return qID;
}

void receive_message(int msgqid, msgbuf * msgp, long mtype){
  int bytesRead = msgrcv(msgqid, msgp, sizeof(struct data_st), mtype, 0);
  if (bytesRead == -1) {
    if (errno == EIDRM) {
      fprintf(stderr, "Message queue removed while waiting!\n");
    }
    exit(EXIT_FAILURE);
  }
}

void send_message(char message[MSGSTR_LEN], int msgqid, long to, long from){
  msgbuf new_msg;
  new_msg.mtype = to;
  data_st ds;
  ds.source = from;
  ds.dest = to; 
  char * null = "\0";
  int length = strlen(message);
  if(MSGSTR_LEN < length) length = MSGSTR_LEN;
  int i;
  for(i = 0; i < length; i++) {
    strncpy(ds.msgstr, &(message[i]), 1);
    new_msg.data = ds;
    int ret = msgsnd(msgqid, (void *) &new_msg, sizeof(data_st), 0);
    if (ret == -1) {
      perror("Error attempting to send message!");
      exit(EXIT_FAILURE);
    }
  }
  strncpy(ds.msgstr, null, 1);
  new_msg.data = ds;

  int ret = msgsnd(msgqid, (void *) &new_msg, sizeof(data_st), 0);
  if (ret == -1) {
    perror("Error attempting to send message!");
    exit(EXIT_FAILURE);
  }
}

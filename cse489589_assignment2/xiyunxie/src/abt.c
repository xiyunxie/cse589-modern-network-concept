#include "../include/simulator.h"
#include <stdio.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
#define BUFFER_SIZE 1000
#define MSG_SIZE 20
#define TIMEOUT 10

#define A_WAITING_FOR_CALL_0 0
#define A_WAITING_FOR_ACK_0 1
#define A_WAITING_FOR_CALL_1 2
#define A_WAITING_FOR_ACK_1 3

#define B_WAITING_FOR_0 0
#define B_WAITING_FOR_1 1
int A = 0;
int B = 1;
int A_state = 0;
int B_state = 0;
struct msg_buffer {
  char message[20];
  struct msg_buffer *next;
  struct msg_buffer *prev;
};
struct msg_buffer *list_head;
struct msg_buffer *list_tail;
int ready_to_send = 1;

int A_goint_to_send = 0;
int B_goint_to_ACK = 0;
int A_waiting_ACK = 0;
struct pkt current_packet;

int next_seq(int current_seq);
void push_msg(struct msg *message);
struct msg_buffer* pop_msg();
int pkt_checksum(int seq,int ack,char* msg_ptr);
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  push_msg(message);
  //check A is in a send-state and what A is going to send is what B wants
  if((A_state == A_WAITING_FOR_CALL_0||A_state == A_WAITING_FOR_CALL_1)&&(A_goint_to_send == B_goint_to_ACK)){
    struct msg_buffer *poped_msg = pop_msg();
    if(poped_msg==NULL){
      printf("no more packets\n");
      return;
    }
    //make packet
    memset(&current_packet,'\0',sizeof(current_packet));
    memcpy(current_packet.payload,poped_msg->message,MSG_SIZE);
    current_packet.seqnum = A_goint_to_send;
    current_packet.acknum = A_waiting_ACK;
    int check_summ = pkt_checksum(current_packet.seqnum,current_packet.acknum,current_packet.payload);
    current_packet.checksum = check_summ;
    //send packet to layer3 and free massage
    tolayer3(A, current_packet);
    free(poped_msg);
    //A change state
    A_state = (A_state+1)%4;
    //start timer
    starttimer(A,TIMEOUT);
  }
  else{
    return;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  int check_summ = pkt_checksum(packet.seqnum,packet.acknum,packet.payload);
  //check checksum and if A is in waiting-state
  if(check_summ==packet.checksum&&A_waiting_ACK==packet.acknum&&(A_state == A_WAITING_FOR_ACK_0||A_state == A_WAITING_FOR_ACK_1)){
    //change state and A's waiting ack and what will be sent
    A_waiting_ACK = next_seq(A_waiting_ACK);
    A_goint_to_send = next_seq(A_goint_to_send);
    A_state = (A_state+1)%4;
  }
  else{
    return;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //if A is in waiting state
  if(A_state == A_WAITING_FOR_ACK_0||A_state == A_WAITING_FOR_ACK_1){
    tolayer3(A, current_packet);
    starttimer(A,TIMEOUT);
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  list_head = NULL;
  list_tail = NULL;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  int check_summ = pkt_checksum(packet.seqnum,packet.acknum,packet.payload);
  if(packet.checksum == check_summ && packet.acknum == B_goint_to_ACK){
    B_goint_to_ACK = next_seq(B_goint_to_ACK);
    tolayer5(B,packet.payload);
  }
  else{
    if(packet.checksum != check_summ){
      printf("packet from A checksum error \n");
    }
    else{
      printf("incorrect packet seq\n");
    }
    
    return;
  }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  list_head = NULL;
  list_tail = NULL;
}

int next_seq(int current_seq)
{
  if(current_seq == 0) return 1;
  else return 0;
}

void push_msg(struct msg message){
  struct msg_buffer *node = malloc(sizeof(struct msg_buffer));
  memcpy(node->message,message.data,MSG_SIZE);
  node->next = NULL;
  if(list_tail==NULL){
    //list empty
    node->prev = NULL;
    list_head = node;
    list_tail = list_head;
  }
  else{
    //add node to tail
    list_tail->next = node;
    node->prev = list_tail;
    list_tail = list_tail->next;
    
  }
}

struct msg_buffer* pop_msg(){
  if(list_head==NULL){
    printf("Empty list\n");
    return NULL;
  }
  struct msg_buffer* message = list_head;
  list_head = list_head->next;
  //no node left
  if(list_head == NULL) list_tail = NULL;
  //dereference the original head node
  else list_head->prev = NULL;
  message->next = NULL;
  return message;
}

int pkt_checksum(int seq,int ack,char* msg_ptr){
  int sum = 0;
  for (int i=0; i<MSG_SIZE; i++){
    sum += (int)(msg_ptr[i]);
  }
  sum += ack;
  sum += seq;
  return sum;
}
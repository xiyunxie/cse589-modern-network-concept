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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define BUFFER_SIZE 1000
#define MSG_SIZE 20
#define TIMEOUT 10.0
int A = 0;
int B = 1;
int base_A = 0;
int base_B = 0;
int window_size;
int next_seq = 0;
char *B_ACK_PKT_payload = "receive";

struct msg_buffer {
  int seq;
  int acked;
  int expire_time;
  struct pkt packet;
  struct msg_buffer *next;
};

struct msg_buffer *list_head;
struct msg_buffer *list_tail;

struct b_buf {
  int acknum;
  int acked;
  struct pkt packet;
  struct b_buf *next;
  
};
struct b_buf *B_buffer_head;
struct b_buf *B_buffer_tail;

struct pkt *B_ACK_PKT;
int current_time = 0;
void push_msg(struct msg *message,int seqnum,int acknum);
struct pkt* create_pkt(int seqnum, int acknum,char *message);
int pkt_checksum(int seq,int ack,char* msg_ptr);
struct msg_buffer* get_nextseq_msg();
void send_packet_nextseq_to_windows_end();
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  //will create packet at list tail, ack is 0 for A
  push_msg(message,next_seq,0);
  if(next_seq>=base_A+window_size) return;//refuse data
  struct msg_buffer *poped_msg = get_nextseq_msg();
  if(poped_msg==NULL){
    printf("no more packets\n");
    return;
  }
  tolayer3(A,poped_msg->packet);
  poped_msg->expire_time = current_time+TIMEOUT;
  next_seq++;
  return;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  int checksum = pkt_checksum(packet.seqnum,packet.acknum,packet.payload);
  if(checksum==packet.checksum){
    if(packet.acknum < base_A+window_size && packet.acknum >= base_A){
      //list ptr to base_A
      struct msg_buffer *list_ptr = list_head;
      for(int i=0;i<base_A;i++){
        list_ptr = list_ptr->next;
      }
      //check if ack received at position base_A
      if(list_ptr->packet.seqnum==packet.seqnum){
        base_A = packet.acknum+1;
        if(base_A == next_seq){
          stoptimer(A);
        }
        else//start new timer for packets between base_A and next_seq
          starttimer(A,TIMEOUT);
        //send the packets between next_seq and window's end
        send_packet_nextseq_to_windows_end();
      }
      else{//set packets to be acked, so that they won't be retransmitted when time out
        list_ptr = list_head;
        for(int i=0;i<packet.acknum;i++){
          list_ptr = list_ptr->next;
        }
        list_ptr->acked=1;
      }
    }
    else{
      printf("get out of range ack\n");
      return;
    }
  }
  else{
    printf("checksum error\n");
    return;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //update current time
  current_time = current_time+TIMEOUT;
  struct msg_buffer *list_ptr = list_head;
  for(int i=0;i<base_A;i++){
    list_ptr = list_ptr->next;
  }
  for(int i=0;i<window_size-base_A;i++){
    if(list_ptr==NULL) return;
    if(current_time>list_ptr->expire_time&&list_ptr->acked==0){
      //packet expired and not acked
      tolayer3(A,list_ptr->packet);
      
    }
    list_ptr = list_ptr->next;
  }
  stoptimer(A);
  starttimer(A,TIMEOUT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  window_size = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  int checksum = pkt_checksum(packet.seqnum,packet.acknum,packet.payload);
  if(packet.seqnum<base_B+window_size&&packet.seqnum>base_B){
    //in range
    if(checksum == packet.checksum){
      //move base B and send ack and to layer5
      
      printf("B send ack of %d\n",packet.seqnum);
      return;
    }
    else{
      //send ACK what B want
      tolayer3(B,*B_ACK_PKT);
      return;
    }
  }
  else{
    printf("check sum error\n");
    return;
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  window_size = getwinsize();
  B_ACK_PKT = create_pkt(0,0,B_ACK_PKT_payload);
}

int pkt_checksum(int seqnum,int acknum,char* msg_ptr){
  int sum = 0;
  for (int i=0; i<MSG_SIZE; i++){
    sum += (int)(msg_ptr[i]);
  } 
  sum += acknum;
  sum += seqnum;
  return sum;
}
struct pkt* create_pkt(int seqnum, int acknum,char *message){
    int checksum = pkt_checksum(seqnum,acknum,message);
    struct pkt *pkt_to_send = malloc(sizeof(struct pkt));
    pkt_to_send->seqnum = seqnum;
    pkt_to_send->acknum = acknum;
    pkt_to_send->checksum = checksum;
    memcpy(pkt_to_send->payload,message,MSG_SIZE);
    return pkt_to_send;
}

void push_msg(struct msg message,int seqnum,int acknum){
  struct msg_buffer *node = malloc(sizeof(struct msg_buffer));
  struct pkt *packet = create_pkt(seqnum,acknum,message.data);
  memcpy(&node->packet,packet,sizeof(packet));
  node->acked=0;
  node->seq = seqnum;
  free(packet);
  node->next = NULL;
  if(list_tail==NULL){
    //list empty
    list_head = node;
    list_tail = list_head;
  }
  else{
    //add node to tail
    list_tail->next = node;
    list_tail = list_tail->next;
    
  }
}

struct msg_buffer* get_nextseq_msg(){
  if(list_head==NULL){
    printf("Empty list\n");
    return NULL;
  }
  struct msg_buffer *list_ptr = list_head;
  for(int i=0;i<next_seq;i++){
    list_ptr = list_ptr->next;
  }
  return list_ptr;
}

void send_packet_nextseq_to_windows_end(){
  struct msg_buffer *list_ptr = list_head;
  for(int i=0;i<next_seq;i++){
    list_ptr = list_ptr->next;
  }
  for(int i=next_seq;i<base_A+window_size;i++){
    if(list_ptr==NULL) break;
    if(!list_ptr->acked){
      tolayer3(A,list_ptr->packet);
      printf("A send seq of %d\n",list_ptr->packet.seqnum);
    }
    list_ptr = list_ptr->next;
  }
  
}
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
int A_count=0;
int A = 0;
int B = 1;
int base_A = 0;
int next_seq = 0;
int B_expect = 0;
int window_size;
int packet_count=0;
char *B_ACK_PKT_payload = "receive";
struct pkt *B_ACK_PKT;
struct A_buf {
  int occupied;
  int seq;
  struct pkt packet;
};
struct A_buf A_buffer[BUFFER_SIZE];


void push_msg(struct msg *message,int seqnum,int acknum);
struct msg_buffer* get_nextseq_msg();
int pkt_checksum(int seq,int ack,char* msg_ptr);
struct pkt* create_pkt(int seqnum, int acknum,char *message);
void send_packet_nextseq_to_windows_end();

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  //will create packet at list tail, ack is 0 for A
  push_msg(message,A_count++,0);
  if(next_seq>=base_A+window_size) return;//refuse data
  
  //will send last(next_seq) packet
  struct A_buf *poped_msg = get_nextseq_msg();
  if(poped_msg==NULL){
    printf("no more packets\n");
    return;
  }
  tolayer3(A,A_buffer[next_seq].packet);
  if(base_A == next_seq) starttimer(A,TIMEOUT);
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
      
      if(A_buffer[base_A].packet.seqnum==packet.acknum){
        base_A = packet.acknum+1;
        if(base_A == next_seq){
          stoptimer(A);
        }
        else{
          starttimer(A,TIMEOUT);
        }
        // int extra_send = 0;  
        // for(int i=next_seq;i<base_A+window_size;i++){
        //   if(A_buffer[i].occupied){

        //     tolayer3(A,A_buffer[i].packet);
        //     printf("A send seq %d\n",A_buffer[i].packet.seqnum);
        //     extra_send++;
        //   }
        //   else break;
        // }
        // next_seq += extra_send;
      }
      else{
        printf("wrong packet ACK\n");
        return;
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
  for(int i=base_A;i<next_seq;i++){
    //will send packets from baseA to next_seq again
    if(A_buffer[i].occupied){
      tolayer3(A,A_buffer[i].packet);
      printf("A send seq of %d\n",A_buffer[i].packet.seqnum);
    }
  }
  starttimer(A,TIMEOUT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  window_size = getwinsize();
  for(int i=0;i<BUFFER_SIZE;i++){
    A_buffer[i].seq=i;
    memcpy(&A_buffer[i].packet,'\0',sizeof(struct pkt));
  }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  int checksum = pkt_checksum(packet.seqnum,packet.acknum,packet.payload);
  if(checksum == packet.checksum){
    if(packet.seqnum==B_expect){
      tolayer5(B,packet.payload);
      free(B_ACK_PKT);
      B_ACK_PKT = create_pkt(0,B_expect++,B_ACK_PKT_payload);
      tolayer3(B,*B_ACK_PKT);
      printf("B send ack of %d\n",B_expect);
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
  B_expect = 0;
  B_ACK_PKT = create_pkt(0,B_expect,B_ACK_PKT_payload);
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
  struct A_buf *node = malloc(sizeof(struct A_buf));
  struct pkt *packet = create_pkt(seqnum,acknum,message.data);
  memcpy(&node->packet,packet,sizeof(packet));
  free(packet);
  
}
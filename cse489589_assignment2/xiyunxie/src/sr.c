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
#define TIMECHECK 2.0
int A = 0;
int B = 1;
int base_A = 0;
int base_B = 0;
int window_size;
int next_seq = 0;
char *B_ACK_PKT_payload = "receive";
int timeron = 0;

struct A_buf {
  int seq;
  int acked;
  int expire_time;
  int occupied;
  struct pkt packet;
};

struct A_buf A_buffer[BUFFER_SIZE];

struct b_buf {
  int acknum;
  int acked;
  struct pkt packet;
};
struct b_buf B_buffer[BUFFER_SIZE];
int a_buffer_count=0;
int b_buffer_count=0;
struct pkt *B_ACK_PKT;
int current_time = 0;

struct pkt* create_pkt(int seqnum, int acknum,char *message);
int pkt_checksum(int seq,int ack,char* msg_ptr);
void A_push_msg(struct msg message,int seqnum,int acknum);
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  //will create packet at list tail, ack is 0 for A
  A_push_msg(message,a_buffer_count,0);
  A_buffer[a_buffer_count++].occupied=1;
  if(next_seq>=base_A+window_size) return;//refuse data
  tolayer3(A,A_buffer[next_seq].packet);
  A_buffer[next_seq].expire_time = current_time+TIMEOUT;
  next_seq++;
  if(timeron==0){
    starttimer(A,TIMECHECK);
    timeron = 1;
  }
  return;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  int checksum = pkt_checksum(packet.seqnum,packet.acknum,packet.payload);
  if(checksum==packet.checksum){
    if(packet.acknum < base_A+window_size && packet.acknum >= base_A){
      //check if ack received at position base_A
      if(A_buffer[base_A].packet.seqnum == packet.acknum){
        A_buffer[packet.acknum].acked=1;
        base_A = packet.acknum+1;
        if(base_A == next_seq){
          stoptimer(A);
          timeron = 0;
        }
        else{//start new timer for packets between base_A and next_seq
          starttimer(A,TIMECHECK);
          timeron = 1;
        }
        //send the packets between next_seq and window's end
        //send_packet_nextseq_to_windows_end();
          int extra_send=0;
          for(int i=next_seq;i<base_A+window_size;i++){
            if(A_buffer[i].occupied){

              tolayer3(A,A_buffer[i].packet);
              printf("A send seq %d\n",A_buffer[i].packet.seqnum);
              extra_send++;
            }
            else break;
          }
          next_seq += extra_send;
      }
      else{//set packets to be acked, so that they won't be retransmitted when time out
        A_buffer[packet.acknum].acked=1;
        
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
  //this function is used to periodically check if there are packets
  //from base to next_seq timeout, not for only one packet 
  //update current time
  current_time = current_time+TIMECHECK;
  
  for(int i=base_B;i<base_A+window_size;i++){
    if(A_buffer[i].occupied==0) break;
    if(current_time>A_buffer[i].expire_time&&A_buffer[i].acked==0){
      //packet expired and not acked
      tolayer3(A,A_buffer[i].packet);
      printf("A send seq %d\n",A_buffer[i].packet.seqnum);
    }
    
  }
  stoptimer(A);
  starttimer(A,TIMECHECK);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  window_size = getwinsize();
  for(int i=0;i<BUFFER_SIZE;i++){
    A_buffer[i].acked=0;
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
    //check if in range
    if(packet.seqnum<base_B+window_size&&packet.seqnum>=base_B){
      B_buffer[packet.acknum].acked = 1;
      if(packet.seqnum == base_B){
        //need to check if current packet fills a gap
        while(1){
          if(B_buffer[base_B].acked==1){
            //send data to layer 5
            tolayer5(B,packet.payload);
            //send ack
            free(B_ACK_PKT);
            B_ACK_PKT = create_pkt(0,base_B,B_ACK_PKT_payload);
            tolayer3(B,*B_ACK_PKT);
            printf("B send ack of %d\n",base_B);
            base_B++;
          }
          else break;
        }
      }
      else{//B get out of order but in-window packet
        //save packet in buffer
        memcpy(&B_buffer[packet.seqnum].packet,packet,sizeof(packet));
        B_buffer[packet.seqnum].acked==1;
        //send ack
        free(B_ACK_PKT);
        B_ACK_PKT = create_pkt(0,packet.seqnum,B_ACK_PKT_payload);
        tolayer3(B,*B_ACK_PKT);
        printf("B send ack of %d\n",packet.seqnum);
        return;
      }
      //send ack
      
    }
    else{
      if(packet.acknum>=base_B-window_size&&packet.acknum<base_B+window_size){
        B_ACK_PKT = create_pkt(0,packet.seqnum,B_ACK_PKT_payload);
        tolayer3(B,*B_ACK_PKT);
        printf("B send ack of %d\n",packet.seqnum);
      }
    }
  }
  else{//if packet seq is in range[base_B-window_size,base_B+window_size],send ack only
    
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
  for(int i=0;i<BUFFER_SIZE;i++){
    B_buffer[i].acked=0;
    B_buffer[i].acknum=i;
    memcpy(&B_buffer[i].packet,'\0',sizeof(struct pkt));
  }
  
  
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

void A_push_msg(struct msg message,int seqnum,int acknum){
  struct A_buf *node = malloc(sizeof(struct A_buf));
  struct pkt *packet = create_pkt(seqnum,acknum,message.data);
  memcpy(&node->packet,packet,sizeof(packet));
  free(packet);
  
}
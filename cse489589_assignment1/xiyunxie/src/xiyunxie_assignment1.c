/**
 * @xiyunxie_assignment1
 * @author  Xiyun Xie <xiyunxie@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>

#include "../include/global.h"
#include "../include/logger.h"
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#define TRUE 1
#define MSG_SIZE 256
#define BUFFER_SIZE 256
#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define HOST_SIZE 100
#define PORT_SIZE 100
#define IP_SIZE 100
#define MSG_BUF_SIZE 100
#define FILE_SIZE 10240
#define FILE_PATH_SIZE 20
int client_first_login=1;
int listen_port;
char ip_for_client[IP_SIZE];
char* client_ip;
char c_port[PORT_SIZE];
char* server_ip;
int serverfd_for_client;
char host_name[BUFFER_SIZE];
int client_logged_in = 0;

int server_socket;
int client_bind;
int run_mode=0;

fd_set server_master_list, server_watch_list;
fd_set client_master_list, client_watch_list;
struct client_record {
    int id;
    char ip_addr[IP_SIZE];
    char hostname[HOST_SIZE];
    int client_port;
    int msg_received;
    int msg_sent;
    int status;
    int sockfd;
    int struct_occupied;
};
struct client_record client_list_ptr;
struct block_list{
    int block_occupied;
    struct client_record blocker;
    struct client_record blocked[3];
};
struct block_list block_list_for_server[4];
struct client_record client_list[4];
struct client_msg{
	char cmd[CMD_SIZE];
	char sender_ip[IP_SIZE];
    char receiver_ip[IP_SIZE];
	char msg[BUFFER_SIZE];
};
struct server_respond_msg{
	int success;
	char sender_ip[IP_SIZE];
    char receiver_ip[IP_SIZE];
	char msg[BUFFER_SIZE];
};
struct server_buffered_msg{
    int sockfd;
    int buffer_occupied;
    int most_recent_empty_index;
    int buffered_message_size;
    char receiverIP[IP_SIZE];
    char senders[MSG_BUF_SIZE][IP_SIZE];
    char msgs[MSG_BUF_SIZE][BUFFER_SIZE];
};
struct p2p_msg{
    int is_txt;
    char path[FILE_PATH_SIZE];
    char file_buffer[FILE_SIZE];
};
struct server_buffered_msg server_msg_buffers[4];
void client_mode(int client_port);
void server_mode(int server_port);
int connect_to_server(char *server_ip, int server_port,char* client_port);
int client_bind_socket(int client_port);
int command_to_list(char* cmd,char** res);
int get_host_ip(char* buffer);
int valid_ip(char* ip_address);
int valid_port(char* port);
int ip_exist(char* client_ip);
void sort_client_list(struct client_record client_list[4]);
void sort_block_list_of_server(char* sender_ip);
void buf_clients(char* buffer);
int validate_number(char *str);
int blocked_by_sender(char* blocker_ip,char* blocked_ip);
void print_client_list();
void print_buffer_msgs(char* client_ip);
void print_all_buffer_msgs();
void print_block_lists(char* client_ip);
void print_all_block_lists();
int get_receiver_fd(char* client_ip);
void sighandler(int sig_num);
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

void client_mode(int client_port){
    listen_port = client_port;
    //client bind socket
    printf("client port %d\n",client_port);
    client_bind = client_bind_socket(client_port);
    if(client_bind==0)
        exit(-1);
    printf("PA1-Client on\n");
    FD_ZERO(&client_master_list);
    FD_ZERO(&client_watch_list);
    FD_SET(STDIN, &client_master_list);
    int head_socket = STDIN;
    int selret,sock_index;
    struct client_msg client_message;
    struct server_respond_msg server_respond_message;
    char c_buffer[BUFFER_SIZE];
    int get_host_successful = get_host_ip(c_buffer);
    struct p2p_msg p2p_message;
    while(1){
        
        memcpy(&client_watch_list, &client_master_list, sizeof(client_master_list));

        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &client_watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.\n");

        /* Check if we have sockets/STDIN to process */
        if(selret > 0){
            /* Loop through socket descriptors to check which ones are ready */
            for(sock_index=0; sock_index<=head_socket; sock_index+=1){
                if(FD_ISSET(sock_index, &client_watch_list)){
                    printf("sock index is %d\n",sock_index);
                    //new command from STDIN
                    if (sock_index == STDIN){
                    	char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);

                    	memset(cmd, '\0', CMD_SIZE);
                        fflush(stdout);	
						if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
                        char *cmd_for_send = (char*) malloc(sizeof(char)*CMD_SIZE);
                        // strcpy(cmd_for_send,cmd);
                        memset(cmd_for_send, '\0', CMD_SIZE);
                        // strcpy(cmd_for_send,cmd);
                        char* pos;
                        if ((pos=strchr(cmd, '\n')) != NULL)
                            *pos = '\0';
                        strcpy(cmd_for_send,cmd);
                        // if ((pos=strchr(cmd_for_send, '\n')) != NULL){
                        //     *pos = '\0';
                        //     printf("next line found\n");
                        // }
                            
                        // memcpy(cmd_for_send,cmd,strlen(cmd)+1);
                        // printf("str length for cmd %d\n",strlen(cmd_for_send));
                        // printf("Command for send is: %s\n", cmd_for_send);
						// printf("Command is: %s\n", cmd);
                        //cmd split used code at https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
						char *client_args[5];
                        int count = 0;
                        char *tmp = strtok(cmd, " ");
                        while(tmp != NULL){
                            client_args[count++] = tmp;
                            tmp = strtok(NULL, " ");
                        }
                        for (int i = 0; i < count; ++i) 
                            printf("%s\n", client_args[i]);
						//Author command
                        if((strcmp(client_args[0],"AUTHOR"))==0)
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
                            fflush(stdout);
							cse4589_print_and_log("I, xiyunxie, have read and understood the course academic integrity policy.\n");
							fflush(stdout);
                            cse4589_print_and_log("[AUTHOR:END]\n");
                            fflush(stdout);
						}
                        else if((strcmp(client_args[0],"PORT"))==0)
						{
                            cse4589_print_and_log("[PORT:SUCCESS]\n");
                            fflush(stdout);
							cse4589_print_and_log("PORT:%d\n", listen_port);
							fflush(stdout);
                            cse4589_print_and_log("[PORT:END]\n");
                            fflush(stdout);
						}
                        else if((strcmp(client_args[0],"IP"))==0)
						{
							// retrieve IP
                            char buffer[BUFFER_SIZE];
                            int get_host_successful = get_host_ip(buffer);
         
                            if(get_host_successful == 1)
                            {
                                cse4589_print_and_log("[IP:SUCCESS]\n");
                                fflush(stdout);
                                cse4589_print_and_log("IP:%s\n",buffer);
                                fflush(stdout);
                                cse4589_print_and_log("[IP:END]\n");
                                fflush(stdout);
                            }
                            else{
                                cse4589_print_and_log("[IP:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[IP:END]\n");
                                fflush(stdout);
                            }
                                
						}
                        else if((strcmp(client_args[0],"LOGIN"))==0)
						{
                            printf("login\n");
                            if (count != 3) {
                                printf("Login must have 3 args\n");
                                continue;
                            }
							char* server_ip = client_args[1];
                            // printf("server IP %s\n",server_ip);
                            int ip_v = valid_ip(server_ip);
                            // printf("server IP %s\n",server_ip);
                            int port_v = valid_port(client_args[2]);
                            // printf("ip valid result: %d\n",ip_v);
                            // printf("port valid result: %d\n",port_v);
                            if(ip_v==0||port_v==0){
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                continue;
                            }
                            // printf("ip and port checked\n");
                            if(client_first_login==0){
                                //has logged in before
                                memset(&client_message, '\0', sizeof(client_message));
                                strcpy(client_message.cmd,"LOGIN");
                                strcpy(client_message.sender_ip,ip_for_client);
                                // printf("%s loging in\n",client_message.sender_ip);
                                if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                    printf("send log in cmd done\n");
                                }
                                client_logged_in = 1;
                                memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                //receive number of buffered ,sg size first
                                if(recv(serverfd_for_client, &server_respond_message, sizeof(server_respond_message), 0)==sizeof(server_respond_message)){
                                    //convert buffer size
                                    int buf_size = atoi(server_respond_message.msg);
                                    printf("client has %d message to receive\n",buf_size);
                                    for(int i=0;i<buf_size;i++){
                                        memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                        //keep receiving new data
                                        if(recv(serverfd_for_client, &server_respond_message, sizeof(server_respond_message), 0)==sizeof(server_respond_message)){
                                            cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                            fflush(stdout);
                            
                                            cse4589_print_and_log("msg from:%s\n[msg]:%s\n", server_respond_message.sender_ip, server_respond_message.msg);
                                            cse4589_print_and_log("[RECEIVED:END]\n");
                                            fflush(stdout);
                                        }
                                    }
                                    
                                }
                                char update[sizeof(struct client_record)*4];
                                //recv other user info
                                recv(serverfd_for_client, update, sizeof(struct client_record)*4, 0);
                                memcpy(client_list,update,sizeof(struct client_record)*4); 
                                int print_index=1;
                                for(int i=0;i<4;i++){
                                    if(client_list[i].struct_occupied==1&&client_list[i].status==1){
                                        char login[20];
                                        strcpy(login,"logged-in");
                                    printf("%-5d%-35s%-20s%-8d\n", print_index++, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                    // cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", print_index++, client_list[i].hostname, client_list[i].msg_sent, client_list[i].msg_received, login);
                                    }
                                }
                            }
                            else{
                                printf("client login first time\n");
                                int server_port = atoi(client_args[2]);
                                // printf("server IP %s\n",server_ip);
                                //connect to server
                                serverfd_for_client = connect_to_server(server_ip,server_port,c_port);
                                // printf("Connecting to server IP: %s with port %d\n",server_ip,server_port);
                                FD_SET(serverfd_for_client, &client_master_list);
                                if(serverfd_for_client>head_socket) head_socket=serverfd_for_client;
                                char update[sizeof(struct client_record)*4];
                                //recv other user info
                                if(recv(serverfd_for_client, update, sizeof(struct client_record)*4, 0)==sizeof(update)){
                                    memcpy(client_list,update,sizeof(struct client_record)*4); 
                                    int print_index=1;
                                    for(int i=0;i<4;i++){
                                        if(client_list[i].struct_occupied==1&&client_list[i].status==1){
                                            char login[20];
                                            strcpy(login,"logged-in");
                                        printf("%-5d%-35s%-20s%-8d\n", print_index, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                        // cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", print_index++, client_list[i].hostname, client_list[i].msg_sent, client_list[i].msg_received, login);
                                        }
                                    }
                                    client_logged_in = 1;
                                    client_first_login=0;
                                }
                                
                            }
                            // printf(update);
                            cse4589_print_and_log("[LOGIN:SUCCESS]\n");
                            fflush(stdout);
                            cse4589_print_and_log("[LOGIN:END]\n");
                            fflush(stdout);
							// cse4589_print_and_log("[IP:END]\n");
                            continue;
						}
                        else if((strcmp(client_args[0],"LIST"))==0)
						{
                            cse4589_print_and_log("[LIST:SUCCESS]\n");
                            fflush(stdout);
                            int print_index=1;
                            for(int i=0;i<4;i++){
                                if(client_list[i].struct_occupied==1&&client_list[i].status==1){
                                    char login[20];
                                    strcpy(login,"logged-in");
                                // printf("%-5d%-35s%-20s%-8d\n", print_index, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", print_index++, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                    // fflush(stdout);
                                }
                            }
                            cse4589_print_and_log("[LIST:END]\n");
                            // fflush(stdout);
                            continue;
						}
                        else if((strcmp(client_args[0],"REFRESH"))==0)
						{
                            if(client_logged_in==0){
                                printf("NOT logged in\n");
                                cse4589_print_and_log("[REFRESH:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[REFRESH:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            // printf("requesting refresh\n");
                            memset(&client_message, '\0', sizeof(client_message));
                            strcpy(client_message.cmd,"REFRESH");
                            strcpy(client_message.sender_ip,ip_for_client);
                            //send refresh request
							if(send(serverfd_for_client, &client_message, sizeof(client_message), 0)==sizeof(client_message)){
                                // printf("refresh request send\n");
                                char update[sizeof(struct client_record)*4];
                                if(recv(serverfd_for_client, update, sizeof(struct client_record)*4, 0)==sizeof(update)){
                                    // printf("refresh respond received\n");
                                    memcpy(client_list,update,sizeof(struct client_record)*4); 
                                    int print_index=1;
                                    for(int i=0;i<4;i++){
                                        if(client_list[i].struct_occupied==1&&client_list[i].status==1){
                                            char login[20];
                                            strcpy(login,"logged-in");
                                        printf("%-5d%-35s%-20s%-8d\n", print_index++, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                        // cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", print_index++, client_list[i].hostname, client_list[i].msg_sent, client_list[i].msg_received, login);
                                        }
                                    }
                                    cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                                    fflush(stdout);
                                    cse4589_print_and_log("[REFRESH:END]\n");
                                    fflush(stdout);
                                }
                                
                            }
                            continue;
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"SEND"))==0)
						{
                            if (count < 3) {
                                printf("Send request must have 3 or more args\n");
                                continue;
                            }
                            if(client_logged_in==0){
                                printf("NOT logged in\n");
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[SEND:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            printf("ip is %s in send\n",client_args[1]);
                            if(valid_ip( client_args[1])==0||ip_exist(client_args[1])<0){
                                printf("IP not valid\n");
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[SEND:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            // printf("packeting message\n");
							char* receiver_ip = client_args[1];
                            char msg[BUFFER_SIZE];
                            int msg_start=6;
                            int msg_length=0;
                            while(cmd_for_send[msg_start]!=' '){
                                msg_start++;
                            }
                            // printf("get where to start\n");
                            msg_start+=1;
                            while(cmd_for_send[msg_start+msg_length]!='\0'){
                                msg_length++;
                            }
                            printf("get msg length %d\n",msg_length);
                            if(msg_length>256)
                                msg_length==256;
                            char *msg_ptr = cmd_for_send+msg_start;
                            memset(&client_message, '\0', sizeof(client_message));
                            strcpy(client_message.cmd,"SEND");
                            strncpy(client_message.msg,msg_ptr,msg_length);
                            strcpy(client_message.sender_ip,ip_for_client);
                            strcpy(client_message.receiver_ip,receiver_ip);
                            printf("Sending from %s '%s 'to: %s\n",client_message.sender_ip,client_message.msg, client_message.receiver_ip);
                            if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                printf("send done\n");
                            }
                            //if receiver blocked, no error
                            cse4589_print_and_log("[SEND:SUCCESS]\n");
                            fflush(stdout);
                            cse4589_print_and_log("[SEND:END]\n");
                            fflush(stdout);
							// cse4589_print_and_log("[IP:END]\n");
                            continue;
						}
                        else if((strcmp(client_args[0],"BROADCAST"))==0)
						{
							if (count < 2) {
                                printf("Msg cannot be empty\n");
                                printf("NOT logged in\n");
                                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[BROADCAST:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            if(client_logged_in==0){
                                printf("NOT logged in\n");
                                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[BROADCAST:END]\n");
                                fflush(stdout);
                                continue;
                            }
							
                            char msg[BUFFER_SIZE];
                            int msg_start=10;
                            int msg_length=0;
                            
                            while(cmd_for_send[msg_start+msg_length]!='\0'){
                                msg_length++;
                            }
                            char *msg_ptr = cmd_for_send+msg_start;
                            memset(&client_message, '\0', sizeof(client_message));
                            strcpy(client_message.cmd,"BROADCAST");
                            strncpy(client_message.msg,msg_ptr,msg_length);
                            strcpy(client_message.sender_ip,ip_for_client);
                            // printf("sender ip is %s\n",client_message.sender_ip);
                            printf("Broad casting '%s'from %s\n",client_message.msg,client_message.sender_ip);
                            if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                printf("broadcast done\n");
                            }
                            //if any receiver is blockin gthis client, no error
                            cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
                            fflush(stdout);
                            cse4589_print_and_log("[BROADCAST:END]\n");
                            fflush(stdout);
							// cse4589_print_and_log("[IP:END]\n");
                            continue;
						}
                        else if((strcmp(client_args[0],"BLOCK"))==0)
						{
                            printf("client block\n");
							if (count != 2) {
                                printf("Block must have 2 args\n");
                                continue;
                            }
                            // printf("ip valid result: %d\n",valid_ip(client_args[1]));
                            if(valid_ip( client_args[1])==0||ip_exist(client_args[1])<0){
                                cse4589_print_and_log("[BLOCK:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[BLOCK:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            memset(&client_message, '\0', sizeof(client_message));
                            char* block_ip;
                            // strcpy( block_ip, client_args[1]);
                            strcpy(client_message.cmd,"BLOCK");
                            strcpy(client_message.sender_ip,ip_for_client);
                            strcpy(client_message.msg,client_args[1]);
                            printf("Blocking %s\n", client_message.msg);
                            if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                printf("block send\n");
                            }
                            memset(&server_respond_message,'\0',sizeof(server_respond_message));
                            if(recv(serverfd_for_client, &server_respond_message, sizeof(server_respond_message), 0)==sizeof(server_respond_message)){
                                //check if server send a success response back
                                if(server_respond_message.success==1){
                                    cse4589_print_and_log("[BLOCK:SUCCESS]\n");
                                    fflush(stdout);
                                    cse4589_print_and_log("[BLOCK:END]\n");
                                    fflush(stdout);
                                }
                                else{
                                    cse4589_print_and_log("[BLOCK:ERROR]\n");
                                    fflush(stdout);
                                    cse4589_print_and_log("[BLOCK:END]\n");
                                    fflush(stdout);
                                }
                            }
                            
							// cse4589_print_and_log("[IP:END]\n");
                            continue;
						}
                        else if((strcmp(client_args[0],"UNBLOCK"))==0)
						{
							if (count != 2) {
                                printf("Block must have 2 args\n");
                                continue;
                            }
                            if(valid_ip( client_args[1])==0||ip_exist(client_args[1])<0){
                                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[UNBLOCK:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            memset(&client_message, '\0', sizeof(client_message));
                            char* unblock_ip;
                            // strcpy( unblock_ip, client_args[1]);
                            strcpy(client_message.cmd,"UNBLOCK");
                            strcpy(client_message.sender_ip,ip_for_client);
                            strcpy(client_message.msg,client_args[1]);
                            printf("Unlocking %s\n", unblock_ip);
                            if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                printf("unblock send\n");
                            }
                            memset(&server_respond_message,'\0',sizeof(server_respond_message));
                            if(recv(serverfd_for_client, &server_respond_message, sizeof(server_respond_message), 0)==sizeof(server_respond_message)){
                                //check if server respond unblock successful or not
                                if(server_respond_message.success==1){
                                    cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
                                    fflush(stdout);
                                    cse4589_print_and_log("[UNBLOCK:END]\n");
                                    fflush(stdout);
                                }
                                else{
                                    cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                                    fflush(stdout);
                                    cse4589_print_and_log("[UNBLOCK:END]\n");
                                    fflush(stdout);
                                }
                            }
							// cse4589_print_and_log("[IP:END]\n");
                            continue;
						}
                        else if((strcmp(client_args[0],"LOGOUT"))==0)
						{
							if(client_logged_in==0){
                                printf("NOT logged in\n");
                                cse4589_print_and_log("[LOGOUT:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[LOGOUT:END]\n");
                                fflush(stdout);
                                continue;
                            }
                            client_logged_in=0;
                            memset(&client_message, '\0', sizeof(client_message));
                            strcpy(client_message.cmd,"LOGOUT");
                            strcpy(client_message.sender_ip,ip_for_client);
                            if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                printf("Logging out\n");
                                cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[LOGOUT:END]\n");
                                fflush(stdout);
                            }
                            
							// cse4589_print_and_log("[IP:END]\n");
                            continue;
						}
                        else if((strcmp(client_args[0],"EXIT"))==0)
						{
							if(client_first_login==0){
                                memset(&client_message, '\0', sizeof(client_message));
                                strcpy(client_message.cmd,"EXIT");
                                strcpy(client_message.sender_ip,ip_for_client);
                                if(send(serverfd_for_client,&client_message, sizeof(client_message),0)==sizeof(client_message)){
                                    cse4589_print_and_log("[EXIT:SUCCESS]\n");
                                    fflush(stdout);
                                    cse4589_print_and_log("[EXIT:END]\n");
                                    fflush(stdout);
                                    close(serverfd_for_client);
                                    exit(0);
                                }
                            }
                            
                            printf("Exiting\n");
                            close(client_bind);
                            cse4589_print_and_log("[EXIT:SUCCESS]\n");
                            fflush(stdout);
                            cse4589_print_and_log("[EXIT:END]\n");
                            fflush(stdout);
                            exit(0);
                            //need to exit program
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"SENDFILE"))==0)
						{
                            printf("p2p\n");
                            if (count != 3) {
                                printf("Login must have 3 args\n");
                                continue;
                            }
							char* p2p_ip = client_args[1];
                            // printf("server IP %s\n",server_ip);
                            int ip_v = valid_ip(p2p_ip);
                            // printf("server IP %s\n",server_ip);
                            char file_path[FILE_PATH_SIZE];
                            strcpy(file_path,client_args[2]);
                            printf("file path '%s'\n",file_path);
                            // printf("ip valid result: %d\n",ip_v);
                            // printf("port valid result: %d\n",port_v);
                            FILE *fp = fopen(file_path, "r");
                            int ip_index = ip_exist(client_args[1]);
                            if(ip_v==0||fp==NULL||ip_index<0){
                                cse4589_print_and_log("[SENDFILE:ERROR]\n");
                                cse4589_print_and_log("[SENDFILE:END]\n");
                                continue;
                            }
                            printf("ip and file input checked\n");
                            memset(&p2p_message,'\0',sizeof(p2p_message));
                            strcpy(p2p_message.path,file_path);
                            if(fp != NULL){
                                char symbol;
                                while((symbol = getc(fp)) != EOF)
                                {
                                    strcat(p2p_message.file_buffer, &symbol);
                                }
                                fclose(fp);
                            }
                            int fdsocket, len;
                            int port = client_list[ip_index].client_port;
                            struct sockaddr_in p2p_addr;
                            printf("p2p ip is %s\n",client_args[1]);
                            printf("p2p port is %d\n",port);
                            
                            fdsocket = socket(AF_INET, SOCK_STREAM, 0);
                            if(fdsocket < 0){
                                perror("Failed to create p2p socket");
                                continue;
                            }
                                
                            printf("p2p socket for host created\n");
                            bzero(&p2p_addr, sizeof(p2p_addr));
                            p2p_addr.sin_family = AF_INET;
                            inet_pton(AF_INET, client_args[1], &p2p_addr.sin_addr);
                            printf("p2p addr get\n");
                            p2p_addr.sin_port = htons(port);
                            printf("trying to connect\n");
                            if(connect(fdsocket, (struct sockaddr*)&p2p_addr, sizeof(p2p_addr)) < 0)
                                perror("Connect failed");
                            else
                                printf("Connected p2p\n");
                            
                            //int s = send(fdsocket,client_port, sizeof(client_port),0);
                            continue;
                        }
						free(cmd);
                        free(cmd_for_send);
                    }


                    else if(sock_index == client_bind){
                        //p2p send file mode
                        int p2p_accept_fd, caddr_len;
	                    struct sockaddr_in p2p_client_addr;
                        caddr_len = sizeof(p2p_client_addr);
                        p2p_accept_fd = accept(client_bind, (struct sockaddr *)&p2p_client_addr, &caddr_len);
                        printf("new client accepted\n");
                        if(p2p_accept_fd < 0){
                            perror("Accept failed.");
                            close(p2p_accept_fd);
                            continue;
                        }
                        // if(recv(p2p_accept_fd,&p2p_message,sizeof(p2p_message))==sizeof(p2p_message)){
                        //     printf("path is %s\n",p2p_message.path);
                        //     printf("buffer %s\n",p2p_message.file_buffer);
                        //     FILE *fp;

                        //     fp = fopen(p2p_message.path, "w+");
                        // }    
                        close(p2p_accept_fd);
                        continue;
                    }
                    //else server send a message to
                    else{
                        printf("server message come in\n");
                        /* Initialize buffer to receieve response */
                        // char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(&server_respond_message, '\0', sizeof(server_respond_message));

                        // if(recv(sock_index, &server_respond_message, sizeof(server_respond_message), 0) <= 0){
                        //     close(sock_index);
                        //     printf("Remote Host terminated connection!\n");

                        //     /* Remove from watched list */
                        //     FD_CLR(sock_index, &server_master_list);
                        // }
                        // else {//print message 
                        // 	cse4589_print_and_log("msg from:%s\n[msg]:%s\n", server_respond_message.sender_ip, server_respond_message.msg);
                        // }
                        if(recv(sock_index, &server_respond_message, sizeof(server_respond_message), 0)==sizeof(server_respond_message)){
                            printf("msg is %s\n",server_respond_message.msg);
                            if(strcmp(server_respond_message.msg,"server_will_exit")==0){
                                close(serverfd_for_client);
                                printf("server fd closed\n");
                            }
                            else{
                                cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                fflush(stdout);
                                cse4589_print_and_log("msg from:%s\n[msg]:%s\n", server_respond_message.sender_ip, server_respond_message.msg);
                                cse4589_print_and_log("[RECEIVED:END]\n");
                                fflush(stdout);
                            }
                            
                        }
                        // free();
                    }
                    printf("++++++++++++++++++++++++++++++++++++++++\n;");
                }
            }
        }
    }
}
void server_mode(int server_port){
    // printf("port is %d\n",server_port);
    listen_port = server_port;
    // printf("listen port %d\n",listen_port);
    //running server
    int client_count = 0;
	printf("Server on\n");
	int port, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in server_addr, client_addr;
	//fd_set master_list, watch_list;

	//server create socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
		perror("Cannot create socket");
    int enable = 1;
    // if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    //     perror("make socket resuable failed");
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0)
        perror("make socket resuable failed");
	//fill socket address required information
	bzero(&server_addr, sizeof(server_addr));
    for(int i=0;i<4;i++){
        memset(&client_list[i],'\0',sizeof(struct client_record ));
        client_list[i].struct_occupied=0;
    }
    memset(block_list_for_server, '\0', sizeof(block_list_for_server));
    for(int i=0;i<4;i++){
        // memset(client_list[i],'\0',sizeof(struct client_record ));
        block_list_for_server[i].block_occupied=0;
    }
    memset(server_msg_buffers,'\0',sizeof(server_msg_buffers));
    // printf("size for all clients is %d\n",sizeof(client_list));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);
    // printf("trying to bind socket with port %d\n",server_port);
    //server socket bind with address
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )
    	perror("Server bind failed");
    // printf("server socket is %d\n",server_socket);
    //server start listening
    if(listen(server_socket, BACKLOG) < 0)
    	perror("Unable to listen on port");
    // printf("Server start listening\n");
    /* ---------------------------------------------------------------------------- */
    struct client_msg client_send_data;

    /* Zero select FD sets */
    FD_ZERO(&server_master_list);
    FD_ZERO(&server_watch_list);
    
    /* Register the listening socket */
    FD_SET(server_socket, &server_master_list);
    /* Register STDIN */
    FD_SET(STDIN, &server_master_list);

    head_socket = server_socket;
    struct server_respond_msg server_respond_message;
    while(TRUE){
        // printf("start while\n");
        memcpy(&server_watch_list, &server_master_list, sizeof(server_master_list));
		//fflush(stdout);

        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &server_watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");
        // printf("can select %d\n",selret);
        /* Check if we have sockets/STDIN to process */
        if(selret > 0){
            /* Loop through socket descriptors to check which ones are ready */
            for(sock_index=0; sock_index<=head_socket; sock_index+=1){
                // printf("sock index is %d\n",sock_index);
                if(FD_ISSET(sock_index, &server_watch_list)){
                    // printf("sock index %d is in set\n",sock_index);
                    //new command from STDIN
                    if (sock_index == STDIN){
                    	char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);

                    	memset(cmd, '\0', CMD_SIZE);
                        fflush(stdout);	
						if(fgets(cmd, CMD_SIZE-1, stdin) == NULL)
							exit(-1);
                        char* pos;
                        if ((pos=strchr(cmd, '\n')) != NULL)
                            *pos = '\0';
						// printf("Command is: %s\n", cmd);
                        //cmd split used code at https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
						char *server_args[5];
                        int count = 0;
                        char *tmp = strtok(cmd, " ");
                        while(tmp != NULL){
                            server_args[count++] = tmp;
                            tmp = strtok(NULL, " ");
                        }
                        for (int i = 0; i < count; ++i) 
                            printf("%s\n", server_args[i]);
						//Author command
                        if((strcmp(server_args[0],"AUTHOR"))==0)
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
							cse4589_print_and_log("I, xiyunxie, have read and understood the course academic integrity policy.\n");
							cse4589_print_and_log("[AUTHOR:END]\n");
						}
                        else if((strcmp(server_args[0],"PORT"))==0)
						{
							cse4589_print_and_log("PORT:%d\n", listen_port);
							
						}
                        else if(strcmp(cmd,"PC")==0){
                                print_client_list();
                        }
                        else if(strcmp(cmd,"PB")==0){
                            print_all_block_lists();
                        }
                        else if(strcmp(cmd,"PM")==0){
                            print_all_buffer_msgs();
                        }
                        else if((strcmp(server_args[0],"IP"))==0)
						{
							// retrieve IP
                            char buffer[BUFFER_SIZE];
                            int get_host_successful = get_host_ip(buffer);
         
                            if(get_host_successful == 1)
                            {
                                cse4589_print_and_log("[IP:SUCCESS]\n");
                                cse4589_print_and_log("IP:%s\n",buffer);
                                cse4589_print_and_log("[IP:END]\n");
                            } 
                            else
                                cse4589_print_and_log("[IP:ERROR]\n");
						}
                        else if((strcmp(server_args[0],"LIST"))==0)
						{
                            cse4589_print_and_log("[LIST:SUCCESS]\n");
                            fflush(stdout);
                            int print_index=1;
                            for(int i=0;i<4;i++){
                                if(client_list[i].struct_occupied==1){
                                    // char login[20];
                                    // if(client_list[i].status==1){
                                    //     strcpy(login,"logged-in");
                                    // }
                                    // else{
                                    //     strcpy(login,"logged-out");
                                    // }
                                    
                                // printf("%-5d%-35s%-20s%-8d\n", print_index, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", print_index++, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
                                }
                            }
                            cse4589_print_and_log("[LIST:END]\n");
                            fflush(stdout);
						}
                        else if((strcmp(server_args[0],"BLOCKED"))==0)
						{
                            int ip_index;
                            int print_index = 1;
                            if(valid_ip(server_args[1])&&(ip_index=ip_exist(server_args[1]))>=0){
                                printf("print blocking of %s\n",server_args[1]);
                                cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                                fflush(stdout);
                                for(int i=0;i<4;i++){
                                    if(strcmp(block_list_for_server[i].blocker.ip_addr,server_args[1])==0){
                                        //find block list of a client
                                        // cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                                        fflush(stdout);
                                        for(int j=0;j<3;j++){
                                            if(block_list_for_server[i].blocked[j].struct_occupied==1){
                                                cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", print_index++, block_list_for_server[i].blocked[j].hostname, block_list_for_server[i].blocked[j].ip_addr, block_list_for_server[i].blocked[j].client_port);
                                            }
                                        }
                                        // cse4589_print_and_log("[BLOCKED:END]\n");
                                        fflush(stdout);
                                        break;
                                    }
                                }
                                cse4589_print_and_log("[BLOCKED:END]\n");
                                fflush(stdout);
                            }
                            else{
                                cse4589_print_and_log("[BLOCKED:ERROR]\n");
                                fflush(stdout);
                                cse4589_print_and_log("[BLOCKED:END]\n");
                                fflush(stdout);
                            }
                            
						}
                        else if((strcmp(server_args[0],"STATISTICS"))==0)
						{
                            cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
                            fflush(stdout);
                            
							int print_index=1;
                            for(int i=0;i<4;i++){
                                if(client_list[i].struct_occupied==1){
                                    char login[20];
                                if(client_list[i].status==1){
                                    strcpy(login,"logged-in");
                                }
                                else{
                                    strcpy(login,"logged-out");
                                }
                                // printf("%-5d%-35s%-8d%-8d%-8s\n", print_index, client_list[i].hostname, client_list[i].msg_sent, client_list[i].msg_received, login);
                                cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", print_index++, client_list[i].hostname, client_list[i].msg_sent, client_list[i].msg_received, login);
                                }
                                
                            }
                            
                            cse4589_print_and_log("[STATISTICS:END]\n");
                            fflush(stdout);
						}
						free(cmd);
                        continue;
                    }
                    //new client is requesting connection
                    else if(sock_index == server_socket){
                        // printf("server socket has event\n");
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
                        // printf("new client accepted\n");
                        if(fdaccept < 0)
                            perror("Accept failed.");
                        char client_ip[BUFFER_SIZE];
                        // int client_port; 
	                    inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,client_ip, BUFFER_SIZE);
                        // fflush(stdout);
                       
                       
                        char client_port[PORT_SIZE];
                        memset(client_port, '\0', PORT_SIZE);
                        // printf("memset done\n");
                        // fflush(stdout);
                        //recv port number
                        int k = recv(fdaccept,client_port,PORT_SIZE,0);
                        // printf("recv port %d\n",client_port);
                        // fflush(stdout);
                        
                        int port = atoi(client_port);
                        
                        char hostname[HOST_SIZE];
                        
	                    getnameinfo((struct sockaddr *)&client_addr, caddr_len,hostname, HOST_SIZE, 0,0,0);
                        
                        printf("Client %s of IP %s with port %d connected!\n",hostname,client_ip,port);     
                        fflush(stdout);
                        struct client_record rec = {.id=client_count+1};
                        memset(&rec,'\0',sizeof(rec));
                        strcpy(rec.ip_addr,client_ip);
                        // printf("ip in record is %s\n",rec.ip_addr);
                        // int exist_index = ip_exist(client_ip);
                        
                            //new client
                        // printf("new client login\n");
                        
                        strcpy(rec.hostname , hostname);
                        
                        rec.client_port = port;
                        rec.msg_sent = 0;
                        rec.msg_received = 0;
                        rec.status=1;
                        rec.struct_occupied=1;
                        rec.sockfd = fdaccept;
                        //find a slot to init record of new client 
                        for(int i=0;i<4;i++){
                            if(client_list[i].struct_occupied!=1){
                                // printf("find %d-th position\n",i);
                                memcpy(&client_list[i],&rec,sizeof(struct client_record));
                                // printf("client record ip %s\n",client_list[i].ip_addr);
                                // printf("client record fd %d\n",client_list[i].sockfd);
                                break;
                            }
                        }
                        //init block list in block_list_for_server
                        for(int i=0;i<4;i++){
                            if(block_list_for_server[i].block_occupied!=1){
                                memset(&block_list_for_server[i],'\0',sizeof(struct block_list));
                                printf("find slot of block list\n");
                                block_list_for_server[i].block_occupied=1;
                                memcpy(&block_list_for_server[i].blocker,&rec,sizeof(struct client_record));
                                printf("host name in block list %s init done\n",block_list_for_server[i].blocker.ip_addr);
                                break;
                            }
                        }
                        //init message buffer for new client
                        for(int i=0;i<4;i++){
                            if(server_msg_buffers[i].buffer_occupied!=1){
                                printf("find slot of buffer list\n");
                                server_msg_buffers[i].buffer_occupied=1;
                                server_msg_buffers[i].sockfd = fdaccept;
                                strcpy(server_msg_buffers[i].receiverIP,rec.ip_addr);
                                printf("server msg buffer ip %s\n",server_msg_buffers[i].receiverIP);
                                break;
                            }
                        }
                        client_count++;
                        
                        
                        // if(strcmp(client_list[i].ip_addr,sender_ip)==0){
                        //     //increase sender's sending count
                        //     client_list[i].msg_sent+=1;
                              
                        // }
                        // printf("goint to sort\n");
                        if(client_count>1){
                            sort_client_list(client_list);
                            //print_client_list(rec.ip_addr);
                        }
                        // char buffer[BUFFER_SIZE];
                        
                        if(send(fdaccept,&client_list,sizeof(struct client_record)*4,0)==sizeof(struct client_record)*4){
                            printf("server send buffer to client\n");
                            fflush(stdout);
                        }
                        /* Add to watched socket list */
                        FD_SET(fdaccept, &server_master_list);
                        if(fdaccept > head_socket) head_socket = fdaccept;
                        continue;
                    }
                    //message from existing clients
                    else{
                        /* Initialize buffer to receieve response */
                        // printf("new operation received\n");
                        char *client_msg_buffer = (char*) malloc(sizeof(struct client_msg));
                        memset(client_msg_buffer, '\0', BUFFER_SIZE);
                        // char buffer[BUFFER_SIZE];
                        char cmd[CMD_SIZE];
                        char sender_ip[IP_SIZE];
                        char receiver_ip[IP_SIZE];
                        char client_message[BUFFER_SIZE];
                        if(recv(sock_index, client_msg_buffer, sizeof(struct client_msg), 0) <= 0){
                            
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");
                            
                            FD_CLR(sock_index, &server_master_list);
                        }
                        else {
                            printf("old client\n");
                        	//Process incoming data from existing clients here ...
                            
                            memcpy(cmd,client_msg_buffer,CMD_SIZE);
                            memcpy(sender_ip,client_msg_buffer+CMD_SIZE,IP_SIZE);
                            memcpy(receiver_ip,client_msg_buffer+CMD_SIZE+IP_SIZE,IP_SIZE);
                            memcpy(client_message,client_msg_buffer+CMD_SIZE+IP_SIZE*2,BUFFER_SIZE);
                            printf("sender ip is %s\n",sender_ip);
                            printf("sender cmd is %s\n",cmd);
                            printf("receiver ip is %s\n",receiver_ip);
                            printf("sender msg is %s\n",client_message);
                            
                            if(strcmp(cmd,"LOGIN")==0){
                                fdaccept = get_receiver_fd(sender_ip);
                                printf("old client %s login back\n",sender_ip);
                                int exist_index = ip_exist(sender_ip);
                                printf("exist index is %d\n",exist_index);
                                printf("client %s current status %d\n",client_list[exist_index].ip_addr,client_list[exist_index].status);
                                if(client_list[exist_index].status == 1){
                                    continue;
                                }
                                else{
                                    client_list[exist_index].status = 1;
                                }
                                // client_list[exist_index].client_port = port;
                                
                                // client_list[exist_index].sockfd = fdaccept;
                                //send buffered message
                                // printf("goint to find login client\n");
                                //find login client
                                for(int j=0;j<4;j++){
                                    if(server_msg_buffers[j].buffer_occupied==1 && strcmp(server_msg_buffers[j].receiverIP,sender_ip)==0){
                                        // printf("find index of client\n");
                                        //find ip in msg buffers
                                        memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                        char buffer_msg_size[4];
                                        sprintf(buffer_msg_size,"%d",server_msg_buffers[j].buffered_message_size);
                                        // printf("beffered msg size converted\n");
                                        // itoa(server_msg_buffers[j].buffered_message_size,buffer_msg_size,sizeof(int));
                                        strcpy(server_respond_message.msg,buffer_msg_size);
                                        printf("client %s has %s messages buffered\n",sender_ip,server_respond_message.msg);
                                        if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                            for (int i=0;i<server_msg_buffers[j].buffered_message_size;i++){
                                                //send buffered messages
                                                memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                                server_respond_message.success=1;
                                                strcpy(server_respond_message.msg,server_msg_buffers[j].msgs[i]);
                                                strcpy(server_respond_message.sender_ip,server_msg_buffers[j].senders[i]);
                                                strcpy(server_respond_message.receiver_ip,server_msg_buffers[j].receiverIP);
                                                if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                                    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", server_respond_message.sender_ip, server_respond_message.receiver_ip, server_respond_message.msg);
                                                    fflush(stdout);
                                                    client_list[exist_index].msg_received++;
                                                }
                                            }
                                        }
                                        memset(&server_msg_buffers[j].senders,'\0',sizeof(server_msg_buffers[j].senders));
                                        memset(&server_msg_buffers[j].msgs,'\0',sizeof(server_msg_buffers[j].msgs));
                                        server_msg_buffers[j].buffered_message_size=0;
                                        server_msg_buffers[j].most_recent_empty_index=0;
                                    }
                                }
                                
                                if(send(fdaccept,&client_list,sizeof(struct client_record)*4,0)==sizeof(struct client_record)*4){
                                    printf("server send client list to client\n");
                                    fflush(stdout);
                                }
                                continue;
                            }
                        	else if(strcmp(cmd,"SEND")==0){
                                printf("server handling send\n");
                                fdaccept = get_receiver_fd(receiver_ip);
                                if(ip_exist(receiver_ip)<0){
                                //send back ack to sender, already block
                                    memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                    server_respond_message.success=0;
                                    if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                        printf("server send block response to client\n");
                                        fflush(stdout);
                                        continue;
                                    }
                                }
                                // printf("fd for receiver of send %d\n",fdaccept);
                                int is_blocked_by_sender=blocked_by_sender(receiver_ip,sender_ip);
                                printf("%s is blocked by %s: %d\n",receiver_ip,sender_ip,is_blocked_by_sender);
                                cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                fflush(stdout);
                                
                                if(is_blocked_by_sender==0){
                                    //send
                                    for(int i=0;i<4;i++){
                                        if(strcmp(client_list[i].ip_addr,receiver_ip)==0){
                                            // printf("find receiver\n");
                                            printf("client login status is %d\n",client_list[i].status);
                                            if(client_list[i].status==1){
                                                //send directly
                                                memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                                server_respond_message.success=1;
                                                strcpy(server_respond_message.sender_ip,sender_ip);
                                                strcpy(server_respond_message.msg,client_message);
                                                strcpy(server_respond_message.receiver_ip,receiver_ip);
                                                if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                                    //printf("server send %s from %s to %s\n",server_respond_message.msg,server_respond_message.sender_ip,server_respond_message.receiver_ip);
                                                    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", server_respond_message.sender_ip, server_respond_message.receiver_ip, server_respond_message.msg);
                                                    fflush(stdout);
                                                    client_list[i].msg_received+=1;
                                                }
                                            }
                                            else{
                                                //buffer it
                                                //print_buffer_msgs(client_list[i].ip_addr);
                                                printf("message send will be buffered\n");
                                                for(int j=0;j<4;j++){
                                                    if(strcmp(server_msg_buffers[j].receiverIP,receiver_ip)==0){
                                                        //find ip in msg buffers
                                                        strcpy(server_msg_buffers[j].msgs[server_msg_buffers[j].most_recent_empty_index],client_message);
                                                        strcpy(server_msg_buffers[j].senders[server_msg_buffers[j].most_recent_empty_index],sender_ip);
                                                        // printf("buffered %s from %s to %s\n",server_msg_buffers[j].msgs[server_msg_buffers[j].most_recent_empty_index],server_msg_buffers[j].senders[server_msg_buffers[j].most_recent_empty_index],server_msg_buffers[j].receiverIP);
                                                        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",server_msg_buffers[j].senders[server_msg_buffers[j].most_recent_empty_index],server_msg_buffers[j].receiverIP, server_msg_buffers[j].msgs[server_msg_buffers[j].most_recent_empty_index]);
                                                        server_msg_buffers[j].most_recent_empty_index++;
                                                        server_msg_buffers[j].buffered_message_size++;
                                                        
                                                    }
                                                }
                                            }
                                            //increase receiver's receive count
                                            
                                        }
                                        if(strcmp(client_list[i].ip_addr,sender_ip)==0){
                                            //increase sender's sending count
                                            client_list[i].msg_sent+=1;
                                        }
                                    }
                                }
                                cse4589_print_and_log("[RELAYED:END]\n");
                                fflush(stdout);
                                //else do nothing
                                continue;
                            }
                            else if(strcmp(cmd,"BROADCAST")==0){
                                //want to find ip addresses not sender and not blocked by sender
                                // printf("broadcast\n");
                                printf("sender ip is %s\n",sender_ip);
                                printf("msg is %s\n",client_message);
                                int unblocked_count=0;
                                for(int i=0;i<4;i++){
                                    int is_blocked_by_sender=-1;
                                    //i loop finds all ip addr
                                    if(client_list[i].struct_occupied==1&&strcmp(sender_ip,client_list[i].ip_addr)!=0){
                                        //client list i is an ip addr and it is not the sender
                                        printf("check block for broadcast of %s\n",client_list[i].ip_addr);
                                        is_blocked_by_sender = blocked_by_sender(client_list[i].ip_addr,sender_ip);
                                        printf("%s is blocked by %s: %d\n",client_list[i].ip_addr,sender_ip,is_blocked_by_sender);
                                        if(is_blocked_by_sender==0){
                                            //need to check if this ip is logged out
                                            if(client_list[i].status==1){
                                                //send directly
                                                memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                                server_respond_message.success=1;
                                                strcpy(server_respond_message.sender_ip,sender_ip);
                                                strcpy(server_respond_message.msg,client_message);
                                                strcpy(server_respond_message.receiver_ip,client_list[i].ip_addr);
                                                fdaccept = get_receiver_fd(server_respond_message.receiver_ip);
                                                if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                                    
                                                    fflush(stdout);
                                                    client_list[i].msg_received++;
                                                }
                                            }
                                            else{
                                                printf("buffer broadcast\n");
                                                //buffer the message
                                                for(int j=0;j<4;j++){
                                                    // printf("receiver ip is %s\n",server_msg_buffers[j].receiverIP);
                                                    if(strcmp(server_msg_buffers[j].receiverIP,client_list[i].ip_addr)==0){
                                                        //find ip in msg buffers
                                                        strcpy(server_msg_buffers[j].msgs[server_msg_buffers[j].most_recent_empty_index],client_message);
                                                        strcpy(server_msg_buffers[j].senders[server_msg_buffers[j].most_recent_empty_index],sender_ip);
                                                        printf("buffered %s from %s to %s\n",server_msg_buffers[j].msgs[server_msg_buffers[j].most_recent_empty_index],server_msg_buffers[j].senders[server_msg_buffers[j].most_recent_empty_index],server_msg_buffers[j].receiverIP);
                                                        server_msg_buffers[j].most_recent_empty_index++;
                                                        server_msg_buffers[j].buffered_message_size++;
                                                    }
                                                }
                                            }
                                            unblocked_count++;
                                        }
                                        
                                        
                                    }
                                    else if(strcmp(sender_ip,client_list[i].ip_addr)==0){
                                        client_list[i].msg_sent+=unblocked_count;
                                    }
                                }
                                cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                char* broadcast_receiver="255.255.255.255";
                                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", sender_ip, broadcast_receiver , client_message);
                                cse4589_print_and_log("[RELAYED:END]\n");
                                continue;
                            }
                            else if(strcmp(cmd,"REFRESH")==0){
                                fdaccept = get_receiver_fd(sender_ip);
                                printf("server receive refresh\n");
                                // printf("to socket %d\n",fdaccept);
                                if(fdaccept>0){
                                    if(send(fdaccept,&client_list,sizeof(client_list),0)==sizeof(client_list)){
                                        printf("server send buffer to client\n");
                                        fflush(stdout);
                                    }
                                }
                                continue;
                            }
                            else if(strcmp(cmd,"BLOCK")==0){
                                printf("server receive block\n");
                                fdaccept = get_receiver_fd(sender_ip);
                                char block_ip[IP_SIZE];
                                strcpy(block_ip,client_message);
                                printf("blocking ip %s\n",block_ip);
                                int ex = ip_exist(block_ip);
                                printf("ip exist %d\n",ex);
                                if(ex<0){
                                //send back ack to sender, already block
                                    memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                    server_respond_message.success=0;
                                    if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                        printf("server send block response to client\n");
                                        fflush(stdout);
                                        continue;
                                    }
                                }
                                
                                int is_blocked_by_sender=0;
                                for (int j=0;j<4;j++){
                                    if(block_list_for_server[j].block_occupied==1&&strcmp(sender_ip,block_list_for_server[j].blocker.ip_addr)==0){
                                        //found the block list of sender
                                        printf("found block list for %s\n",sender_ip);
                                        int first_empty_slot=-1;
                                        for(int k=0;k<3;k++){
                                            if(block_list_for_server[j].blocked[k].struct_occupied==1&&strcmp(block_list_for_server[j].blocked[k].ip_addr,block_ip)==0){
                                                //clientlist[i] is blocked by sender
                                                printf("is blocked!\n");
                                                is_blocked_by_sender=1;
                                            }
                                            else if(block_list_for_server[j].blocked[k].struct_occupied==0&&first_empty_slot<0){
                                                //find empty slot for block
                                                first_empty_slot=k;
                                                printf("first empty slot is %d\n",first_empty_slot);
                                            }
                                        }
                                        // printf("is blocked by sender: %d\n",is_blocked_by_sender);
                                        if(is_blocked_by_sender==0){
                                            printf("trying to block\n" );
                                            //block it
                                            for(int k=0;k<4;k++){
                                                    // printf("occupied is %d\n",client_list[k].struct_occupied);
                                                    // printf("client's ip is %s\n",client_list[k].ip_addr);
                                                
                                                if(client_list[k].struct_occupied==1&&strcmp(client_list[k].ip_addr,block_ip)==0){
                                                    // printf("find info for client that will be bloced");
                                                    printf("%s blocking %s\n",block_list_for_server[j].blocker.ip_addr,client_list[k].ip_addr);
                                                    //find the client in client_list that will be blocked and copy it to block list
                                                    memcpy(&block_list_for_server[j].blocked[first_empty_slot],&client_list[k],sizeof(struct client_record));
                                                    printf("%s blocked\n",block_list_for_server[j].blocked[first_empty_slot].ip_addr);
                                                    break;
                                                }
                                                
                                            }
                                            sort_block_list_of_server(sender_ip);
                                        }
                                        printf("is blocked: %d\n",is_blocked_by_sender);
                                        printf("set response message for block\n");
                                        memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                        if(is_blocked_by_sender==1){
                                        //send back ack to sender, already block
                                            server_respond_message.success=0;
                                        }
                                        else{
                                            server_respond_message.success=1;
                                            //send back ack to sender, will be blocked block
                                        }
                                        
                                        if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                            printf("server send block response to client\n");
                                            fflush(stdout);
                                        }
                                        break;
                                    }
                                    
                                }
                                printf("printing block list for %s:\n",sender_ip);
                                print_block_lists(sender_ip);
                                printf("print block list done\n");
                                continue;
                            }
                            else if(strcmp(cmd,"UNBLOCK")==0){
                                printf("server handling unblock\n");
                                fdaccept = get_receiver_fd(sender_ip);
                                char unblock_ip[IP_SIZE];
                                strcpy(unblock_ip,client_message);
                                int ex = ip_exist(unblock_ip);
                                printf("ip exist %d\n",ex);
                                if(ex<0){
                                //send back ack to sender, already block
                                    memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                    server_respond_message.success=0;
                                    if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                        printf("server send block response to client\n");
                                        fflush(stdout);
                                        continue;
                                    }
                                }
                                int is_blocked_by_sender=0;
                                for (int j=0;j<4;j++){
                                    if(block_list_for_server[j].block_occupied==1&&strcmp(sender_ip,block_list_for_server[j].blocker.ip_addr)==0){
                                        //found the block list of sender
                                        int blocked_index=-1;
                                        for(int k=0;k<3;k++){
                                            if(block_list_for_server[j].block_occupied==1&&strcmp(block_list_for_server[j].blocked[k].ip_addr,unblock_ip)==0){
                                                //clientlist[i] is blocked by sender
                                                is_blocked_by_sender=1;
                                                blocked_index=k;
                                                printf("blocked index is: %d\n",blocked_index);
                                            }
                                        }
                                        // printf("blocked index is %d\n",blocked_index);
                                        if(is_blocked_by_sender==1){
                                            //unblock it
                                            memset(&block_list_for_server[j].blocked[blocked_index],'\0',sizeof(struct client_record));
                                            block_list_for_server[j].blocked[blocked_index].struct_occupied=0;
                                        }
                                        memset(&server_respond_message,'\0',sizeof(server_respond_message));
                                        if(is_blocked_by_sender==0){
                                        //send back ack to sender, target client not blocked
                                            server_respond_message.success=0;
                                        }
                                        else{
                                            //send back ack to sender, will be unblocked
                                            server_respond_message.success=1;
                                        }
                                        if(send(fdaccept,&server_respond_message,sizeof(server_respond_message),0)==sizeof(server_respond_message)){
                                            printf("server send unblock response to client\n");
                                            fflush(stdout);
                                        }
                                        break;
                                    }
                                    
                                }
                                printf("printing block list for %s:\n",sender_ip);
                                print_block_lists(sender_ip);
                                printf("print block list done\n");
                                continue;
                            }
                            else if(strcmp(cmd,"LOGOUT")==0){
                                printf("servre handle LOGOUT\n");
                                for(int i=0;i<4;i++){
                                    if(client_list[i].struct_occupied==1 && strcmp(client_list[i].ip_addr,sender_ip)==0){
                                        // printf("find sender\n");
                                        client_list[i].status=0;
                                        printf("client record ip %d logged out\n",client_list[i].ip_addr);
                                        break;
                                    }
                                }
                                continue;
                            }
                            
                            else if(strcmp(cmd,"EXIT")==0){
                                close(sock_index);
                                printf("Remote Host %s terminated connection!\n",sender_ip);
                                //delete client in client_list
                                for(int i=0;i<4;i++){
                                    if(strcmp(client_list[i].ip_addr,sender_ip)==0){
                                        printf("find exciting client\n");
                                        memset(&client_list[i],'\0',sizeof(struct client_record));
                                        client_list[i].struct_occupied=0;
                                        printf("client %s record in client_list deleted\n",sender_ip);
                                        break;
                                    }
                                }
                                //delete client in any other clients' block list
                                for(int i=0;i<4;i++){
                                    if(block_list_for_server[i].block_occupied==1&&strcmp(block_list_for_server[i].blocker.ip_addr,sender_ip)==0){
                                        printf("find exiting client in block list\n");
                                        
                                        memset(&block_list_for_server[i],'\0',sizeof(struct block_list));
                                        printf("client's block list removed\n");
                                        block_list_for_server[i].block_occupied=0;
                                    }
                                    else if(block_list_for_server[i].block_occupied==1){//check if exiting client is in other client's block list
                                        for(int j=0;j<3;j++){
                                            if(strcmp(block_list_for_server[i].blocked[j].ip_addr,sender_ip)==0){
                                                memset(&block_list_for_server[i].blocked[j],'\0',sizeof(struct client_record));
                                                block_list_for_server[i].blocked[j].struct_occupied=0;
                                            }
                                        }
                                    }
                                    
                                }  
                                //delete msg buffer for exiting client
                                for(int i=0;i<4;i++){
                                    if(server_msg_buffers[i].buffer_occupied==1 && strcmp(server_msg_buffers[i].receiverIP,sender_ip)==0){
                                        memset(&server_msg_buffers[i],'\0',sizeof(struct server_buffered_msg));
                                    }
                                    break;
                                } 
                                FD_CLR(sock_index, &server_master_list);
                                printf("all exiting client record cleared\n");
                                continue;
                            }
							printf(" client request done \n++++++++++++++++++++++++++++++++++++++\n");
							// if(send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
							// 	printf("Done!\n");
							//fflush(stdout);
                        }

                        free(client_msg_buffer);
                        // free(buffer);
                    }
                }
            }
        }
    }
}
void sighandler(int sig_num) 
{ 
    // Reset handler to catch SIGTSTP next time 
    signal(SIGTSTP, sighandler); 
    printf("run mode %d\n",run_mode);
    printf("***execute Ctrl+Z\n"); 
    if(run_mode==0){
        //client
        close(client_bind);
    }
    else{
        //server
        struct server_respond_msg server_last_message;
        memset(&server_last_message,'\0',sizeof(server_last_message));
        strcpy(server_last_message.msg,"server_will_exit");
        for(int i=0;i<4;i++){
            if(client_list[i].struct_occupied==1){
                if(send(client_list[i].sockfd,&server_last_message,sizeof(server_last_message),0)==sizeof(server_last_message)){
                    printf("server notify exit to client\n");
                    fflush(stdout);
                    close(client_list[i].sockfd);
                }
                
            }
        }
        close(server_socket);
    }
    exit(0);
} 
int main(int argc, char **argv){
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	//comment 01
    signal(SIGTSTP, sighandler); 
	if (argc != 3) {
		printf("Must be 3 args\n");
		return 1;
	}
	if(strcmp(argv[1], "s")==0)
	{
        run_mode=1;
		server_mode(atoi(argv[2]));
	}
	else if(strcmp(argv[1], "c")==0)
	{
        run_mode=0;
        strcpy(c_port,argv[2]);
		client_mode(atoi(argv[2]));
	}
	else
	{
		printf("Exit.");
		return -1;
	}
	return 0;
}

int connect_to_server(char *server_ip, int server_port,char* client_port){
    int fdsocket, len;
    struct sockaddr_in server_addr;
    // printf("server ip is %s\n",server_ip);
    // printf("server port is %d\n",server_port);
    // printf("client port is %d\n",client_port);
    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(fdsocket < 0)
       perror("Failed to create socket");
    // printf("server socket for host created\n");
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    printf("server port %d\n",server_port);
    printf("server IP %s\n",server_ip);
    server_addr.sin_port = htons(server_port);
    printf("trying to connect\n");
    if(connect(fdsocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        perror("Connect failed");
	else
		printf("Connected to server\n");
    
    int s = send(fdsocket,client_port, sizeof(client_port),0);
    
    // printf("send port %d\n",s);
    // printf("connection test done\n");
    return fdsocket;
}
int client_bind_socket(int client_port) {
    int fdsocket, len;
    struct sockaddr_in client_addrs;
    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
    //set socket to be reusable
    int enable = 1;
    if (setsockopt(fdsocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("make socket resuable failed");
    if (fdsocket < 0){
        perror("Cannot create socket");
        return 0;
    }
    client_addrs.sin_family = AF_INET;
    client_addrs.sin_addr.s_addr = INADDR_ANY;
    client_addrs.sin_port = htons(client_port);

    if (bind(fdsocket, (struct  sockaddr*)&client_addrs, sizeof(struct sockaddr_in)) < 0){
        printf("Client bind socket to port fail\n");
        return fdsocket;
    }
    //client listen in p2p
    if(listen(fdsocket, BACKLOG) < 0)
    	perror("Unable to listen on port");
    printf("Server start listening\n");
    // printf("Client bind socket to port successful\n");
    return fdsocket;
    
}
int command_to_list(char* cmd,char** res){
    char delim[] = " ";
    char *ptr = strtok(cmd, delim);

}
int get_host_ip(char* buffer){
    char* google_dns_server = "8.8.8.8";
    int googel_dns_port = 53;
    struct sockaddr_in google_addr;
    struct sockaddr_in host_addr;
    socklen_t host_addr_length = sizeof(host_addr);
    int sock = socket ( AF_INET, SOCK_DGRAM, 0);
     
    if(sock < 0)
        perror("Socket error");
    
    memset( &host_addr, 0, sizeof(host_addr) );
    //fill google connection information
    google_addr.sin_family = AF_INET;
    google_addr.sin_addr.s_addr = inet_addr( google_dns_server );
    google_addr.sin_port = htons( googel_dns_port );
    //connect with google
    int google_connection = connect( sock , (struct sockaddr*) &google_addr , sizeof(google_addr) );
    int get_socket_name = getsockname(sock, (struct sockaddr*) &host_addr, &host_addr_length);
         
    const char* ip = inet_ntop(AF_INET, &host_addr.sin_addr, buffer, BUFFER_SIZE);
    strcpy(ip_for_client,buffer);
    gethostname(host_name,  BUFFER_SIZE);
    printf("Host name: %s\n", host_name);
    close(sock);     
    if(ip != NULL)
        return 1;
    
    else
        return 0;
}
int validate_number(char *str) {
    //used code in https://www.tutorialspoint.com/c-program-to-validate-an-ip-address
   while (*str) {
      if(!isdigit(*str)){ 
         return 0;
      }
      str++; 
   }
   return 1;
}
int valid_ip(char* ip_address){
    //used code in https://www.tutorialspoint.com/c-program-to-validate-an-ip-address
    char shadow_IP[IP_SIZE];
    strcpy(shadow_IP,ip_address);
    int i, num, dots = 0;
    char *ptr;
    if (shadow_IP == NULL)
        return 0;
        ptr = strtok(shadow_IP, ".");
        if (ptr == NULL)
            return 0;
    // printf("shadow ip not null checked\n");
    while (ptr) {
        // printf("checking valid number\n");
        if (!validate_number(ptr)) 
            return 0;
        num = atoi(ptr); 
        if (num >= 0 && num <= 255) {
            // printf("in number range passing\n");
            ptr = strtok(NULL, "."); 
            if (ptr != NULL)
                dots++; 
        } 
        else
            return 0;
    }
    // printf("exit while\n");
    if (dots != 3) 
        return 0;
    // printf("ip is valid\n");
    return 1;
	
}
int valid_port(char* port){
    for (int i = 0; i < strlen(port); i++)
        if (isdigit(port[i]) == 0)
            return 0;
    if(atoi(port)>65535||atoi(port)<0)
        return 0;
    return 1;
}
int ip_exist(char* client_ip){
    // printf("in ip_exist\n");
    char shadow_ip[IP_SIZE];
    strcpy(shadow_ip,client_ip);
    int index=-1;
    // printf("shadow ip is %s\n",shadow_ip);
    for(int i=0;i<4;i++){
        printf("checking ip %s\n",client_list[i].ip_addr);
        if(strcmp(client_list[i].ip_addr, shadow_ip) == 0){
            // printf("ip match\n");
            index=i;
            return index;
        }
    }
    return index;
}
void sort_client_list(struct client_record client_list[4]){
    for (int i=0;i<3;i++){
        for (int j=i;j<4;j++){
            if(client_list[i].struct_occupied==1&&client_list[j].struct_occupied==1){
                if(client_list[i].client_port>client_list[j].client_port){
                    struct client_record tmp = client_list[i];
                    memcpy(&tmp,&client_list[i],sizeof(struct client_record));
                    memcpy(&client_list[i],&client_list[j],sizeof(struct client_record));
                    memcpy(&client_list[j],&tmp,sizeof(struct client_record));
                    
                }
            }
            
        }
    }
}
int blocked_by_sender(char* blocker_ip,char* blocked_ip){
    int is_blocked_by_sender=0;
    printf("-------------------------\n");
    for (int j=0;j<4;j++){
        if(block_list_for_server[j].block_occupied==1&&strcmp(blocker_ip,block_list_for_server[j].blocker.ip_addr)==0){
            //found the block list of sender
            for(int k=0;k<3;k++){
                // if(block_list_for_server[j].block_occupied==1){
                //     printf("checking block of %s\n",block_list_for_server[j].blocked[k].ip_addr);
                // }
                if(block_list_for_server[j].block_occupied==1&&strcmp(block_list_for_server[j].blocked[k].ip_addr,blocked_ip)==0){
                    //clientlist[i] is blocked by sender
                    return 1;
                }
            }
            break;
        }
    }
    printf("-------------------------\n");
    return 0;
}
void sort_block_list_of_server(char* sender_ip){
    for(int m=0;m<4;m++){
        if(strcmp(block_list_for_server[m].blocker.ip_addr,sender_ip)==0){
            //find sender
            for (int i=0;i<2;i++){
                for (int j=i;j<3;j++){
                    if(block_list_for_server[m].blocked[i].struct_occupied==1&&block_list_for_server[m].blocked[j].struct_occupied==1){
                        if(block_list_for_server[m].blocked[i].client_port>block_list_for_server[m].blocked[j].client_port){
                            struct client_record tmp;
                            memcpy(&tmp,&block_list_for_server[m].blocked[i],sizeof(struct client_record));
                            memcpy(&block_list_for_server[m].blocked[i],&block_list_for_server[m].blocked[j],sizeof(struct client_record));
                            memcpy(&block_list_for_server[m].blocked[j],&tmp,sizeof(struct client_record));
                            
                        }
                    }
                    
                }
            }
        }
        
    }
    
}
void print_client_list(char* client_ip){
    int print_index=1;
    for(int i=0;i<4;i++){
        if(client_list[i].struct_occupied==1){
            printf("%-5d%-35s%-20s%-8d\n", print_index++, client_list[i].hostname, client_list[i].ip_addr, client_list[i].client_port);
            
        }
    }
                            
}
void print_buffer_msgs(char* client_ip){
    
    //print all bufferd messages for a client
    for(int i=0;i<4;i++){
        if(server_msg_buffers[i].buffer_occupied==1&&strcmp(client_ip,server_msg_buffers[i].receiverIP)==0){
            // printf("find buffer list\n");
            for(int j=0;j<server_msg_buffers[i].buffered_message_size;j++){
                printf("msg from:%s, to:%s\n[msg]:%s\n", server_msg_buffers[i].senders[j], server_msg_buffers[i].receiverIP, server_msg_buffers[i].msgs[j]);
            }
        }
    }
}
void print_all_buffer_msgs(){
    
    //print all bufferd messages for a client
    for(int i=0;i<4;i++){
        //printf("find buffer list\n");
        printf("-------------------------------------\n");
        for(int j=0;j<server_msg_buffers[i].buffered_message_size;j++){
            if(server_msg_buffers[i].buffer_occupied==1){
                printf("msg from:%s, to:%s\n[msg]:%s\n", server_msg_buffers[i].senders[j], server_msg_buffers[i].receiverIP, server_msg_buffers[i].msgs[j]);
                printf("+++++++++++++++++++++++++++++++++++++++++\n");
            }
            
        }
        
    }
}
void print_block_lists(char* client_ip){
    // printf("print block list for %s\n",client_ip);
    //print block list 
    for(int i=0;i<4;i++){
        if(block_list_for_server[i].block_occupied==1&&strcmp(client_ip,block_list_for_server[i].blocker.ip_addr)==0){
            printf("find blocker\n");
            for(int j=0;j<3;j++){
                if(block_list_for_server[i].blocked[j].struct_occupied==1){
                    printf("---------\n");
                    printf("%s is blocking %s\n",block_list_for_server[i].blocker.ip_addr, block_list_for_server[i].blocked[j].ip_addr);
                }
            }

        }
    }
                            
}
void print_all_block_lists(){
    
    //print block list 
    for(int i=0;i<4;i++){
        printf("------------------------------------------\n");
        for(int j=0;j<3;j++){
            if(block_list_for_server[i].blocked[j].struct_occupied==1){
                printf("%s is blocking %s\n",block_list_for_server[i].blocker.ip_addr, block_list_for_server[i].blocked[j].ip_addr);
            }
        }

        
    }
                            
}
int get_receiver_fd(char* client_ip){
    printf("get receiver fd checks ip %s\n",client_ip);
    for(int i=0;i<4;i++){
        // printf("list occupied %d, ip is %s\n",client_list[i].struct_occupied,client_list[i].ip_addr);
        if(client_list[i].struct_occupied==1 && strcmp(client_list[i].ip_addr,client_ip)==0){
            printf("get socket %d\n",client_list[i].sockfd);
            return client_list[i].sockfd;
        }
    }
    printf("cannot find a socket\n");
    return -1;
}
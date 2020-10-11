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

#define TRUE 1
#define MSG_SIZE 256
#define BUFFER_SIZE 256
#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
char* client_ip;
char* server_ip;
char host_name[BUFFER_SIZE];
fd_set server_master_list, server_watch_list;
fd_set client_master_list, client_watch_list;
void client_mode(int client_port);
void server_mode(int server_port);
int connect_to_server(char *server_ip, int server_port);
int client_bind_socket(int client_port);
int command_to_list(char* cmd,char** res);
int get_host_ip(char* buffer);
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

void client_mode(int client_port){
	
    //client bind socket
    int client_bind = client_bind_socket(client_port);
    if(client_bind==0)
        exit(-1);
    printf("PA1-Client on\n");
    FD_ZERO(&client_master_list);
    FD_ZERO(&client_watch_list);
    FD_SET(STDIN, &client_master_list);
    int head_socket = STDIN;
    int selret,sock_index;
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
                    //new command from STDIN
                    if (sock_index == STDIN){
                    	char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);

                    	memset(cmd, '\0', CMD_SIZE);
                        fflush(stdout);	
						if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
                        char* pos;
                        if ((pos=strchr(cmd, '\n')) != NULL)
                            *pos = '\0';
						printf("Command is: %s\n", cmd);
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
							cse4589_print_and_log("I, xiyunxie, have read and understood the course academic integrity policy.\n");
							cse4589_print_and_log("[AUTHOR:END]\n");
						}
                        else if((strcmp(client_args[0],"IP"))==0)
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
                        else if((strcmp(client_args[0],"LOGIN"))==0)
						{
                            printf("login\n");
                            if (count != 3) {
                                printf("Login must have 3 args\n");
                                return;
                            }
							char* server_ip = client_args[1];
                            int server_port = atoi(client_args[2]);
                            printf("Connecting to server IP: %s with port %d\n",server_ip,server_port);
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"REFRESH"))==0)
						{
							
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"SEND"))==0)
						{
                            if (count != 3) {
                                printf("Send request must have 3 args\n");
                                return;
                            }
							char* receiver_ip = client_args[1];
                            char* msg = client_args[2];
                            printf("Sending '%s'to: %s\n",msg, receiver_ip);
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"BROADCAST"))==0)
						{
							if (count != 2) {
                                printf("Msg cannot be empty\n");
                                return;
                            }
                            char* msg = client_args[1];
                            printf("Sending '%s' broadcast\n",msg);
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"BLOCK"))==0)
						{
							if (count != 2) {
                                printf("Block must have 2 args\n");
                                return;
                            }
                            char* block_ip = client_args[1];
                            printf("Blocking %s\n", block_ip);
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"UNBLOCK"))==0)
						{
							if (count != 2) {
                                printf("Unblock must have 2 args\n");
                                return;
                            }
                            char* unblock_ip = client_args[1];
                            printf("Unblocking %s\n", unblock_ip);
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"LOGOUT"))==0)
						{
							
                            
                            printf("Logging out\n");
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
                        else if((strcmp(client_args[0],"EXIT"))==0)
						{
							
                            
                            printf("Exiting\n");
                            
							// cse4589_print_and_log("[IP:END]\n");
						}
						free(cmd);
                    }
                    //else server send a message
                    
                    else{
                        /* Initialize buffer to receieve response */
                        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);

                        if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");

                            /* Remove from watched list */
                            FD_CLR(sock_index, &server_master_list);
                        }
                        else {
                        	//Process incoming data from existing clients here ...

                        	
                        }

                        free(buffer);
                    }
                }
            }
        }
    }
}
void server_mode(int server_port){
    //running server
	printf("Server on\n");
	int port, server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in server_addr, client_addr;
	//fd_set master_list, watch_list;

	//server create socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
		perror("Cannot create socket");

	//fill socket address required information
	bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    //server socket bind with address
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )
    	perror("Server bind failed");

    //server start listening
    if(listen(server_socket, BACKLOG) < 0)
    	perror("Unable to listen on port");
    printf("Server start listening\n");
    /* ---------------------------------------------------------------------------- */

    /* Zero select FD sets */
    FD_ZERO(&server_master_list);
    FD_ZERO(&server_watch_list);
    
    /* Register the listening socket */
    FD_SET(server_socket, &server_master_list);
    /* Register STDIN */
    FD_SET(STDIN, &server_master_list);

    head_socket = server_socket;

    while(TRUE){
        memcpy(&server_watch_list, &server_master_list, sizeof(server_master_list));
		//fflush(stdout);

        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &server_watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");

        /* Check if we have sockets/STDIN to process */
        if(selret > 0){
            /* Loop through socket descriptors to check which ones are ready */
            for(sock_index=0; sock_index<=head_socket; sock_index+=1){

                if(FD_ISSET(sock_index, &server_watch_list)){

                    //new command from STDIN
                    if (sock_index == STDIN){
                    	char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);

                    	memset(cmd, '\0', CMD_SIZE);
						if(fgets(cmd, CMD_SIZE-1, STDIN) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);

						printf("\nI got: %s\n", cmd);
						
						//Process PA1 commands here ...

						free(cmd);
                    }
                    /* Check if new client is requesting connection */
                    else if(sock_index == server_socket){
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
                        if(fdaccept < 0)
                            perror("Accept failed.");

						printf("\nRemote Host connected!\n");                        

                        /* Add to watched socket list */
                        FD_SET(fdaccept, &server_master_list);
                        if(fdaccept > head_socket) head_socket = fdaccept;
                    }
                    /* Read from existing clients */
                    else{
                        /* Initialize buffer to receieve response */
                        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);

                        if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");

                            /* Remove from watched list */
                            FD_CLR(sock_index, &server_master_list);
                        }
                        else {
                        	//Process incoming data from existing clients here ...

                        	printf("\nClient sent me: %s\n", buffer);
							printf("ECHOing it back to the remote host ... ");
							if(send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
								printf("Done!\n");
							fflush(stdout);
                        }

                        free(buffer);
                    }
                }
            }
        }
    }
}
int main(int argc, char **argv){
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	//comment 01
	if (argc != 3) {
		printf("Must be 3 args\n");
		return 1;
	}
	if(strcmp(argv[1], "s")==0)
	{
		server_mode(atoi(argv[2]));
	}
	else if(strcmp(argv[1], "c")==0)
	{
		client_mode(atoi(argv[2]));
	}
	else
	{
		printf("Exit.");
		return -1;
	}
	return 0;
}

int connect_to_server(char *server_ip, int server_port){
    int fdsocket, len;
    struct sockaddr_in remote_server_addr;

    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(fdsocket < 0)
       perror("Failed to create socket");

    bzero(&remote_server_addr, sizeof(remote_server_addr));
    remote_server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &remote_server_addr.sin_addr);
    remote_server_addr.sin_port = htons(server_port);

    if(connect(fdsocket, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) < 0)
        perror("Connect failed");
	else
		printf("Connected to server\n");
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
    if (fdsocket < 0)
    {
        perror("Cannot create socket");
        return 0;
    }
    client_addrs.sin_family = AF_INET;
    client_addrs.sin_addr.s_addr = INADDR_ANY;
    client_addrs.sin_port = htons(client_port);

    if (bind(fdsocket, (struct  sockaddr*)&client_addrs, sizeof(struct sockaddr_in)) < 0){
        printf("Client bind socket to port fail\n");
        return 0;
    }
    printf("Client bind socket to port successful\n");
    return 1;
    
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
    
    gethostname(host_name,  BUFFER_SIZE);
    printf("Host name: %s\n", host_name);
    close(sock);     
    if(ip != NULL)
        return 1;
    
    else
        return 0;
}
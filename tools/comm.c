/*
 * Copyright (c) 2017/2018, Klaus Nienhaus, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "comm.h"

#define PORT 3490

//@brief:   Server side C function for TCP Socket Connections 
//          atm recieves data file from client (e.g. checkpoint)
int commserver(void)
{
    int server_fd, new_socket, valread; //data_size;
    struct sockaddr_in address;
    int opt = 1;
    int filesrecv = 0;
    int addrlen = sizeof(address);
    char buffer[1024]= {0};
    //char *data_name,*data_position;
    struct stat st = {0};
    
    comm_socket_header_t meta_data = {0};

    if (stat("checkpoint", &st) == -1)
		mkdir("checkpoint", 0700);

    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
     /* 
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");
    /*if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }*/


    while(1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        
        // recieving file metadata for positioning, name and size from client 
        recv(new_socket, (void*)&meta_data.data_name, sizeof(buffer), 0);
        recv(new_socket, (void*)&meta_data.data_size, sizeof(int), 0);
        recv(new_socket, (void*)&meta_data.data_position, sizeof(buffer), 0);
        printf("metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

        // hardcoded for testing purposes atm
        //strcpy(meta_data.data_name, "checkpoint/chk_config_copy.txt"); //"checkpoint/testcopy_Abrechnung.xlsm"
        

        // create file where data will be stored
        FILE *fp;
        fp = fopen(meta_data.data_name, "wb"); 
        if(NULL == fp)
        {
            printf("Error opening file");
            return 1;
        }

        // recieve data and write to file
        while(1)
        {
            // first read file in chunks of buffer bytes
            int recv_writen = 0;
            int valrecv = recv(new_socket , buffer, sizeof(buffer), 0);
            //printf("Bytes recieved %d \n", valrecv);        
            // if recv was success, write data to file.
            if(valrecv > 0)
            {
                //printf("Writing buffer to file \n");
                recv_writen += fwrite(buffer, sizeof(char), valrecv, fp);
              
            }

            
            // checking if everything was recieved or something is missing and an error occured 
            if (valrecv < sizeof(buffer))
            {
                fseek(fp, 0, SEEK_END); // seek to end of file
                int recieved_fsize = ftell(fp);   // get current file pointer
                printf("recieveddata: %d , %d, metafilesize: %d to filename: %s \n" , (recv_writen*sizeof(char)), recieved_fsize, meta_data.data_size, meta_data.data_name);
                if (recieved_fsize == meta_data.data_size)
                {
                    printf("finished transfer\n");
                    fflush(fp);
                    fclose(fp);
                    filesrecv++;
                    printf("files: %d \n",filesrecv);
                    
                    /*
                    char *msg = "file recieved";
                    if (send(new_socket, (void*)&msg, sizeof(buffer), 0)>0)
                    {
                        printf("send: %s", msg);
                    }*/
                }
                if (ferror(fp))
                {
                    printf("Error reading from socket\n");
                    perror("Reading buffer error");  
                }
                
                break;
            }
        }

        printf("finished file transfer closing socket and waiting for new connection\n");
        close(new_socket);
        sleep(1);
        if (filesrecv>=3) break;
    }

    //close(server_fd);
    return 0;
}

//@brief:   Client side C function for TCP Socket Connections 
//          atm sends data file to server (e.g. checkpoint)
int commclient(char *path)
{
    struct sockaddr_in address;
    int sock = 0, valread, length; //data_size;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    // char *data_name, *data_position;
    char *server_ip = "127.0.0.1";
    comm_socket_header_t meta_data;
    //char name_arg[1024];
    struct stat st = {0};

    //printf("start comm client \n");
    
    //printf("data_name_arg %s", argv[0]);
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    if (stat("checkpoint", &st) == -1)
		mkdir("checkpoint", 0700);

    //setting the file to be transfered into place
    //if (argv[0]>0)
    //    strcpy(meta_data.data_name,argv[0]);
    //else
        strcpy(meta_data.data_name, path); //"checkpoint/Abrechnung.xlsm" "checkpoint/chk_config.txt"
    strcpy(meta_data.data_position,"checkpoint");

    // open the file that we wish to transfer
    FILE *fp = fopen(meta_data.data_name,"rb");
    if(fp==NULL)
    {
        printf("File open error \n");
        return 1;   
    } 
    //int len = strlen((char*)&fp)+1;  
    fseek(fp, 0, SEEK_END); // seek to end of file and set current file to it
    //int filesize = ftell(fp);
    meta_data.data_size = ftell(fp);   // assign current filepointer (end of file), which is size of the file, to data_size
    rewind(fp); // set file pointer back to start of file
   
    // sending file meta information to server before sending file
    printf("metafilesize: %d to filename: %s and position: %s \n " , meta_data.data_size, meta_data.data_name, meta_data.data_position);
    int nsent = send(sock, (void*)&meta_data.data_name, sizeof(buffer), 0);
    nsent += send(sock, (void*)&meta_data.data_size, sizeof(int), 0);
    nsent += send(sock, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            return -1;
        } 

        
    // read data from file and send it
    while(1)
    {
         
        // reading file in chunks of buffer bytes
        int nread = fread(buffer,sizeof(char),sizeof(buffer),fp);
        //printf("Bytes read %d \n", nread);        

        // succesive reading leads to sending the data. 
        if(nread > 0)
        {
            //printf("Sending \n");
            send(sock, buffer, nread, 0);
        }

        
        // checking if end of file was reached or error occured     
        if (nread < sizeof(buffer))
        {
            if (feof(fp))
                printf("End of file\n");
                
            if (ferror(fp))
            {
                printf("Error reading from file\n");
                perror("Sending buffer error");
            }
            printf("Closing filedescriptor\n");
            fclose(fp);
            break;
        }

    }

    /*
    char msg[1024]; 
    while(1)
    {
        if (recv(sock, (void*)&msg, sizeof(buffer), 0)>0)
        {    
            printf("%s",msg);
            if (strncmp(msg,"file recieved", 14)==0)
            {
                printf("Server did validate: %s",msg);
                break;
            }
        }
    }*/
    
    close(sock);
    return 0;
}



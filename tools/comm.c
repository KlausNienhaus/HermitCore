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
#include <stddef.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/const.h>
#include <linux/kvm.h>

#include "comm.h"

#define PORT 3490

/// Page offset bits
#define PAGE_BITS			12
#define PAGE_2M_BITS	21
#define PAGE_SIZE			(1L << PAGE_BITS)
/// Mask the page address without page map flags and XD flag
#if 0
#define PAGE_MASK		((~0L) << PAGE_BITS)
#define PAGE_2M_MASK		(~0L) << PAGE_2M_BITS)
#else
#define PAGE_MASK			(((~0UL) << PAGE_BITS) & ~PG_XD)
#define PAGE_2M_MASK	(((~0UL) << PAGE_2M_BITS) & ~PG_XD)
#endif

#define PG_PSE			(1 << 7)

/// Disable execution for this page
#define PG_XD			(1L << 63)

static uint8_t* guest_mem = NULL;
//size_t* pgt;

//@brief:   Server side C function for TCP Socket Connections 
//          atm recieves data file from client (e.g. checkpoint)
int commserver(void)
{
    int server_fd, new_conn_fd, valread; //data_size;
    struct sockaddr_in address;
    int opt = 1, filesrecv = 0, addrlen = sizeof(address);
    char buffer[1024]= {0};
    //char *data_name,*data_position;
    struct stat st = {0};
    size_t location;
	int ret;
    
    comm_socket_header_t meta_data = {0};


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
      
    // attaching socket to the port
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


    while(1)
    {
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        
        // recieving file metadata for positioning, name and size from client 
        recv(new_conn_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
        recv(new_conn_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
        recv(new_conn_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
        printf("metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);


        if (meta_data.data_name=="memory" && meta_data.data_position=="clock")
        {
            struct kvm_clock_data clock = {};
            uint clocksize=sizeof(clock); 
            int nrecv = 0;
            while (nrecv<meta_data.data_size)
                nrecv += recv(new_conn_fd, (void*)&clock+nrecv, sizeof(clock)-nrecv,0);
        }


        if (meta_data.data_name=="memory" && meta_data.data_position=="NULL")
        {
            size_t pgdpgt, mem_chunck;
            uint masksize;

            int nrecv1 = 0;
            while (nrecv1<sizeof(size_t))
                nrecv1 += recv(new_conn_fd, (void*)pgdpgt+nrecv1, sizeof(size_t)-nrecv1,0);
            int nrecv2 = 0;
            while (nrecv2<sizeof(size_t))
                nrecv2 += recv(new_conn_fd, (void*)mem_chunck+nrecv2, sizeof(size_t)-nrecv2,0);
            int nrecv3 = 0;
            while (nrecv3<(sizeof(size_t)+sizeof(size_t)+sizeof(unsigned long)))
                nrecv3 += recv(new_conn_fd, (void*)masksize+nrecv3, sizeof(unsigned long)-nrecv3,0);
            if ((nrecv1+nrecv2+nrecv3)<(sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize)))
                perror("Memory Chunk incomplete");

        }

        if (meta_data.data_name=="register" && meta_data.data_position=="NULL")
        {
            size_t pgdpgt, mem_chunck;
            uint masksize;

            int nrecv = 0;
            nrecv += recv(new_conn_fd, (void*)pgdpgt, sizeof(size_t), 0);
            nrecv += recv(new_conn_fd, (void*)mem_chunck, sizeof(size_t), 0);
            nrecv += recv(new_conn_fd, (void*)masksize, sizeof(unsigned long), 0);
            if (nrecv<(sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize)))
                perror("Memory Chunk incomplete");

        }


 /*
        size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
        The function fread() reads nmemb items of data, each size bytes long, from the stream pointed to by stream, storing them at the location given by ptr.

        ssize_t recv(int sockfd, void *buf, size_t len, int flags);
  
       
(pgd+k,sizeof(size_t),(size_t*) (guest_mem + (pgd[k] & PAGE_2M_MASK)),(1UL << PAGE_2M_BITS));
(&pgt_entry,sizeof(size_t),(size_t*)(guest_mem + (pgt[l] & PAGE_MASK)),(1UL << PAGE_BITS));

while (fread(&location, sizeof(location), 1, f) == 1) {
			//printf("location 0x%zx\n", location);
			if (location & PG_PSE)
				ret = fread((size_t*) (mem + (location & PAGE_2M_MASK)), (1UL << PAGE_2M_BITS), 1, f);
			else
				ret = fread((size_t*) (mem + (location & PAGE_MASK)), (1UL << PAGE_BITS), 1, f);

			if (ret != 1) {
				fprintf(stderr, "Unable to read checkpoint: ret = %d", ret);
				err(1, "fread failed");
			}

if (masksize == (1UL << PAGE_2M_BITS))


    strcpy(meta_data.data_name, path); //"checkpoint/Abrechnung.xlsm" "checkpoint/chk_config.txt"
    strcpy(meta_data.data_position, position);
    if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);

*/  
        
        else
        {

        // hardcoded for testing purposes atm
        //strcpy(meta_data.data_name, "checkpoint/chk_config_copy.txt"); //"checkpoint/testcopy_Abrechnung.xlsm"
        if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);

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
            int valrecv = recv(new_conn_fd , buffer, sizeof(buffer), 0);
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
        }

        printf("finished file transfer closing socket and waiting for new connection\n");
        close(new_conn_fd);
        sleep(1);
        if (filesrecv>=3) 
        {
            printf("recieved all checkpoint files starting up system\n");
            break;
        }
    }

    close(server_fd);
    return 0;
}

//@brief:   Client side C function for TCP Socket Connections sending Memory Chunk
//          atm sends pgd or pgt with corresponding memory to server (e.g. checkpoint)

int comm_clock_client(struct kvm_clock_data clock, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0, valread; //data_size;
    char buffer[1024] = {0};
    char *serv_ip; // = "127.0.0.1";
    comm_socket_header_t meta_data;
    // char *data_name, *data_position;
    //char name_arg[1024];
    //struct stat st = {0};
     
    if (server_ip)
        strcpy(serv_ip, server_ip);
    else
    {
        printf("\nInvalid address/ Address not supported, falling back to loop adr \n");
        strcpy(serv_ip, "127.0.0.1");
    }
    //printf("start comm client \n");
    
    //printf("data_name_arg %s", argv[0]);
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = (sizeof(clock));
    int nsent = send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(clock), 0);
    nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            return -1;
        } 

    //printf("Sending \n");
    int nsent=0;
    while(nsent<sizeof(clock))
    {
        nsent += send(client_fd, (void*)&clock+nsent, sizeof(clock)-nsent, 0);
    }
    if (nsent < (sizeof(clock)))
        {
            perror("Clock not correct send \n");
            return -1;
        } 
   
    close(client_fd);
    return 0;
}


//@brief:   Client side C function for TCP Socket Connections sending Memory Chunk
//          atm sends pgd or pgt with corresponding memory to server (e.g. checkpoint)

int comm_chunk_client(size_t *pgdpgt, size_t *mem_chunck, unsigned long masksize, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0, valread; //data_size;
    char buffer[1024] = {0};
    char *serv_ip; // = "127.0.0.1";
    comm_socket_header_t meta_data;
    // char *data_name, *data_position;
    //char name_arg[1024];
    //struct stat st = {0};
     
    if (server_ip)
        strcpy(serv_ip, server_ip);
    else
    {
        printf("\nInvalid address/ Address not supported, falling back to loop adr \n");
        strcpy(serv_ip, "127.0.0.1");
    }
    //printf("start comm client \n");
    
    //printf("data_name_arg %s", argv[0]);
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = (sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize));
    int nsent = send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            return -1;
        } 

    //printf("Sending \n");
    int nsent1=0;
    while(nsent<sizeof(pgdpgt))
        nsent += send(client_fd, (void*)pgdpgt+nsent1, sizeof(size_t)-nsent1, 0);
    int nsent2=0;
    while(nsent2<(sizeof(pgdpgt)+sizeof(mem_chunck)))
        nsent2 += send(client_fd, (void*)mem_chunck+nsent2, sizeof(size_t)-nsent2, 0);
    int nsent3=0;
    while(nsent3<(sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize)))    
        nsent3 += send(client_fd, (void*)masksize+nsent3, sizeof(unsigned long)-nsent3, 0);
    if ((nsent1+nsent2+nsent3) < (sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize)))
        {
            perror("Memory not correct send not correct send \n");
            return -1;
        } 
   
    close(client_fd);
    return 0;
}



//@brief:   Client side C function for TCP Socket Connections 
//          atm sends data file to server (e.g. checkpoint)
int commclient(char *path, char *position, char *server_ip)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0, valread; //data_size;
    char buffer[1024] = {0};
    char *serv_ip; // = "127.0.0.1";
    comm_socket_header_t meta_data;
    // char *data_name, *data_position;
    //char name_arg[1024];
    struct stat st = {0};

    if (server_ip)
        strcpy(serv_ip, server_ip);
    else
    {
        printf("\nInvalid address/ Address not supported, falling back to loop adr \n");
        strcpy(serv_ip, "127.0.0.1");
    }
    //printf("start comm client \n");
    
    //printf("data_name_arg %s", argv[0]);
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }


    strcpy(meta_data.data_name, path); //"checkpoint/Abrechnung.xlsm" "checkpoint/chk_config.txt"
    strcpy(meta_data.data_position, position);
    if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);

    // open the file that we wish to transfer
    FILE *fp = fopen(meta_data.data_name,"rb");
    if(fp==NULL)
    {
        printf("File open error \n");
        return 1;   
    } 
     
    fseek(fp, 0, SEEK_END); // seek to end of file and set current file to it
    meta_data.data_size = ftell(fp);   // assign current filepointer (end of file), which is size of the file, to data_size
    rewind(fp); // set file pointer back to start of file
   
    // sending file meta information to server before sending file
    printf("metafilesize: %d to filename: %s and position: %s \n " , meta_data.data_size, meta_data.data_name, meta_data.data_position);
    int nsent = send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
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
            send(client_fd, buffer, nread, 0);
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
    
    close(client_fd);
    return 0;
}

/*
(pgd+k,sizeof(size_t),(size_t*) (guest_mem + (pgd[k] & PAGE_2M_MASK)),(1UL << PAGE_2M_BITS));
(&pgt_entry,sizeof(size_t),(size_t*)(guest_mem + (pgt[l] & PAGE_MASK)),(1UL << PAGE_BITS));

while (fread(&location, sizeof(location), 1, f) == 1) {
			//printf("location 0x%zx\n", location);
			if (location & PG_PSE)
				ret = fread((size_t*) (mem + (location & PAGE_2M_MASK)), (1UL << PAGE_2M_BITS), 1, f);
			else
				ret = fread((size_t*) (mem + (location & PAGE_MASK)), (1UL << PAGE_BITS), 1, f);

			if (ret != 1) {
				fprintf(stderr, "Unable to read checkpoint: ret = %d", ret);
				err(1, "fread failed");
			}

if (masksize == (1UL << PAGE_2M_BITS))


    strcpy(meta_data.data_name, path); //"checkpoint/Abrechnung.xlsm" "checkpoint/chk_config.txt"
    strcpy(meta_data.data_position, position);
    if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);




(pgd+k,sizeof(size_t),(size_t*) (guest_mem + (pgd[k] & PAGE_2M_MASK)),(1UL << PAGE_2M_BITS));
(&pgt_entry,sizeof(size_t),(size_t*)(guest_mem + (pgt[l] & PAGE_MASK)),(1UL << PAGE_BITS));

while (fread(&location, sizeof(location), 1, f) == 1) {
			//printf("location 0x%zx\n", location);
			if (location & PG_PSE)
				ret = fread((size_t*) (mem + (location & PAGE_2M_MASK)), (1UL << PAGE_2M_BITS), 1, f);
			else
				ret = fread((size_t*) (mem + (location & PAGE_MASK)), (1UL << PAGE_BITS), 1, f);

			if (ret != 1) {
				fprintf(stderr, "Unable to read checkpoint: ret = %d", ret);
				err(1, "fread failed");
			}

if (masksize == (1UL << PAGE_2M_BITS))


    strcpy(meta_data.data_name, path); //"checkpoint/Abrechnung.xlsm" "checkpoint/chk_config.txt"
    strcpy(meta_data.data_position, position);
    if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);

*/

/*
//@brief:   Client side C function for TCP Socket Connections 
//          atm sends data file to server (e.g. checkpoint)
int comm_register_client(char *path, char *position, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0, valread; //data_size;
    char buffer[1024] = {0};
    char *serv_ip; // = "127.0.0.1";
    comm_socket_header_t meta_data;
    // char *data_name, *data_position;
    //char name_arg[1024];
    //struct stat st = {0};
    size_t* pgd_pgt_entry, guest_mem;
    unsigned long size;
    char *comm_type = "register";

    if (server_ip)
        strcpy(serv_ip, server_ip);
    else
    {
        printf("\nInvalid address/ Address not supported, falling back to loop adr \n");
        strcpy(serv_ip, "127.0.0.1");
    }
    //printf("start comm client \n");
    
    //printf("data_name_arg %s", argv[0]);
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    //path in this case codes the type of transfer comming
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = (sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize));
    int nsent = send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    nsent += send(client_fd, (void*)meta_data.data_size, sizeof(uint), 0);
    nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            return -1;
        } 

    //printf("Sending \n");
    int nsent=0;
    while(nsent<sizeof(pgdpgt))
    {
        nsent += send(client_fd, (void*)pgdpgt, sizeof(size_t), 0);
    }
    while(nsent<(sizeof(pgdpgt)+sizeof(mem_chunck)))
    {
        nsent += send(client_fd, (void*)mem_chunck, sizeof(size_t), 0);
    }
    while(nsent<(sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize)))    
    {
        nsent += send(client_fd, (void*)masksize, sizeof(unsigned long), 0);
    }
    if (nsent < (sizeof(pgdpgt)+sizeof(mem_chunck)+sizeof(masksize)))
        {
            perror("Memory not correct send not correct send \n");
            return -1;
        } 
   
    close(client_fd);
    return 0;
}
*/

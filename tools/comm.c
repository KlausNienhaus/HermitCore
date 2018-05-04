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


#include "comm.h"


#define maxtry 50
#define retytime 50
//static uint8_t *guest_mem = NULL;
//size_t* pgt;

//@brief:   Server side C function for TCP Socket Connections 
//          atm recieves data file from client (e.g. checkpoint)
int commserver(void)
{
    int server_fd, new_conn_fd; //data_size;
    struct sockaddr_in address;
    int opt = 1, filesrecv = 0, addrlen = sizeof(address);
    char buffer[1024]= {0};
    struct stat st = {0};
    size_t location;
	int ret;
    comm_socket_header_t meta_data = {0};


    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0){
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");


    while(1)
    {
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
            perror("accept failed\n");
            exit(EXIT_FAILURE);
        }
        
        
        // recieving file metadata for positioning, name and size from client 
        int nrecv = 0;
        while (nrecv < sizeof(meta_data))
            nrecv += recv(new_conn_fd, (void*)((char*)&meta_data)+nrecv, sizeof(meta_data)-nrecv, 0);            
        if (nrecv < (sizeof(meta_data))){
            perror("Meta_data not correct recieved in commserver \n");
            exit(EXIT_FAILURE);
        }
    
        //strcpy(meta_data.data_name, "checkpoint/chk_config_copy.txt"); //"checkpoint/testcopy_Abrechnung.xlsm"
        if (stat(meta_data.data_position, &st) == -1)
		    mkdir(meta_data.data_position, 0700);

        // create file where data will be stored
        FILE *fp;
        fp = fopen(meta_data.data_name, "wb"); 
        if(NULL == fp){
            printf("Error opening file\n");
            exit(EXIT_FAILURE);
        }

        // recieve data and write to file
        while(1)
        {
            int recv_writen = 0;
            int valrecv = 0;
            while(valrecv > 0){
            // first read file in chunks of buffer bytes
            valrecv = recv(new_conn_fd , buffer, sizeof(buffer), 0);
            //printf("Bytes recieved %d \n", valrecv);        
            // if recv was success, write data to file.
                if (valrecv > 0)
                //printf("Writing buffer to file \n");
                recv_writen += fwrite(buffer, sizeof(char), valrecv, fp);
            }

            
            // checking if everything was recieved or something is missing and an error occured 
            if (valrecv < sizeof(buffer)){
                fseek(fp, 0, SEEK_END); // seek to end of file
                int recieved_fsize = ftell(fp);   // get current file pointer
                printf("recieveddata: %d , %d, metafilesize: %d to filename: %s \n" , (recv_writen*sizeof(char)), recieved_fsize, meta_data.data_size, meta_data.data_name);
                if (recieved_fsize == meta_data.data_size){
                    printf("finished transfer\n");
                    fflush(fp);
                    fclose(fp);
                    filesrecv++;
                    printf("files: %d \n",filesrecv);

                }
                if (ferror(fp)){
                    printf("Error reading from socket\n");
                    perror("Reading buffer error");  
                } 
                break;
            }
        }
        

        printf("finished file transfer closing socket and waiting for new connection\n");
        close(new_conn_fd);
        usleep(10);
        if (filesrecv>=3) {
            printf("recieved all checkpoint files starting up system\n");
            break;
        }
    }

    close(server_fd);
    return 0;
}


//@brief:   Client side C function for TCP Socket Connections 
//          atm sends data file to server (e.g. checkpoint)
int commclient(char *path, char *position, char *server_ip)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0; //data_size;
    char buffer[1024] = {0};
    comm_socket_header_t meta_data;

    struct stat st = {0};
    //printf("start comm client \n");

    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
    }


    strcpy(meta_data.data_name, path);
    strcpy(meta_data.data_position, position);
    if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);

    // open the file that we wish to transfer
    FILE *fp = fopen(meta_data.data_name,"rb");
    if(fp==NULL){
        printf("File open error \n");
        exit(EXIT_FAILURE);   
    } 
     
    fseek(fp, 0, SEEK_END); // seek to end of file and set current file to it
    meta_data.data_size = ftell(fp);   // assign current filepointer (end of file), which is size of the file, to data_size
    rewind(fp); // set file pointer back to start of file
   
    // sending file meta information to server before sending file
    int nsent=0;
    while (nsent<sizeof(meta_data))
        nsent += send(client_fd, (void*)((char*)&meta_data)+nsent, sizeof(meta_data)-nsent,0);
    if (nsent < (sizeof(meta_data))){
        perror("Meta_data not correct send \n");
        exit(EXIT_FAILURE);
    }

        
    // read data from file and send it
    while(1){
        int nread = 0;

        // reading file in chunks of buffer bytes
        nread = fread(buffer,sizeof(char),sizeof(buffer),fp);
        //printf("Bytes read %d \n", nread);        

        // succesive reading leads to sending. 
        if(nread > 0){
            int nsend = 0;
            while (nsend < sizeof(buffer))
                nsend+=send(client_fd, buffer, nread, 0);
            
        }
      
        // checking if end of file was reached or error occured     
        if (nread < sizeof(buffer)){
            if (feof(fp))
                printf("End of file\n");
            if (ferror(fp)){
                printf("Error reading from file\n");
                perror("Sending buffer error\n");
            }
            printf("Closing filedescriptor\n");
            fclose(fp);
            break;
        }

    }

    close(client_fd);
    return 0;
}


int comm_config_server(comm_config_t *checkpoint_config)
{
    int server_fd, new_conn_fd; //data_size;
    struct sockaddr_in address;
    int opt = 1, addrlen = sizeof(address);
    char buffer[1024]= {0};
    comm_socket_header_t meta_data = {0};

    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
     
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0){
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");



    if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept failed\n");
        exit(EXIT_FAILURE);
    }
    
    //recieving metadata for data type and control information
    int nrecv = 0;
    while (nrecv<sizeof(meta_data))
        nrecv += recv(new_conn_fd, (void*)((char*)&meta_data)+nrecv, sizeof(meta_data)-nrecv, 0);            
    if (nrecv < (sizeof(meta_data))){
        perror("Meta_data not correct received in register_server \n");
        exit(EXIT_FAILURE);
    }
    //printf("In comm_config_server recieved sizeof(checkpoint_config) %d, checkpoint_config.elf_entry %d, checkpoint_config.guest_size %d",sizeof(checkpoint_config), checkpoint_config.elf_entry , checkpoint_config.guest_size);

    if (strcmp(meta_data.data_name,"config")==0){
        int nrecv = 0;
        while (nrecv<sizeof(comm_config_t))
            nrecv += recv(new_conn_fd, ((char*)checkpoint_config)+nrecv, sizeof(comm_config_t)-nrecv, 0);
        //printf("config nrecv %d sizeof(comm_config_t) %d meta_size %d \n", nrecv ,sizeof(comm_config_t),meta_data.data_size);
        if (nrecv<sizeof(comm_config_t))
            perror("Register recieved incomplete\n");
    }else{
        printf("config_server wrong meta_data: Name %s, Position: %s in config recieved\n", meta_data.data_name, meta_data);
        close(new_conn_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    close(new_conn_fd);   

    printf("Config for migration recieved\n");
    close(server_fd);
    return 0;
}

//@brief:   Client side C function for TCP Socket Connections sending Config of checkpoint
//          atm sends Config to server (e.g. checkpoint)
int comm_config_client(comm_config_t *checkpoint_config, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0; //data_size;
    char buffer[1024] = {0};
    comm_socket_header_t meta_data;
    //comm_type = "config";
     
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    //usleep(1);
    int try=0;
    retry1:
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        if (try<maxtry){
                usleep(retytime);
                try++;
                goto retry1;
            }   
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    
    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = sizeof(comm_config_t);
      
    //sending metadata for data type and control information
    int nsent=0;
    while (nsent < sizeof(meta_data))
        nsent += send(client_fd, (void*)((char*)&meta_data)+nsent, sizeof(meta_data)-nsent,0);
    if (nsent < sizeof(meta_data)){
        perror("Meta_data not correct send in config_client\n");
        exit(EXIT_FAILURE);
    }

    nsent=0;
    while(nsent < sizeof(comm_config_t))
        nsent += send(client_fd, (void*)((char*)checkpoint_config)+nsent, sizeof(comm_config_t)-nsent, 0);
    if (nsent < sizeof(comm_config_t)){
        perror("Config for migration not correct send \n");
        exit(EXIT_FAILURE);
    }
  
    printf("Config for migration send \n");
    close(client_fd);
    return 0;
}


int comm_register_server(comm_register_t *vcpu_register, uint32_t *cpuid, uint32_t *ncores)
{
    int server_fd, new_conn_fd; //data_size;
    struct sockaddr_in address;
    int opt = 1, filesrecv = 0, addrlen = sizeof(address);
    char buffer[1024]= {0};
    comm_socket_header_t meta_data = {0};


    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
     
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0){
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");



    if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept failed\n");
        exit(EXIT_FAILURE);
    }
    
    //recieving metadata for data type and control information
    int nrecv = 0;
    while (nrecv < sizeof(meta_data))
        nrecv += recv(new_conn_fd, (void*)((char*)&meta_data)+nrecv, sizeof(meta_data)-nrecv, 0);            
    if (nrecv < sizeof(meta_data)){
        perror("Meta_data not correct received in register_server \n");
        exit(EXIT_FAILURE);
    }

    //printf("In register_server metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);
    //printf("In register_server size of ncores register: %d", *ncores*sizeof(comm_register_t));
    
    if(strcmp(meta_data.data_name,"register")==0){
        int nrecv = 0;
        while (nrecv < (*ncores*sizeof(comm_register_t)))
            nrecv += recv(new_conn_fd, (void*)((size_t)vcpu_register+nrecv), (*ncores*sizeof(comm_register_t))-nrecv, 0);
        if (nrecv < (*ncores*sizeof(comm_register_t)))
            perror("Register recieved incomplete\n");
    } else {
        printf("wrong meta_data %s in register recieved", meta_data.data_name);
        close(new_conn_fd);
        close(server_fd); 
        exit(EXIT_FAILURE);
    }
    close(new_conn_fd);     
        
    
    printf("Register for migration recieved\n");
    close(server_fd);
    return 0;
}


//@brief:   Comm function for sending VCPU register for checkpoint transfer
//          atm sends data file to server (e.g. checkpoint)
int comm_register_client(comm_register_t *vcpu_register,uint32_t *cpuid , uint32_t *ncores, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0; //data_size;
    char buffer[1024] = {0};
    comm_socket_header_t meta_data;
    //char *comm_type = "register";

    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    usleep(5);
    int try=0;
    retry2:
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        if (try<maxtry){
                usleep(retytime);
                try++;
                goto retry2;
            }   
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
    }

    //path in this case codes the type of transfer comming
    strcpy(meta_data.data_name, comm_type);
    //printf("Register meta data name, %s \n", meta_data.data_name);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size=(*ncores*sizeof(comm_register_t));
 
    //sending metadata for data type and control information
    int nsent=0;
    while (nsent < sizeof(meta_data))
        nsent += send(client_fd, (void*)((char*)&meta_data)+nsent, sizeof(meta_data)-nsent,0);
    if (nsent < (sizeof(meta_data))){
        perror("Meta_data not correct send in register_client\n");
        exit(EXIT_FAILURE);
    }

    nsent=0;
    while(nsent<(*ncores*sizeof(comm_register_t)))
        nsent += send(client_fd, (void*)((size_t)vcpu_register+nsent), (*ncores*sizeof(comm_register_t))-nsent, 0);
    if (nsent < (*ncores*sizeof(comm_register_t))){
            perror("Register not correct send \n");
            close(client_fd);
            exit(EXIT_FAILURE);
        } 
    
    //printf("In comm_register_client send size %d vcpu_register->regs %d vcpu_register->lapic %d\n",sizeof(*vcpu_register),vcpu_register->regs, vcpu_register->lapic);
    //printf("In comm_register_client send size %d vcpu_register[*cpuid].regs %d vcpu_register[*cpuid].lapic %d\n",vcpu_register[*cpuid].regs, vcpu_register[*cpuid].lapic);

    printf("Register for migration send\n");
    close(client_fd);
    return 0;
}



int comm_clock_server(struct kvm_clock_data *clock)
{
    int server_fd, new_conn_fd; //data_size;
    struct sockaddr_in address;
    int opt = 1, filesrecv = 0, addrlen = sizeof(address);
    char buffer[1024]= {0};
    comm_socket_header_t meta_data = {0};

    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0){
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("clock_server waiting on connection by listen \n");


    if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept failed\n");
        exit(EXIT_FAILURE);
    }
    
    //recieving metadata for data type and control information
    int nrecv = 0;
    while (nrecv < sizeof(meta_data))
        nrecv += recv(new_conn_fd, (void*)((char*)&meta_data)+nrecv, sizeof(meta_data)-nrecv, 0);            
    if (nrecv < (sizeof(meta_data))){
        perror("Meta_data not correct received in clock_server \n");
        exit(EXIT_FAILURE);
    }
    //printf("In clock_server metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

    if (strcmp(meta_data.data_name,"clock")==0){
        int nrecv = 0;
        while (nrecv<sizeof(*clock))
            nrecv += recv(new_conn_fd,(void*)((char*)clock)+nrecv, sizeof(*clock)-nrecv, 0);
        if (nrecv<(sizeof(*clock)))
            perror("Clock recieved incomplete\n");
    } else {
        printf("Wrong meta_data %s in clock recieved", meta_data.data_name);
        close(new_conn_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    close(new_conn_fd);


    printf("Clock for migration recieved\n");
    close(server_fd);
    return 0;
}

//@brief:   Client side C function for TCP Socket Connections sending Clock
//          atm sends clock to server (e.g. checkpoint)
int comm_clock_client(struct kvm_clock_data *clock, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0; //data_size;
    char buffer[1024] = {0};
    comm_socket_header_t meta_data;
    
    //printf("start comm client \n"); 
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0){
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    //needed to be add as otherwise server hasn't socket open in time
    usleep(40);
    int try=0;
    retry3:
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        if (try<maxtry){
                usleep(retytime);
                try++;
                goto retry3;
            }   
            
        printf("Connection Failed in clock client \n");
        exit(EXIT_FAILURE);
    }
    //printf("clock_client connected\n");

    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = (sizeof(*clock));

    //sending metadata for data type and control information
    int nsent=0;
    while (nsent < sizeof(meta_data))
        nsent += send(client_fd, (void*)((char*)&meta_data)+nsent, sizeof(meta_data)-nsent,0);
    if (nsent < (sizeof(meta_data))){
        perror("Meta_data not correct send in clock_client\n");
        exit(EXIT_FAILURE);
    }


    nsent=0;
    while(nsent<sizeof(*clock)){
        nsent += send(client_fd, (void*)((char*)clock)+nsent, sizeof(*clock)-nsent, 0);
        //printf("In Clock client nsent %d sizeof(*clock)\n", nsent, sizeof(*clock));
    }
    if (nsent < (sizeof(*clock))){
            perror("Clock not correct send \n");
            exit(EXIT_FAILURE);
        } 
    
    printf("Clock for migration send\n");
    close(client_fd);
    return 0;
}


//@brief: recieves memory_chunks until all parts have been recieved and are aligned back together to a bigger memory image
int comm_chunk_server(uint8_t* mem)
{
    int server_fd, new_conn_fd; //data_size;
    struct sockaddr_in address;
    int opt = 1, filesrecv = 0, addrlen = sizeof(address);
    char buffer[1024]= {0};
    size_t location;
    //size_t memorypart=0;
    //unsigned long masksize;
    comm_socket_header_t meta_data = {0};


    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 10) < 0){
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen in mem chunk server \n");

    while(1){
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
            perror("accept failed\n");
            exit(EXIT_FAILURE);
        }
        
        //recieving metadata for data type and control information
        int nrecv = 0;
        while (nrecv < sizeof(meta_data))
            nrecv += recv(new_conn_fd, (void*)((char*)&meta_data)+nrecv, sizeof(meta_data)-nrecv, 0);            
        if (nrecv < (sizeof(meta_data))){
            perror("Meta_data not correct received in chunk_server \n");
            exit(EXIT_FAILURE);
        }
        //printf("In chunk_server metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

        //printf("Sending \n");
        if ((strcmp(meta_data.data_name,"mem")==0) && (strcmp(meta_data.data_position,"finished")!=0)){
            size_t chunk_size = 0;         
            int nrecv=0;
            int total=0;
            while (nrecv < sizeof(size_t))
                total += nrecv += recv(new_conn_fd, (void*)((char*)&location)+nrecv, sizeof(size_t)-nrecv, 0);
            //printf("location nrecv %d sizeof(size_t) %d\n", nrecv, sizeof(size_t));
            nrecv=0;
            //printf("Data_position: %s",meta_data.data_position);
            if (strcmp(meta_data.data_position,"PAGE_BITS")==0){
                while(nrecv<(1UL << PAGE_BITS))
                    total += nrecv += recv(new_conn_fd, (void*)(size_t*)(mem + (location & PAGE_MASK))+nrecv, (1UL << PAGE_BITS)-nrecv, 0);
                //printf("nrecv %d Page_Bits %d\n", nrecv, (1UL << PAGE_BITS));
                chunk_size=(1UL << PAGE_BITS);
            }
            else if (strcmp(meta_data.data_position,"PAGE_2M_BITS")==0){
                while(nrecv<(1UL << PAGE_2M_BITS))
                    total += nrecv += recv(new_conn_fd, (void*)(size_t*)(mem + (location & PAGE_2M_MASK))+nrecv, (1UL << PAGE_2M_BITS)-nrecv, 0);
                //printf("nrecv %d Page_2M %d\n", nrecv, (1UL << PAGE_2M_BITS));
                chunk_size=(1UL << PAGE_2M_BITS);
            }
            else if (strcmp(meta_data.data_position,"PAGE_SIZE")==0){
                while(nrecv<PAGE_SIZE)
                    total += nrecv += recv(new_conn_fd, (void*)(size_t*)(mem + location)+nrecv, PAGE_SIZE-nrecv, 0);
                //printf("nrecv %d Page_Size %d\n", nrecv, (PAGE_SIZE));
                chunk_size=PAGE_SIZE;
            }
            /*
            if (nrecv==PAGE_SIZE)
                printf("Page_Size recv\n");
            else if (nrecv==(1UL << PAGE_2M_BITS))
                printf("(1UL << PAGE_2M_BITS) recv\n");
            else if (nrecv==(1UL << PAGE_BITS))
                printf("(1UL << PAGE_BITS) recv\n");
            */

            if ((total) < (sizeof(size_t)+chunk_size)){
                perror("Memory chunk not correct recieved \n");
                exit(EXIT_FAILURE);
            }
        }else if((strcmp(meta_data.data_name,"mem")==0) && (strcmp(meta_data.data_position,"finished")==0)){
            close(new_conn_fd); 
            break;
        } else {
            perror("data_name in chunk_server wrong %s\n");
            close(new_conn_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        //memorypart++;
        //printf("memorypart %d recieved. \n ", memorypart);
        close(new_conn_fd);
    }
          

    printf("Memory for migration recieved returning to loading process\n");
    close(server_fd);
    return 0;
}

//@brief:   Client side C function for TCP Socket Connections sending Memory Chunk
//          atm sends pgd or pgt with corresponding memory to server (e.g. checkpoint)
int comm_chunk_client(size_t *pgdpgt, size_t *mem_chunck, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0; //data_size;
    char buffer[1024] = {0};

    comm_socket_header_t meta_data;

      
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
    

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    //needed to be add as otherwise server hasn't socket open in time
    usleep(2);
    int try=0;
    retry4:
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        if (try<maxtry){
                usleep(retytime);
                try++;
                goto retry4;
            }   
        printf("Connection Failed in memory chunk client\n");
        exit(EXIT_FAILURE);
    }
    
    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    if (meta_data.data_position=="PAGE_BITS")
        meta_data.data_size = (sizeof(size_t)+sizeof(unsigned long)+(1UL << PAGE_BITS));
    else if (meta_data.data_position=="PAGE_2M_BITS")
        meta_data.data_size = (sizeof(size_t)+sizeof(unsigned long)+(1UL << PAGE_2M_BITS));
    else if (meta_data.data_position=="PAGE_SIZE")
        meta_data.data_size = (sizeof(size_t)+sizeof(unsigned long)+PAGE_SIZE);
    
    //sending metadata for data type and control information
    int nsent=0;
    while (nsent < sizeof(meta_data))
        nsent += send(client_fd, (void*)((char*)&meta_data)+nsent, sizeof(meta_data)-nsent,0);
    if (nsent < (sizeof(meta_data))){
        perror("Meta_data not correct send in chunk_client\n");
        exit(EXIT_FAILURE);
    }

    if((strcmp(meta_data.data_name,"mem")==0) && (strcmp(meta_data.data_position,"finished")!=0)){
        unsigned long chunk_size=0;  
        int nsent=0;
        int total=0;
        while (nsent < sizeof(size_t))
            total+= nsent += send(client_fd, (void*)((char*)pgdpgt)+nsent, sizeof(size_t)-nsent, 0);
        //printf("chunk_client nsent %d sizeof(size_t) %d\n", nsent, sizeof(size_t));
        nsent=0;
        if (strcmp(meta_data.data_position,"PAGE_BITS")==0){
            while(nsent<(1UL << PAGE_BITS))
                total += nsent += send(client_fd, (void*)((size_t)mem_chunck)+nsent, (1UL << PAGE_BITS)-nsent, 0);
            //printf("nsent %d Page_Bits %d\n", nsent, (1UL << PAGE_BITS));
            chunk_size=(1UL << PAGE_BITS);
            } else if (strcmp(meta_data.data_position,"PAGE_2M_BITS")==0){
                while(nsent<(1UL << PAGE_2M_BITS))
                    total += nsent += send(client_fd, (void*)((size_t)mem_chunck)+nsent, (1UL << PAGE_2M_BITS)-nsent, 0);
                //printf("nsent %d Page_2M %d\n", nsent, (1UL << PAGE_2M_BITS));
                chunk_size=(1UL << PAGE_2M_BITS);
            } else if (strcmp(meta_data.data_position,"PAGE_SIZE")==0) {
                while(nsent<PAGE_SIZE)
                    total += nsent += send(client_fd, (void*)((size_t)mem_chunck)+nsent, PAGE_SIZE-nsent, 0);
                //printf("nsent %d Page_Size %d\n", nsent, PAGE_SIZE);
                chunk_size=PAGE_SIZE;
            } else {
                printf("ChunkSize in meta_data wrong in comm_chunk_client \n");
                close(client_fd);
                exit(EXIT_FAILURE);
            }
            if (total < (sizeof(size_t)+chunk_size)){
                perror("Memory not correct send \n");
                exit(EXIT_FAILURE);
            }
    } else if ((strcmp(meta_data.data_name,"mem")==0) && (strcmp(meta_data.data_position,"finished")==0)) {
        printf("All Memory chunks sent \n");
        close(client_fd);
        return 0;
    } else {
        perror("transfer type in Data_Name wrong in comm_chunk_client\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    close(client_fd);
    return 0;
}


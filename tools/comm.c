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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0)
    {
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
        while (nrecv<(sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
        if (nrecv < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }
        printf("metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

    
        //strcpy(meta_data.data_name, "checkpoint/chk_config_copy.txt"); //"checkpoint/testcopy_Abrechnung.xlsm"
        if (stat(meta_data.data_position, &st) == -1)
		mkdir(meta_data.data_position, 0700);

        // create file where data will be stored
        FILE *fp;
        fp = fopen(meta_data.data_name, "wb"); 
        if(NULL == fp)
        {
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
        sleep(1);
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
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);   
    } 
     
    fseek(fp, 0, SEEK_END); // seek to end of file and set current file to it
    meta_data.data_size = ftell(fp);   // assign current filepointer (end of file), which is size of the file, to data_size
    rewind(fp); // set file pointer back to start of file
   
    // sending file meta information to server before sending file
    printf("metafilesize: %d to filename: %s and position: %s \n " , meta_data.data_size, meta_data.data_name, meta_data.data_position);
    int nsent=0;
    while (nsent<sizeof(buffer))
        nsent += send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)))    
        nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
        nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }

        
    // read data from file and send it
    while(1)
    {
        int nread = 0;


        // reading file in chunks of buffer bytes
        nread = fread(buffer,sizeof(char),sizeof(buffer),fp);
        //printf("Bytes read %d \n", nread);        

        // succesive reading leads to sending the data. 
        if(nread > 0)
        {
            int nsend = 0;
            while (nsend < sizeof(buffer))
                nsend+=send(client_fd, buffer, nread, 0);
            //printf("Sending \n");
            
        }
      
        // checking if end of file was reached or error occured     
        if (nread < sizeof(buffer))
        {
            if (feof(fp))
                printf("End of file\n");
                
            if (ferror(fp))
            {
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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
     
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");


    while(1)
    {
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed\n");
            exit(EXIT_FAILURE);
        }
        
        // recieving file metadata for positioning, name and size from client 
        int nrecv = 0;
        while (nrecv<(sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
        if (nrecv < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }
        //printf("In comm_config_server recieved sizeof(checkpoint_config) %d, checkpoint_config.elf_entry %d, checkpoint_config.guest_size %d",sizeof(checkpoint_config), checkpoint_config.elf_entry , checkpoint_config.guest_size);

        if (strcmp(meta_data.data_name,"config")==0){
            int nrecv = 0;
            while (nrecv<sizeof(meta_data.data_size))
                nrecv += recv(new_conn_fd, (void*)((size_t)checkpoint_config+nrecv), sizeof(meta_data.data_size)-nrecv, 0);
            if (nrecv<(sizeof(meta_data.data_size)))
                perror("Register recieved incomplete\n");
            else if (nrecv=sizeof(meta_data.data_size))
                break;
        }else{
            printf("config_server wrong meta_data: Name %s, Position: %s in config recieved\n", meta_data.data_name, meta_data);
            exit(EXIT_FAILURE);
        }
        
    }

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
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    
    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = (sizeof(comm_config_t));
    //printf("In Comm_Config_client sent sizeof(checkpoint_config) %d, checkpoint_config.elf_entry %d, checkpoint_config.guest_size %d",sizeof(checkpoint_config), checkpoint_config.elf_entry , checkpoint_config.guest_size);
    int nsent=0;
    while (nsent<sizeof(buffer))
        nsent += send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)))    
        nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
        nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }

    //printf("Sending \n");
    nsent=0;
    while(nsent<sizeof(comm_config_t))
    {
        nsent += send(client_fd, (void*)((size_t)checkpoint_config+nsent), sizeof(comm_config_t)-nsent, 0);
    }
    if (nsent < (sizeof(comm_config_t)))
    {
        perror("Config for migration not correct send \n");
        exit(EXIT_FAILURE);
    } 
   
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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
     
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");


    while(1)
    {
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed\n");
            exit(EXIT_FAILURE);
        }
        
        // recieving file metadata for positioning, name and size from client
        int nrecv = 0;
        while (nrecv<(sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
        if (nrecv < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }

        printf("In register_server metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

        if(strcmp(meta_data.data_name,"register")==0)
        {
            int nrecv = 0;
            while (nrecv<sizeof(meta_data.data_size))
                nrecv += recv(new_conn_fd, (void*)((size_t)vcpu_register+nrecv), (sizeof(meta_data.data_size))-nrecv, 0);
            if (nrecv<(sizeof(meta_data.data_size)))
                perror("Register recieved incomplete\n");
            else if (nrecv=(sizeof(meta_data.data_size)))
                break;
        }else {
        printf("wrong meta_data %s in register recieved", meta_data.data_name);
        exit(EXIT_FAILURE);
        }
        
        
    }
    printf("In comm_register_server recieved size %d vcpu_register->regs %d cpu_register->lapic %d\n",sizeof(*vcpu_register),vcpu_register->regs, vcpu_register->lapic);
    printf("In comm_register_server recieved size %d vcpu_register[*cpuid].regs %d vcpu_register[*cpuid].lapic %d\n", vcpu_register[*cpuid].regs, vcpu_register[*cpuid].lapic);
    close(server_fd);
    return 0;
}

// struct kvm_sregs *sregs, struct kvm_regs *regs, struct kvm_fpu *fpu, struct msr_data *msr_data, struct kvm_lapic_state *lapic,
// struct kvm_xsave *xsave, struct kvm_xcrs *xcrs, struct kvm_vcpu_events *events, struct kvm_mp_state *mp_state

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
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    

    //path in this case codes the type of transfer comming
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size=(sizeof(*vcpu_register));
    //printf("((*ncores*sizeof(comm_register_t)) %d ,sizeof(*vcpu_register) %d\n", (*ncores*sizeof(comm_register_t)),sizeof(*vcpu_register) );
    int nsent=0;
    while (nsent<sizeof(buffer))
        nsent += send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)))    
        nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
        nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }

    
    //printf("Sending \n");
    nsent=0;
    while(nsent<(sizeof(*vcpu_register)))
    {
        nsent += send(client_fd, (void*)((size_t)vcpu_register+nsent), (sizeof(*vcpu_register))-nsent, 0);
    }
    if (nsent < (sizeof(*vcpu_register)))
        {
            perror("Register not correct send \n");
            exit(EXIT_FAILURE);
        } 
    
    printf("In comm_register_client send size %d vcpu_register->regs %d vcpu_register->lapic %d\n",sizeof(*vcpu_register),vcpu_register->regs, vcpu_register->lapic);
    printf("In comm_register_client send size %d vcpu_register[*cpuid].regs %d vcpu_register[*cpuid].lapic %d\n",vcpu_register[*cpuid].regs, vcpu_register[*cpuid].lapic);

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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");

    while(1)
    {
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed\n");
            exit(EXIT_FAILURE);
        }
        
        // recieving file metadata for positioning, name and size from client 
        int nrecv = 0;
        while (nrecv<(sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
        if (nrecv < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }
        printf("In clock_server metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

        if (strcmp(meta_data.data_name,"clock")==0)
        {
            int nrecv = 0;
            while (nrecv<sizeof(clock))
                nrecv += recv(new_conn_fd,(void*)((size_t)clock+nrecv), sizeof(clock)-nrecv, 0);
            if (nrecv<(sizeof(clock)))
                perror("Clock recieved incomplete\n");
            else if (nrecv=sizeof(clock))
                break;
        }
        else {
        printf("wrong meta_data %s in clock recieved", meta_data.data_name);
        exit(EXIT_FAILURE);
        }
        
    }

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
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    
    //data_name and data_position in this case codes the type of transfer comming for comm_server
    strcpy(meta_data.data_name, comm_type);
    strcpy(meta_data.data_position, comm_subtype);
    meta_data.data_size = (sizeof(clock));
    int nsent=0;
    while (nsent<sizeof(buffer))
        nsent += send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)))    
        nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
        nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }

    //printf("Sending \n");
    nsent=0;
    while(nsent<sizeof(clock))
    {
        nsent += send(client_fd, (void*)((size_t)clock+nsent), sizeof(clock)-nsent, 0);
    }
    if (nsent < (sizeof(clock)))
        {
            perror("Clock not correct send \n");
            exit(EXIT_FAILURE);
        } 
   
    close(client_fd);
    return 0;
}

//TODO: adding loop for all tables until all mem tables are recieved
//size_t *pgdpgt, size_t *mem_chunck, unsigned long *masksize

int comm_chunk_server(uint8_t* mem)
{
    int server_fd, new_conn_fd; //data_size;
    struct sockaddr_in address;
    int opt = 1, filesrecv = 0, addrlen = sizeof(address);
    char buffer[1024]= {0};
    size_t location;
    unsigned long masksize;

    comm_socket_header_t meta_data = {0};


    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    // enable reuse of port and address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }
    // start listening to incomming TCP connections on port
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
    //printf("waiting on connection by listen \n");

    while(1)
    {
        if ((new_conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed\n");
            exit(EXIT_FAILURE);
        }
        
        // recieving file metadata for positioning, name and size from client 
        int nrecv = 0;
        while (nrecv<(sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
        while (nrecv<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
            nrecv+=recv(new_conn_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
        if (nrecv < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }
        printf("In chunk_server metafilesize: %d to filename: %s and position: %s \n" , meta_data.data_size, meta_data.data_name, meta_data.data_position);

        //printf("Sending \n");
        if (strcmp(meta_data.data_name,"mem")==0)
        {         
            int nrecv1=0;
            while(nrecv1<sizeof(size_t))
                nrecv1 += recv(new_conn_fd, (void*)((size_t)location+nrecv1), sizeof(size_t)-nrecv1, 0);
            int nrecv2=0;
            while(nrecv2<sizeof(unsigned long))
                nrecv2 += recv(new_conn_fd, (void*)((size_t)masksize+nrecv2), sizeof(unsigned long)-nrecv2, 0);
            int nrecv3=0;
            if (meta_data.data_position=="PAGE_BITS")
            {
                while(nrecv3<(1UL << PAGE_BITS))
                    nrecv3 += recv(new_conn_fd, (void*)(size_t*)(mem + (location & PAGE_MASK))+nrecv3, (1UL << PAGE_BITS)-nrecv3, 0);
            }
            else if (meta_data.data_position=="PAGE_2M_BITS")
            {
                while(nrecv3<(1UL << PAGE_2M_BITS))
                    nrecv3 += recv(new_conn_fd, (void*)(size_t*)(mem + (location & PAGE_2M_MASK))+nrecv3, (1UL << PAGE_2M_BITS)-nrecv3, 0);
            }
            else if (meta_data.data_position=="PAGE_SIZE")
            {
                while(nrecv3<PAGE_SIZE)
                    nrecv3 += recv(new_conn_fd, (void*)(size_t*)(mem + location)+nrecv3, PAGE_SIZE-nrecv3, 0);
            }
            if (nrecv3==PAGE_SIZE)
                printf("Page_Size recv");
            else if (nrecv3==(1UL << PAGE_2M_BITS))
                printf("(1UL << PAGE_2M_BITS) recv");
            else if (nrecv3==(1UL << PAGE_BITS))
                printf("(1UL << PAGE_BITS) recv");

            if ((nrecv1+nrecv2+nrecv3) < (meta_data.data_size))
            {
                perror("Memory chunk not correct recieved \n");
                //exit(EXIT_FAILURE);
            }
        }else if(strcmp(meta_data.data_name,"mem")==0 && strcmp(meta_data.data_name,"finished")==0)
            break;
        else{
            perror("data_name in chunk_server wrong \n");
            exit(EXIT_FAILURE);
        }
    }
          

    printf("Memory for migration recieved\n");
    close(server_fd);
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

*/

//@brief:   Client side C function for TCP Socket Connections sending Memory Chunk
//          atm sends pgd or pgt with corresponding memory to server (e.g. checkpoint)
int comm_chunk_client(size_t *pgdpgt, size_t *mem_chunck, unsigned long *masksize, char *server_ip, char *comm_type, char *comm_subtype)
{
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    int client_fd = 0; //data_size;
    char buffer[1024] = {0};

    comm_socket_header_t meta_data;


    //printf("start comm client \n");
    
    
    // starting socket in IPv4 mode as AF_INET indicates
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error \n");
        exit(EXIT_FAILURE);
    }
    

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 (and IPv6) addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) 
    {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }
    // Connect to Server with assambeled information in struct serv_addr
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed \n");
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
    int nsent=0;
    while (nsent<sizeof(buffer))
        nsent += send(client_fd, (void*)&meta_data.data_name, sizeof(buffer), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)))    
        nsent += send(client_fd, (void*)&meta_data.data_size, sizeof(uint), 0);
    while (nsent<(sizeof(buffer)+sizeof(uint)+sizeof(buffer)))
        nsent += send(client_fd, (void*)&meta_data.data_position, sizeof(buffer), 0);
    if (nsent < (sizeof(meta_data.data_name)+sizeof(meta_data.data_size)+sizeof(meta_data.data_position)))
        {
            perror("Meta_data not correct send \n");
            exit(EXIT_FAILURE);
        }

    if(strcmp(meta_data.data_name,"mem")==0)
    {
    //printf("Sending \n");
    int nsent1=0;
        while(nsent1<sizeof(size_t))
            nsent1 += send(client_fd, (void*)((size_t)pgdpgt+nsent1), sizeof(size_t)-nsent1, 0);
        int nsent2=0;
        while(nsent2<(sizeof(unsigned long)))    
            nsent2 += send(client_fd, (void*)((size_t)masksize+nsent2), sizeof(unsigned long)-nsent2, 0);
        int nsent3=0;
        if (meta_data.data_position=="PAGE_BITS")
        {
        while(nsent3<(1UL << PAGE_BITS))
            nsent3 += send(client_fd, (void*)((size_t)mem_chunck+nsent3), (1UL << PAGE_BITS)-nsent3, 0);
        }
        else if (meta_data.data_position=="PAGE_2M_BITS")
        {
        while(nsent3<(1UL << PAGE_2M_BITS))
            nsent3 += send(client_fd, (void*)((size_t)mem_chunck+nsent3), (1UL << PAGE_2M_BITS)-nsent3, 0);
        }
        else if (meta_data.data_position=="PAGE_SIZE")
        {
        while(nsent3<PAGE_SIZE)
            nsent3 += send(client_fd, (void*)((size_t)mem_chunck+nsent3), PAGE_SIZE-nsent3, 0);
        }
        

        if ((nsent1+nsent2+nsent3) < meta_data.data_size)
            {
                perror("Memory not correct send not correct send \n");
                exit(EXIT_FAILURE);
            }
    }
    else {
        perror("Data_Name wrong in comm_chunk_client\n");
        exit(EXIT_FAILURE);
    }
     
    close(client_fd);
    return 0;
}


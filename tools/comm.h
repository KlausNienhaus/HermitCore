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

#ifndef __COMM_H__
#define __COMM_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdbool.h>
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

#define MAX_MSR_ENTRIES	25

// struct for information size and data name
typedef struct{
	uint	data_size;	/* Data Size  */
	char	data_name[1024];	/* File Name   */
    char    data_position[1024]; /* Data/File Path */
} comm_socket_header_t;

typedef struct{
	struct msr_data{
		struct kvm_msrs info;
		struct kvm_msr_entry entries[MAX_MSR_ENTRIES];
	} msr_data;

	struct kvm_regs regs;
	struct kvm_sregs sregs;
	struct kvm_fpu fpu;
	struct kvm_lapic_state lapic;
	struct kvm_xsave xsave;
	struct kvm_xcrs xcrs;
	struct kvm_vcpu_events events;
	struct kvm_mp_state mp_state;
}comm_register_t;

typedef struct{
	uint32_t 	*ncores; 
	size_t		*guest_size; 
	uint32_t 	*no_checkpoint; 
	uint64_t 	*elf_entry; 
	bool 		*full_checkpoint;
}comm_config_t;

int commserver(void);
int commclient(char *path, char *position, char *server_ip);

int comm_clock_client(struct kvm_clock_data *clock, char *server_ip, char *comm_type, char *comm_subtype);
int comm_chunk_client(size_t *pgdpgt, size_t *mem_chuck, unsigned long masksize, char *server_ip, char *comm_type, char *comm_subtype);
int comm_register_client(struct kvm_sregs *sregs, struct kvm_regs *regs, struct kvm_fpu *fpu, struct msr_data *msr_data, struct kvm_lapic_state *lapic,
struct kvm_xsave *xsave, struct kvm_xcrs *xcrs, struct kvm_vcpu_events *events,struct kvm_mp_state *mp_state, char server_ip, char *comm_type, char *comm_subtype);
int comm_config_client(comm_config_t *config_struct, char *server_ip, char *comm_type, char *comm_subtype);

#endif

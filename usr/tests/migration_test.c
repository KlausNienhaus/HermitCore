/*
 * Copyright (c) 2017, Klaus Nienhaus, RWTH Aachen University
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <hermit/syscall.h>
#include <assert.h>
#include <malloc.h>

#ifndef SIZE
#define SIZE    1*1024*1024
#endif 

int main(int argc, char** argv)
{
    //uint64_t memory_size = *argv[argc-1];
    int zaehler_sleeps = 0;
    uint16_t no_mb = 250;
    /*if(argc < 2) {
		no_mb = *argv[1];
        printf("argv[1] %d, *argv[1] %d", argv[1], *argv[1]);
	}else{
        printf("no arguments passed");
    }*/

    char (*memory) [no_mb*SIZE] = malloc(sizeof(*memory));
    printf("charsize %d, int8_t size %d", sizeof(char), sizeof(int8_t));
    
    if (memory) 
        memset(memory, 1, sizeof(*memory));
    else
        perror("malloc() failed");

    char* comm = getenv("PROXY_COMM");
    printf("proxy comm %d \n", comm);

    malloc_stats();

/*   @brief Run Infintie Loop to test Checkpointing by looking at:
 *  local time differences, process time difference and loop iteration count */
    while (zaehler_sleeps<100)
    {
        //printf("argv[1] %d, *argv[1] %d, sizeof(memory) %d", argv[1], *argv[1], sizeof(memory));
        
        sys_msleep(1000);
        
        zaehler_sleeps++;

        printf("Loop Number %d sizeof(*memory) %d\n", zaehler_sleeps, sizeof(*memory));
    }
    
    free(memory);
    //*memory = NULL;

	return 0;
}

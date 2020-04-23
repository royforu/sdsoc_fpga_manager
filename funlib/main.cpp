/*
(c) Copyright 2013 - 2016 Xilinx, Inc. All rights reserved. 

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER 
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law: (1) THESE
MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX HEREBY
DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising
under or in connection with these materials, including for any direct, or any
indirect, special, incidental, or consequential loss or damage (including loss
of data, profits, goodwill, or any type of loss or damage suffered as a result
of any action brought by a third party) even if such damage or loss was
reasonably foreseeable or Xilinx had been advised of the possibility of the
same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer assumes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES. 
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "matrix.h"

#define numClasses 10
#define numFeatures  784
#define chunkSize  4000

#include "sds_lib.h"
unsigned long long sw_sds_counter_total = 0;
unsigned long long hw_sds_counter_total = 0;
unsigned int sw_sds_counter_num_calls = 0;
unsigned int hw_sds_counter_num_calls = 0;
unsigned long long sw_sds_counter = 0;
unsigned long long hw_sds_counter = 0;

#define sw_sds_clk_start() { sw_sds_counter = sds_clock_counter(); sw_sds_counter_num_calls++; }
#define hw_sds_clk_start() { hw_sds_counter = sds_clock_counter(); hw_sds_counter_num_calls++; }
#define sw_sds_clk_stop() { unsigned long long tmp = sds_clock_counter(); \
    sw_sds_counter_total += ((tmp < sw_sds_counter) ? (sw_sds_counter - tmp): (tmp - sw_sds_counter)); }
#define hw_sds_clk_stop() { unsigned long long tmp = sds_clock_counter(); \
    hw_sds_counter_total += ((tmp < hw_sds_counter) ? (hw_sds_counter - tmp): (tmp - hw_sds_counter)); }
#define sw_avg_cpu_cycles() (sw_sds_counter_total / sw_sds_counter_num_calls)
#define hw_avg_cpu_cycles() (hw_sds_counter_total / hw_sds_counter_num_calls)


static void init_arrays_in(float *in0,  float *in1, int in0len, int in1len)
{

    for (int i = 0; i < in0len; i++) {
             in0[i] = (float(i) + 1.0) / float(int(chunkSize*(numClasses+numFeatures+1)));
     }

     for (int i = 0; i < in1len; i++) {
             in1[i] = (float(i) + 1.0) / float(int(numClasses*(numFeatures+1)));
     }
     

     
}


int gettime(struct timeval  t0, struct timeval t1)
{
        return ((t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec -t0.tv_usec) / 1000.0f);
}

int fpga_state()
{
	FILE *fptr;
	char buf[10], *state;

	system("cat /sys/class/fpga_manager/fpga0/state >> state.txt");
	state = "operating";
	fptr = fopen("state.txt", "r");
	if (fptr) {
		fgets(buf, 10, fptr);
		fclose(fptr);
		system("rm state.txt");
		if (strcmp(buf, state) == 0)
			return 0;
		else
			return 1;
	}

	return 1;
}

void download_bitstream()
{
	int ret;

	//char *binfile = NULL;
	char *Module[100] = {0};
	int flags = 0;
	char command[2048];
	double time;
	struct timeval t1, t0;


	system("mkdir -p /lib/firmware");
	snprintf(command, sizeof(command), "cp zc706_wrapper.bit.bin /lib/firmware");
	system(command);
	snprintf(command, sizeof(command), "echo %x > /sys/class/fpga_manager/fpga0/flags", flags);
	system(command);

	//tmp = strdup(binfile);
	//while((token = strsep(&tmp, "/"))) {
		//tmp1 = token;
//	}
	snprintf(command, sizeof(command), "echo zc706_wrapper.bit.bin > /sys/class/fpga_manager/fpga0/firmware");
	gettimeofday(&t0, NULL);
	system(command);
	gettimeofday(&t1, NULL);
	time = gettime(t0, t1);
	if (!fpga_state()) {
				printf("Time taken to load BIN is %f Milli Seconds\n\r", time);
				printf("BIN FILE loaded through FPGA manager successfully\n\r");
			} else {
				printf("BIN FILE loading through FPGA manager failed\n\r");
			}
	snprintf(command, sizeof(command), "rm /lib/firmware/zc706_wrapper.bit.bin");
	system(command);
}



int main(int argc, char* argv[]){
  float *inbuf0, *inbuf1, *hwoutbuf;

  
  download_bitstream();

    int inbuffer0len = int(chunkSize*(numClasses+numFeatures+1));
    int inbuffer1len = int(numClasses*(numFeatures+1));
    int outbufferlen =  int(numClasses*(numFeatures+1));

    std::cout << "inbuffer0len: "
               << inbuffer0len << std::endl;

    
    std::cout << "inbuffer1len1: "
               << inbuffer1len << std::endl;


     std::cout << "outbufferlen: "
               << outbufferlen << std::endl;

     inbuf0 = (float *)sds_alloc(inbuffer0len* sizeof(float));
     inbuf1 = (float *)sds_alloc(inbuffer1len * sizeof(float));
     hwoutbuf = (float *)sds_alloc(outbufferlen * sizeof(float));
     init_arrays_in(inbuf0, inbuf1, inbuffer0len, inbuffer1len);

  std::cout << "data: " << inbuf0[0] << std::endl;
  std::cout << "weight: "  << inbuf1[0] << std::endl;


     bgd(inbuf0,inbuffer0len,inbuf1,inbuffer1len,hwoutbuf);


std::cout << "hwout: " << std::endl;
     for(int i=0; i<4 ;i++)
	 {
		if(i%10==0)
			std::cout << std::endl;
		std::cout << hwoutbuf[i] << " ";
	 }
	 for(int i=outbufferlen-4; i<outbufferlen ;i++)
	 {

	   std::cout << hwoutbuf[i] << " ";
	 }




       
     
}

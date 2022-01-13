/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>

Core blocked_core;
Core fine_grained_core;

Core CreateCore(int thread_count){
	Core* core = (Core*)malloc(sizeof(Core));
	if(core == NULL){
		perror("malloc");
		exit(-1);
	}
	core->thread_pool = (thread_args*)malloc(sizeof(thread_args) * thread_count);
	core->working_threads = (bool*)malloc(sizeof(bool) * thread_count);
	for(int i = 0 ; i < thread_count ; i++){	
		core->thread_pool[i].thread_id = i;
		core->thread_pool[i].is_availble = 0;
		for(int j = 0 ; j < REGS_COUNT ; j++){
			core->thread_pool[i].context.reg[j] = 0;
		}
		core->thread_pool[i].rip = 0;
		core->thread_pool[i].cycles_count = 0;
		core->working_threads[i] = TRUE;
	}
	core->thread_count = thread_count;
	return *core;
}

void coreDestroy(Core* core){
	free(core->thread_pool);
	free(core->working_threads);
	free(core);
}

int nextThread(Core* core, int curr_thread, int _switch){
	int iter = (curr_thread + 1) % core->thread_count;
	int index;
	for(int i = 0 ; i < core->thread_count ; i++){
		index = (iter + i) % core->thread_count;
		if(core->working_threads[index] && core->thread_pool[index].is_availble == 0){
			core->thread_pool[curr_thread].cycles_count += _switch;
			return index;
		}
	}
	for(int i = 0 ; i < core->thread_count ; i++){
		index = (curr_thread + i) % core->thread_count;
		if(core->working_threads[index]){
			if(i == 0) _switch = 0;
			core->thread_pool[curr_thread].cycles_count += _switch;
			return index;
		}
	}
	return -1;
}



bool workingThreadsLeft(Core* core){
	for(int i = 0 ; i < core->thread_count ; i++){
		if(core->working_threads[i] == TRUE){
			return TRUE;
		}
	}
	return FALSE;
}

void tickAllCmd(Core* core){
	for(int i = 0 ; i < core->thread_count ; i++){
		if(core->thread_pool[i].is_availble > 0){
			core->thread_pool[i].is_availble--;
		}
	}
}

int countCycles(Core* core){
	int cycles = 0;
	for(int i = 0 ; i < core->thread_count ; i++){
		cycles += core->thread_pool[i].cycles_count;
	}
	return cycles;
}


void CORE_BlockedMT() {
	int load_latency = SIM_GetLoadLat();
	int store_latency = SIM_GetStoreLat();
	int _switch = SIM_GetSwitchCycles(); //the cycles that switch between cycles takes
	int thread_number = SIM_GetThreadsNum();
	blocked_core = CreateCore(thread_number);

	int working_thread = 0;
	int inst_count = 0;
	int src_addr;
	int dst_addr;
	int tmp;
	Instruction inst;

	while(workingThreadsLeft(&blocked_core)){
		thread_args& thread = blocked_core.thread_pool[working_thread];
		SIM_MemInstRead(thread.rip, &inst, working_thread);
		int* dst_reg = &thread.context.reg[inst.dst_index];
		int src1_reg = thread.context.reg[inst.src1_index];
		int src2_reg_or_imm = inst.src2_index_imm;
		if(inst.isSrc2Imm == FALSE){
			src2_reg_or_imm = thread.context.reg[src2_reg_or_imm];
		}

		if(thread.is_availble > 0) inst.opcode = CMD_IDLE;
		switch(inst.opcode){
			case CMD_IDLE:
				thread.cycles_count++;
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(working_thread == -1){
					working_thread = tmp;
				}
				break;
			case CMD_NOP:
				thread.cycles_count++;
				blocked_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_ADD:     // dst <- src1 + src2
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg + src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_SUB:     // dst <- src1 - src2
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg - src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_ADDI:    // dst <- src1 + imm
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg + src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_SUBI:    // dst <- src1 - imm
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg - src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_LOAD:    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
				inst_count++;
				thread.cycles_count += 1;
				thread.is_availble = load_latency + 1;
				src_addr = src1_reg + src2_reg_or_imm;
				SIM_MemDataRead(src_addr, dst_reg);
				blocked_core.thread_pool[working_thread].rip += 1;
				tmp = working_thread;
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(working_thread == -1){
					working_thread = tmp;
				}
				break;
			case CMD_STORE:   // Mem[dst + src2] <- src1  (src2 may be an immediate)
				inst_count++;
				thread.cycles_count += 1;
				thread.is_availble = store_latency + 1;
				dst_addr = *dst_reg + src2_reg_or_imm;
				SIM_MemDataWrite(dst_addr, src1_reg);
				blocked_core.thread_pool[working_thread].rip += 1;
				tmp = working_thread;
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(working_thread == -1){
					working_thread = tmp;
				}
				break;
			case CMD_HALT:
				inst_count++;
				thread.cycles_count++;
				blocked_core.working_threads[working_thread] = FALSE;
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(working_thread == -1){
					blocked_core.inst_count = inst_count;
					return;
				}
				break;
		}
		
		tickAllCmd(&blocked_core);
	}
	blocked_core.inst_count = inst_count;
}

void CORE_FinegrainedMT(){
	int load_latency = SIM_GetLoadLat();
	int store_latency = SIM_GetStoreLat();
	// int _switch = SIM_GetSwitchCycles(); //the cycles that switch between cycles takes
	int thread_number = SIM_GetThreadsNum();

	fine_grained_core = CreateCore(thread_number);

	int working_thread = 0;
	int inst_count = 0;
	int src_addr;
	int dst_addr;
	Instruction inst;

	while(workingThreadsLeft(&fine_grained_core)){
		thread_args thread = fine_grained_core.thread_pool[working_thread];
		SIM_MemInstRead(thread.rip, &inst, working_thread);

		int* dst_reg = &thread.context.reg[inst.dst_index];
		int src1_reg = thread.context.reg[inst.src1_index];
		int src2_reg_or_imm = inst.src2_index_imm;
		if(inst.isSrc2Imm == FALSE){
			src2_reg_or_imm = thread.context.reg[src2_reg_or_imm];
		}

		if(thread.is_availble > 0) inst.opcode = CMD_IDLE;

		switch(inst.opcode){
			case CMD_IDLE:
				thread.cycles_count++;
				break;
			case CMD_NOP:
				thread.cycles_count++;
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_ADD:     // dst <- src1 + src2
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg + src2_reg_or_imm;
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_SUB:     // dst <- src1 - src2
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg - src2_reg_or_imm;
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_ADDI:    // dst <- src1 + imm
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg + src2_reg_or_imm;
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_SUBI:    // dst <- src1 - imm
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg - src2_reg_or_imm;
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_LOAD:    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
				inst_count++;
				thread.cycles_count += load_latency + 1;
				thread.is_availble = load_latency + 1;
				src_addr = src1_reg + src2_reg_or_imm;
				SIM_MemDataRead(src_addr, dst_reg);
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_STORE:   // Mem[dst + src2] <- src1  (src2 may be an immediate)
				inst_count++;
				thread.cycles_count += store_latency + 1;
				thread.is_availble = store_latency + 1;
				dst_addr = *dst_reg + src2_reg_or_imm;
				SIM_MemDataWrite(dst_addr, src1_reg);
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_HALT:
				inst_count++;
				thread.cycles_count++;
				fine_grained_core.working_threads[working_thread] = FALSE;
				break;
		}
		int tmp = working_thread;
		working_thread = nextThread(&fine_grained_core, working_thread, 0);
		if(working_thread == -1){
			working_thread = tmp;
		}
		tickAllCmd(&fine_grained_core);
	}
	fine_grained_core.inst_count = inst_count;
}

double CORE_BlockedMT_CPI(){
	double inst = blocked_core.inst_count;
	double cycle = countCycles(&blocked_core);
	coreDestroy(&blocked_core);
	return cycle / inst;
}

double CORE_FinegrainedMT_CPI(){
	double inst = fine_grained_core.inst_count;
	double cycle = countCycles(&fine_grained_core);	
	coreDestroy(&fine_grained_core);
	return cycle / inst;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid){
	for(int i = 0 ; i < REGS_COUNT ; i++){
		context->reg[i] = blocked_core.thread_pool[threadid].context.reg[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid){
	for(int i = 0 ; i < REGS_COUNT ; i++){
		context->reg[i] = fine_grained_core.thread_pool[threadid].context.reg[i];
	}
}

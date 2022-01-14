/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>

class thread_args{
public:
	int thread_id;
	int is_availble;
	tcontext context;
	uint32_t rip;
	uint32_t cycles_count;
	thread_args();
	~thread_args() = default;
};

thread_args::thread_args():thread_id(0), is_availble(0), rip(0), cycles_count(0){
	for (int i=0; i<REGS_COUNT; i++) {
		context.reg[i] = 0;
	}
}

class Core{
public:
	thread_args* thread_pool;
	bool* working_threads;
	int thread_count;
	int inst_count;
	Core(int thread_count);
	~Core() = default;
};

Core::Core(int thread_count = 0): thread_count(thread_count), inst_count(0) {
	working_threads = new bool[thread_count];
	thread_pool = new thread_args[thread_count];
	for(int i = 0 ; i < thread_count ; i++){	
		thread_pool[i].thread_id = i;
		thread_pool[i].is_availble = 0;
		for(int j = 0 ; j < REGS_COUNT ; j++){
			thread_pool[i].context.reg[j] = 0;
		}
		thread_pool[i].rip = 0;
		thread_pool[i].cycles_count = 0;
		working_threads[i] = TRUE;
	}
}

Core blocked_core;
Core fine_grained_core;


bool workingThreadsLeft(Core* core){
	for(int i = 0 ; i < core->thread_count ; i++){
		if(core->working_threads[i] == TRUE){
			return TRUE;
		}
	}
	return FALSE;
}

int nextThread(Core* core, int curr_thread, int _switch, bool Fine = FALSE){
	if(!Fine && core->working_threads[curr_thread] && core->thread_pool[curr_thread].is_availble <= 0) return curr_thread;
	int iter = (curr_thread + 1) % core->thread_count;
	int index;
	for(int i = 0 ; i < core->thread_count ; i++){
		index = (iter + i) % core->thread_count;
		if(core->working_threads[index] && core->thread_pool[index].is_availble <= 0){
			core->thread_pool[curr_thread].cycles_count += _switch;
			return index;
		}
	}
	if(Fine){
		for(int i = 0 ; i < core->thread_count ; i++){
			index = (curr_thread + i) % core->thread_count;
			if(core->working_threads[index]){
				return index;
			}
		}
	}
	if(workingThreadsLeft(core)) return curr_thread;
	else return -2;
}


void tickAllCmd(Core* core, int _switch){
	for(int i = 0 ; i < core->thread_count ; i++){
		if(core->thread_pool[i].is_availble > 0){
			core->thread_pool[i].is_availble -= _switch;
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
	blocked_core = Core(thread_number);

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

		if(thread.is_availble > 0 || blocked_core.working_threads[working_thread] == FALSE) inst.opcode = CMD_IDLE;
		switch(inst.opcode){
			case CMD_IDLE:
				thread.cycles_count++;
				tmp = working_thread;
				tickAllCmd(&blocked_core, 1);
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(tmp != working_thread) tickAllCmd(&blocked_core, _switch);
				if(working_thread == -1){
					working_thread = tmp;
				}
				break;
			case CMD_NOP:
				thread.cycles_count++;
				blocked_core.thread_pool[working_thread].rip += 1;
				tickAllCmd(&blocked_core, 1);
				break;
			case CMD_ADD:     // dst <- src1 + src2
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg + src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				tickAllCmd(&blocked_core, 1);
				break;
			case CMD_SUB:     // dst <- src1 - src2
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg - src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				tickAllCmd(&blocked_core, 1);
				break;
			case CMD_ADDI:    // dst <- src1 + imm
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg + src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				tickAllCmd(&blocked_core, 1);
				break;
			case CMD_SUBI:    // dst <- src1 - imm
				inst_count++;
				thread.cycles_count++;
				*dst_reg = src1_reg - src2_reg_or_imm;
				blocked_core.thread_pool[working_thread].rip += 1;
				tickAllCmd(&blocked_core, 1);
				break;
			case CMD_LOAD:    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
				inst_count++;
				thread.cycles_count += 1;
				thread.is_availble = load_latency + 1;
				src_addr = src1_reg + src2_reg_or_imm;
				SIM_MemDataRead(src_addr, dst_reg);
				blocked_core.thread_pool[working_thread].rip += 1;
				tmp = working_thread;
				tickAllCmd(&blocked_core, 1);
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(tmp != working_thread) tickAllCmd(&blocked_core, _switch);
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
				tickAllCmd(&blocked_core, 1);
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(tmp != working_thread) tickAllCmd(&blocked_core, _switch);
				if(working_thread == -1){
					working_thread = tmp;
				}
				break;
			case CMD_HALT:
				inst_count++;
				thread.cycles_count++;
				blocked_core.working_threads[working_thread] = FALSE;
				tickAllCmd(&blocked_core, 1);
				tmp = working_thread;
				working_thread = nextThread(&blocked_core, working_thread, _switch);
				if(tmp != working_thread) tickAllCmd(&blocked_core, _switch);
				if(working_thread == -1){
					working_thread = tmp;
				}
				if(working_thread == -2){
					blocked_core.inst_count = inst_count;
					return;
				}
				break;
		}
	}
	blocked_core.inst_count = inst_count;
}

void CORE_FinegrainedMT(){
	int load_latency = SIM_GetLoadLat();
	int store_latency = SIM_GetStoreLat();
	int thread_number = SIM_GetThreadsNum();
	fine_grained_core = Core(thread_number);

	int working_thread = 0;
	int inst_count = 0;
	int src_addr;
	int dst_addr;
	int tmp;
	Instruction inst;

	while(workingThreadsLeft(&fine_grained_core)){
		thread_args& thread = fine_grained_core.thread_pool[working_thread];
		SIM_MemInstRead(thread.rip, &inst, working_thread);
		int* dst_reg = &thread.context.reg[inst.dst_index];
		int src1_reg = thread.context.reg[inst.src1_index];
		int src2_reg_or_imm = inst.src2_index_imm;
		if(inst.isSrc2Imm == FALSE){
			src2_reg_or_imm = thread.context.reg[src2_reg_or_imm];
		}
		if(thread.is_availble > 0 || fine_grained_core.working_threads[working_thread] == FALSE) inst.opcode = CMD_IDLE;
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
				thread.cycles_count += 1;
				thread.is_availble = load_latency + 1;
				src_addr = src1_reg + src2_reg_or_imm;
				SIM_MemDataRead(src_addr, dst_reg);
				fine_grained_core.thread_pool[working_thread].rip += 1;
				break;
			case CMD_STORE:   // Mem[dst + src2] <- src1  (src2 may be an immediate)
				inst_count++;
				thread.cycles_count += 1;
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
		tickAllCmd(&fine_grained_core, 1);
		tmp = working_thread;
		working_thread = nextThread(&fine_grained_core, working_thread, 0, TRUE);
		if(working_thread == -1){
			working_thread = tmp;
		}
		if(working_thread == -2){
			fine_grained_core.inst_count = inst_count;
			return;
		}
	}
	fine_grained_core.inst_count = inst_count;
}

double CORE_BlockedMT_CPI(){
	double inst = blocked_core.inst_count;
	double cycle = countCycles(&blocked_core);
	return cycle / inst;
}

double CORE_FinegrainedMT_CPI(){
	double inst = fine_grained_core.inst_count;
	double cycle = countCycles(&fine_grained_core);	
	return cycle / inst;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid){
	for(int i = 0 ; i < REGS_COUNT ; i++){
		context[threadid].reg[i] = blocked_core.thread_pool[threadid].context.reg[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid){
	for(int i = 0 ; i < REGS_COUNT ; i++){
		context[threadid].reg[i] = fine_grained_core.thread_pool[threadid].context.reg[i];
	}
}

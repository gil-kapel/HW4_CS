/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#ifndef CORE_API_H_
#define CORE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0
#define REGS_COUNT 8

typedef enum {
	CMD_NOP = 0,
    CMD_ADD,     // dst <- src1 + src2
    CMD_SUB,     // dst <- src1 - src2
    CMD_ADDI,    // dst <- src1 + imm
    CMD_SUBI,    // dst <- src1 - imm
    CMD_LOAD,    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
    CMD_STORE,   // Mem[dst + src2] <- src1  (src2 may be an immediate)
	CMD_HALT,
	CMD_IDLE,
} cmd_opcode;

typedef struct _inst {
	cmd_opcode opcode;
	int dst_index;
	int src1_index;
	int src2_index_imm;
	bool isSrc2Imm; // if the second argument is immediate
} Instruction;

typedef struct _regs {
	int reg[REGS_COUNT];
} tcontext;

typedef struct{
	int thread_id;
	int is_availble;
	tcontext context;
	uint32_t rip;
	uint32_t cycles_count; 
}thread_args;

typedef struct{
	thread_args* thread_pool;
	bool* working_threads;
	int thread_count;
	int inst_count;
}Core;

Core CreateCore(int thread_count);

void coreDestroy(Core* core);

/* Simulates blocked MT and fine-grained MT behavior, respectively */
void CORE_BlockedMT();
void CORE_FinegrainedMT();

/* Get thread register file through the context pointer */
void CORE_BlockedMT_CTX(tcontext context[], int threadid);
void CORE_FinegrainedMT_CTX(tcontext context[], int threadid);

/* Return performance in CPI metric */
double CORE_BlockedMT_CPI();
double CORE_FinegrainedMT_CPI();

#ifdef __cplusplus
}
#endif

#endif /* CORE_API_H_ */
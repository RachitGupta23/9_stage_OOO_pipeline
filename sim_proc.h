#include <stdio.h>
#include <string.h>
#include <vector>
#include <array>
#include <iostream>
#include <stdlib.h>
#include <inttypes.h>
#include <cmath>
#include <cassert>
#include <iomanip>
#include <deque>
#include <memory>
#include <algorithm>
#ifndef SIM_PROC_H
#define SIM_PROC_H

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;
proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
// Put additional data structures here as per your requirement

struct stage_cnt{
	uint32_t entry = 0;
	uint32_t time = 0;
};

struct operation{
	uint64_t seq_no = 0;
	uint64_t pc = 0;
	int dst = -1;
	int dst_rename = -1;
	int rs1 = -1;
	int rs1_rename = -1;
	int rs2 = -1;
	int rs2_rename = -1;
	int optype = -1;
	int exe_latency = 0;
	int rob_index = 0;
	stage_cnt FE;
	stage_cnt DE;
	stage_cnt RN;
	stage_cnt RR;
	stage_cnt DI;
	stage_cnt IS;
	stage_cnt EX;
	stage_cnt WB;
	stage_cnt RT;
};
struct rmt_entry{
	bool vld = false;
	int tag = -1;
};

struct rob_entry{
	int dst = -1;
	int dst_rename = -1;
	bool ready = false;
	bool exe_bypass = false;
	uint64_t pc = 0;
	uint64_t seq_no = 0;
};

struct IS_entry{
	bool vld = false;
	int dst = -1;
	bool rs1_rdy = false;
	int rs1 = -1;
	bool rs2_rdy = false;
	int rs2 = -1;
};

std::deque<operation> FE_reg;
std::deque<operation> DE_reg;
std::deque<operation> RN_reg;
std::deque<operation> RR_reg;
std::deque<std::pair<operation, IS_entry>> DI_reg;
std::deque<std::pair<operation, IS_entry>> IQ_reg;
std::deque<operation> EX_reg;
std::deque<operation> WB_reg;
std::deque<operation> RT_reg;

class ROB_TABLE {
public:
	std::vector<rob_entry> entries;
	int head = 0;
	int tail = 0;
	uint64_t ROB_count = 0;

	ROB_TABLE(uint64_t ROB_size) {
		entries.resize(ROB_size);
		ROB_count = ROB_size;
	}	
};
std::unique_ptr<ROB_TABLE> ROB_table;
class Issue_Queue {
public:
	std::vector<std::pair<operation, IS_entry>> entries;
	std::deque<uint32_t> idx_queue;
	uint64_t IQ_count = 0;

	Issue_Queue(uint64_t IQ_size) {
		entries.resize(IQ_size);
		IQ_count = IQ_size;
		for(size_t i = 0; i < entries.size(); i++) {
			idx_queue.emplace_back(i);
		}
	}

	int find_oldest_ready() {
    	int best_index = -1;
    	for (int i = 0; i < (int)entries.size(); i++) {
    	    const auto& e = entries[i];
    	    if (!e.second.vld || !e.second.rs1_rdy || !e.second.rs2_rdy)
    	        continue;
    	    if (best_index == -1 || e.first.seq_no < entries[best_index].first.seq_no) {
    	        best_index = i;
    	    }
    	}
    	return best_index;
	}	
};
std::unique_ptr<Issue_Queue> Issue_Q;

std::vector<rmt_entry> RMT(67);
uint64_t seq_no = 0;
uint64_t cycle_count = 0;
bool eof_reached = false;
std::array<bool, 9> stage_idle = {true, true, true, true, true, true, true, true, true};
void print_instruction(operation& inst);
void Retire();
void Writeback();
void Execute();
void Issue();
void Dispatch();
void RegRead();
void Rename();
void Decode();
void Fetch(FILE* FP);
bool Advance_Cycle();

#endif

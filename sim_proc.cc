#include "sim_proc.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name; 
    //int op_type, dest, src1, src2;  // Variables are read from trace file
    //uint64_t pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    //printf("rob_size:%lu "
    //        "iq_size:%lu "
    //        "width:%lu "
    //        "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
	ROB_table = std::make_unique<ROB_TABLE>(params.rob_size);
	Issue_Q = std::make_unique<Issue_Queue>(params.iq_size);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
	
	do {
		Retire();
		Writeback();
		Execute();
		Issue();
		Dispatch();
		RegRead();
		Rename();
		Decode();
		Fetch(FP);
	} while(Advance_Cycle());
	std::cout << "# === Simulator Command =========\n";
	std::cout << "# ./sim " << params.rob_size << " " << params.iq_size << " " << params.width << " " << trace_file << "\n";
	std::cout << "# === Processor Configuration ===\n";
	std::cout << "# ROB_SIZE = " << params.rob_size << "\n";
	std::cout << "# IQ_SIZE  = " << params.iq_size << "\n";
	std::cout << "# WIDTH    = " << params.width << "\n";
	std::cout << "# === Simulation Results ========\n";
	std::cout << "# Dynamic Instruction Count    = " << seq_no << "\n";
	std::cout << "# Cycles                       = " << cycle_count-1 << "\n";
	std::cout << "# Instructions Per Cycle (IPC) = " << std::fixed << std::setprecision(2) << static_cast<double>(seq_no)/(cycle_count - 1) << "\n";
    //while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //    printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly

    return 0;
}

bool Advance_Cycle() {
	cycle_count++;
	bool alltrue = true;
	for (size_t i = 0; i < stage_idle.size(); i++) {
		if(!stage_idle[i]){
			alltrue = false;
			break;
		}
	}
	return (!(eof_reached && alltrue));
}

void Fetch(FILE* FP) {
	operation temp;
	if(eof_reached) {
		stage_idle[0] = true;
		return;
	}	
	//if(!FE_reg.empty()) {
	if(DE_reg.size() >= params.width) {
		stage_idle[0] = false;
		return;
	}
	while((DE_reg.size() < params.width) && !eof_reached) {
		if(fscanf(FP, "%lx %d %d %d %d", &temp.pc, &temp.optype, &temp.dst, &temp.rs1, &temp.rs2) != EOF) {
			temp.seq_no = seq_no;
			seq_no++;
			temp.FE.entry = cycle_count;
			temp.DE.entry = cycle_count + 1;
			temp.FE.time = 1;
			DE_reg.emplace_back(temp);
		} else {
			eof_reached = true;
			break;
		}
	}
	stage_idle[0] = false;
	//}
	//while((FE_reg.size() < params.width) && !eof_reached) {
	//	if(fscanf(FP, "%lx %d %d %d %d", &temp.pc, &temp.optype, &temp.dst, &temp.rs1, &temp.rs2) != EOF) {
	//		temp.seq_no = seq_no;
	//		seq_no++;
	//		temp.FE.entry = cycle_count;
	//		FE_reg.emplace_back(temp);
	//	} else {
	//		eof_reached = true;
	//		break;
	//	}	
	//}
	//stage_idle[0] = false;
}

void Decode() {
	if(RN_reg.size() >= params.width) {
		stage_idle[1] = false;
		return;
	}
	if((DE_reg.size() == 0)) {
		stage_idle[1] = true;
		return;
	}
	while(!DE_reg.empty()) { //((RN_reg.size() < params.width) && 
		DE_reg[0].RN.entry = cycle_count+1;
		DE_reg[0].DE.time = cycle_count + 1 - DE_reg[0].DE.entry;
		RN_reg.emplace_back(DE_reg.front());
		DE_reg.pop_front();	
	}
	stage_idle[1] = false;
}

void Rename() {
	if(RR_reg.size() >= params.width) {
		stage_idle[2] = false;
		return;
	}
	if((RN_reg.size() == 0)) {
		stage_idle[2] = true;
		return;
	}
	if(!RN_reg.empty()) {
		//std::cout << "ROB_count = " << ROB_table->ROB_count << " head = " << ROB_table->head << " tail = " << ROB_table->tail << "\n";
		if(ROB_table->ROB_count < RN_reg.size()) {
			stage_idle[2] = false;
			return;
		} else {
			while(!RN_reg.empty()) {//(RR_reg.size() < params.width) &&
				//std::cout << " ROB_count_entered = " << ROB_table->ROB_count << " seq_no = " << RN_reg[0].seq_no << "\n";
				if(RN_reg[0].rs1 >= 0) {
					RN_reg[0].rs1_rename = RMT[RN_reg[0].rs1].vld ? RMT[RN_reg[0].rs1].tag : -1;
				}
				if(RN_reg[0].rs2 >= 0) {
					RN_reg[0].rs2_rename = RMT[RN_reg[0].rs2].vld ? RMT[RN_reg[0].rs2].tag : -1;
				}
				RN_reg[0].dst_rename = -1;
				if(RN_reg[0].dst >= 0) {
					RMT[RN_reg[0].dst].vld = true;
					RMT[RN_reg[0].dst].tag = ROB_table->tail;
					RN_reg[0].dst_rename = ROB_table->tail;
				}
				ROB_table->entries[ROB_table->tail].dst = RN_reg[0].dst;
				ROB_table->entries[ROB_table->tail].pc = RN_reg[0].pc;
				ROB_table->entries[ROB_table->tail].seq_no = RN_reg[0].seq_no;
				ROB_table->entries[ROB_table->tail].dst_rename = RN_reg[0].dst_rename;
				ROB_table->entries[ROB_table->tail].ready = false;
				ROB_table->entries[ROB_table->tail].exe_bypass = false;
				RN_reg[0].rob_index = ROB_table->tail;
				RN_reg[0].RR.entry = cycle_count+1;
				RN_reg[0].RN.time = cycle_count + 1 - RN_reg[0].RN.entry;
				RR_reg.emplace_back(RN_reg.front());
				RN_reg.pop_front();
				stage_idle[2] = false;
				if(ROB_table->tail == static_cast<int>(params.rob_size - 1)) {
					ROB_table->tail = 0;
				} else {
					ROB_table->tail++;
				}
				ROB_table->ROB_count--;
			}
		}
	}
}

void RegRead() {
	if(DI_reg.size() >= params.width) {
		stage_idle[3] = false;
		return;
	}
	if(RR_reg.empty()) {
		stage_idle[3] = true;
		return;
	}
	while(!RR_reg.empty()) {//DI_reg.size() < params.width && 
		IS_entry temp;
		temp.vld = true;
		temp.dst = RR_reg[0].dst_rename;//(RR_reg[0].dst_rename == -1) ? RR_reg[0].dst : 
		temp.rs1 = (RR_reg[0].rs1_rename == -1) ? RR_reg[0].rs1 : RR_reg[0].rs1_rename;
		temp.rs2 = (RR_reg[0].rs2_rename == -1) ? RR_reg[0].rs2 : RR_reg[0].rs2_rename;
		temp.rs1_rdy = (RR_reg[0].rs1_rename == -1) ? true : ROB_table->entries[RR_reg[0].rs1_rename].ready || ROB_table->entries[RR_reg[0].rs1_rename].exe_bypass;
		temp.rs2_rdy = (RR_reg[0].rs2_rename == -1) ? true : ROB_table->entries[RR_reg[0].rs2_rename].ready || ROB_table->entries[RR_reg[0].rs2_rename].exe_bypass;
		RR_reg[0].DI.entry = cycle_count+1;
		RR_reg[0].RR.time = cycle_count+1 - RR_reg[0].RR.entry;
		DI_reg.emplace_back(std::make_pair(RR_reg.front(), temp));
		RR_reg.pop_front();	
		stage_idle[3] = false;
	}
}

void Dispatch() {
	if(DI_reg.empty()) {
		stage_idle[4] = true;
		return;
	}
	if(Issue_Q->idx_queue.size() < DI_reg.size()) {
		stage_idle[4] = false;
		return;
	}	
	while(!DI_reg.empty()) {
		uint32_t idx = Issue_Q->idx_queue.front();
		Issue_Q->idx_queue.pop_front();
		DI_reg[0].first.IS.entry = cycle_count+1;
		DI_reg[0].first.DI.time = cycle_count+1 - DI_reg[0].first.DI.entry;
		Issue_Q->entries[idx] = DI_reg[0];
		DI_reg.pop_front();
	}
	stage_idle[4] = false;
}

void Issue() {	
	if(Issue_Q->idx_queue.size() == params.iq_size) {
		stage_idle[5] = true;
		return;
	}
	int issue_count = 0;
	if(Issue_Q->idx_queue.size() < params.iq_size) {	
		while((issue_count < static_cast<int>(params.width)) && (Issue_Q->idx_queue.size() < params.iq_size)) {	
			int temp = Issue_Q->find_oldest_ready();
			if(temp == -1) {
				break;	
			}
			Issue_Q->entries[temp].first.IS.time = cycle_count + 1 - Issue_Q->entries[temp].first.IS.entry;
			Issue_Q->entries[temp].first.EX.entry = cycle_count + 1;
			EX_reg.emplace_back(Issue_Q->entries[temp].first);
			Issue_Q->entries[temp].second.vld = false;
			Issue_Q->idx_queue.emplace_back(temp);
			issue_count++;
			EX_reg.back().exe_latency = (EX_reg.back().optype == 0) ? 1 :
										((EX_reg.back().optype == 1) ? 2 : 
										((EX_reg.back().optype == 2) ? 5 : 0));
		}
		stage_idle[5] = false;
	}
}

void Execute() {
	if(EX_reg.empty()) {
		stage_idle[6] = true;
		return;
	}
	if(!EX_reg.empty()) {
		stage_idle[6] = false;
		for(int i = 0; i < (int)EX_reg.size(); i++) {
			EX_reg[i].exe_latency--;
			if(EX_reg[i].exe_latency == 0) {
				int update_dst = EX_reg[i].dst_rename;
				if(update_dst != -1) {
					//std::cout << "seq_no = " << EX_reg[i].seq_no << " cycle_count = " << cycle_count;
					//std::cout << "update_dst = " << update_dst << " exe_bypass = " << ROB_table->entries[update_dst].exe_bypass << "\n";
					ROB_table->entries[update_dst].exe_bypass = true;
					if(!DI_reg.empty()) {
						for(int j = 0; j < (int)DI_reg.size(); j++) {
							if(DI_reg[j].second.rs1 == update_dst) {
								DI_reg[j].second.rs1_rdy = true;
							}
							if(DI_reg[j].second.rs2 == update_dst) {
								DI_reg[j].second.rs2_rdy = true;
							}
						}
					}
					for(int j = 0; j < (int)Issue_Q->entries.size(); j++) {
						if(Issue_Q->entries[j].second.vld) {
							if(Issue_Q->entries[j].second.rs1 == update_dst) {
								Issue_Q->entries[j].second.rs1_rdy = true;
							}
							if(Issue_Q->entries[j].second.rs2 == update_dst) {
								Issue_Q->entries[j].second.rs2_rdy = true;
							}
						}
					}
				}
				EX_reg[i].EX.time = cycle_count + 1 - EX_reg[i].EX.entry;
				EX_reg[i].WB.entry = cycle_count + 1;
				WB_reg.emplace_back(EX_reg[i]);
			}
		}
		EX_reg.erase(
    		std::remove_if(EX_reg.begin(), EX_reg.end(),
                   		[](const operation& e){ return e.exe_latency <= 0; }),
    		EX_reg.end()
		);
	}
}

void Writeback() {
	if(WB_reg.empty()) {
		stage_idle[7] = true;
		return;
	}
	while(!WB_reg.empty()) {
		WB_reg[0].WB.time = cycle_count + 1 - WB_reg[0].WB.entry;
		WB_reg[0].RT.entry = cycle_count + 1;
		ROB_table->entries[WB_reg[0].rob_index].ready = true;
		RT_reg.emplace_back(WB_reg.front());
		WB_reg.pop_front();
	}
	stage_idle[7] = false;
}

void Retire() {
	if(RT_reg.empty()) {
		stage_idle[8] = true;
		return;
	}
	bool retired_any = false;
	for(int i = 0; i < (int)params.width; i++) {
		if(ROB_table->entries[ROB_table->head].ready) {
			if(ROB_table->entries[ROB_table->head].dst >= 0) {
				if(RMT[ROB_table->entries[ROB_table->head].dst].tag == ROB_table->head)
					RMT[ROB_table->entries[ROB_table->head].dst].vld = false;
			}
			for(int j = 0; j < (int)RT_reg.size(); j++) {
				if(RT_reg[j].seq_no == ROB_table->entries[ROB_table->head].seq_no) {
					RT_reg[j].RT.time = cycle_count + 1 - RT_reg[j].RT.entry;
					print_instruction(RT_reg[j]);
					RT_reg.erase(RT_reg.begin() + j);
					retired_any = true;
					break;
				}
			}	
			if(ROB_table->head == static_cast<int>(params.rob_size - 1)) {
				ROB_table->head = 0;
			} else {
				ROB_table->head = ROB_table->head + 1;
			}
			ROB_table->ROB_count++;
		} else {
			break;
		}
	}
	stage_idle[8] = !retired_any;
}

void print_instruction(operation& inst) {
	std::cout << inst.seq_no << " fu{" << inst.optype << "} src{" << inst.rs1 << ","
			  << inst.rs2 << "} dst{" << inst.dst
			  << "} FE{" << inst.FE.entry << "," << inst.FE.time 
			  << "} DE{" << inst.DE.entry << "," << inst.DE.time
			  << "} RN{" << inst.RN.entry << "," << inst.RN.time			  
			  << "} RR{" << inst.RR.entry << "," << inst.RR.time
			  << "} DI{" << inst.DI.entry << "," << inst.DI.time
			  << "} IS{" << inst.IS.entry << "," << inst.IS.time
			  << "} EX{" << inst.EX.entry << "," << inst.EX.time
			  << "} WB{" << inst.WB.entry << "," << inst.WB.time
			  << "} RT{" << inst.RT.entry << "," << inst.RT.time << "}\n";
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include "procsim.h"

reg_file_entry* new_rf_entry() {
	reg_file_entry* r = malloc(sizeof(reg_file_entry));
	r->ready = true;
	r->free = true;
	return r;
}

reg_file_entry** new_reg_file(int32_t size) {
	reg_file_entry** rf = malloc(size * sizeof(reg_file_entry*));
	for(int32_t i=0; i<size; i++)
		rf[i] = new_rf_entry();
	return rf;
}

scheduling_queue_entry* new_sh_entry() {
	scheduling_queue_entry* s = malloc(sizeof(scheduling_queue_entry));
	s->valid = false;
	s->fired = false;
	s->free = true;
	return s;
}

scheduling_queue_entry** new_sh(int32_t size) {
	scheduling_queue_entry** sh = malloc(size * sizeof(scheduling_queue_entry*));
	for(int32_t i=0; i<size; i++)
		sh[i] = new_sh_entry();
	return sh;
}

sh_buffer_entry** new_sh_buffer(int size) {
	sh_buffer_entry** sh_b = malloc(size * sizeof(sh_buffer_entry*));
	for(int i=0; i<size; i++) {
		sh_b[i] = malloc(sizeof(sh_buffer_entry));
		sh_b[i]->valid = false;
	}
	return sh_b;
}

rat_entry* new_rat_entry(int32_t init) {
	rat_entry* r = malloc(sizeof(rat_entry));
	r->index = init;
	return r;
}

rat_entry** new_rat(int32_t size) {
	rat_entry** rat = malloc(size * sizeof(rat_entry*));
	for(int32_t i=0; i<size; i++)
		rat[i] = new_rat_entry(i);
	return rat;
}

functional_unit_group* new_fu(uint64_t k) {
	functional_unit_group* fu = malloc(sizeof(functional_unit_group));
	fu->group = malloc(k * sizeof(functional_unit*));
	fu->size = k;

	for(uint64_t i=0; i<k; i++)
		fu->group[i] = malloc(sizeof(functional_unit));
	return fu;
}

functional_unit_group** new_funits(uint64_t k0, uint64_t k1, uint64_t k2) {
	functional_unit_group** funits = malloc(3 * sizeof(functional_unit_group*));

	funits[0] = new_fu(k0);
	funits[1] = new_fu(k1);
	funits[2] = new_fu(k2);

	return funits;
}

rob_entry* new_rob_entry() {
	rob_entry* entry = malloc(sizeof(rob_entry));
	entry->free = true;
	entry->completed = false;
	return entry;
}

_rob* new_rob(uint64_t size) {
	_rob* rob = malloc(sizeof(rob));
	rob_entry** entry_list = malloc(size * sizeof(rob_entry*));
	for(uint64_t i=0; i<size; i++)
		entry_list[i] = new_rob_entry();
	rob->entries = entry_list;
	rob->size = size;
	rob->oldest_index = 0;
	return rob;
}

/*
 * Returns index to a free slot in the scheduling queue, and -1 if the queue is full.
 */
int find_free_sh_slot() {
	for(int32_t i=0; i<rs_size; i++) {
		if (sh_queue[i]->free)
			return i;
	}
	return -1;
}

int find_free_preg() {
	for(int32_t i=32; i<preg+32; i++) {
		if (reg_file[i]->free)
			return i;
	}
	return -1;
}

int find_free_fu(int32_t op) {
	for(int i=0; i<funits[op]->size; i++) {
		if (!funits[op]->group[i]->busy)
			return i;
	}
	return -1;
}

int find_free_rob() {
	for(int i=rob->oldest_index; i<rob->size; i++) {
		if (rob->entries[i]->free)
			return i;
	}
	for(int i=0; i<rob->oldest_index; i++) {
		if (rob->entries[i]->free)
			return i;
	}
	return -1;
}

//adds sh entries in program order to buffer
void add_to_sh_buffer(int sh_index, uint64_t tag) {
	for(int i=0; i<rs_size; i++) {
		if (!sh_buffer[i]->valid || tag < sh_buffer[i]->tag) {
			//insert new entry here
			//start by moving all entries past index i in buffer down 1
			for(int j=rs_size-1; j>i; j--) {
				if (sh_buffer[j-1]->valid) {
					sh_buffer[j]->valid = true;
					sh_buffer[j]->sh_index = sh_buffer[j-1]->sh_index;
					sh_buffer[j]->tag = sh_buffer[j-1]->tag;
				}
			}

			sh_buffer[i]->valid = true;
			sh_buffer[i]->sh_index = sh_index;
			sh_buffer[i]->tag = tag;
			return;
		}
	}
	//newest instr in buffer
	sh_buffer[rs_size-1]->valid = true;
	sh_buffer[rs_size-1]->sh_index = sh_index;
	sh_buffer[rs_size-1]->tag = tag;
}

/*
 * Issues instruction to given FU.
 */
void issue(scheduling_queue_entry* sh, int fu_index, int sh_index) {
	*funits[sh->op_code]->group[fu_index] = (functional_unit){
		.tag = sh->tag,
		.sh_index = sh_index,
		.busy = true
	};
	return;
}

bool sh_empty() {
	for(int i=0; i<rs_size; i++) {
		if (sh_queue[i]->valid && !sh_queue[i]->fired)
			return false;
	}
	return true;
}

bool ex_empty() {
	for(int i=0; i<3; i++) {
		for(int j=0; j<funits[i]->size; j++) {
			if (funits[i]->group[j]->busy)
				return false;
		}
	}
	return true;
}

bool rob_empty() {
	for(int i=0; i<rob_size; i++) {
		if (!rob->entries[i]->free)
			return false;
	}
	return true;
}

/*
 * Print internal information nicely.
 */
void print_cycle() {
	// //print RAT
	// // fprintf(out, "========================\n%-10s| %-15s\n----------|-------------\n", "RAT Index", "AREG/PREG ID");
	// for(int i=0; i<32; i++)
	// 	// fprintf(out, "%9d | %-14d\n", i, rat[i]->index);
	// // fprintf(out, "=============================================================\n");

	// //print Scheduling Queue
	// // fprintf(out, "%-10s | %-10s | %-10s | %-10s | %-10s\n", "Instr Num", "FU", "DEST PREG", "SRC1 PREG", "SRC2 PREG");
	// // fprintf(out, "-----------|------------|------------|------------|----------\n");
	// for(int32_t i=0; i<rs_size; i++) {
	// 	if (sh_queue[i]->valid)
	// 		// fprintf(out, "%-10" PRIu64 " | %-10d | %-10d | %-10d | %-10d\n", 
	// 			sh_queue[i]->tag, sh_queue[i]->op_code, sh_queue[i]->dest_preg, sh_queue[i]->src_reg[0], sh_queue[i]->src_reg[1]);
	// }
	// // fprintf(out, "=============================================================\n");

	// //print ROB
	// // fprintf(out, "%-10s | %-10s | %-10s\n", "Instr Num", "DEST AREG", "PREV PREG");
	// // fprintf(out, "-----------|------------|----------\n");
	// for(int i=0; i<rob->size; i++) {
	// 	if (!rob->entries[i]->free)
	// 		// fprintf(out, "%-10" PRIu64 " | %-10d | %-10d\n", rob->entries[i]->tag, rob->entries[i]->areg, rob->entries[i]->prev_preg);
	// }
	return;
}

void dispatch() {
	for(int i=0; i<f; i++) {
		//check for free slots in scheduling queue and pregs
		int free_sh = find_free_sh_slot();
		int free_preg = find_free_preg();
		int free_rob = find_free_rob();

		//some slot not free, can't dispatch
		if (free_sh == -1 || free_preg == -1 || free_rob == -1)
			return;

		if (read_instruction(instr)) {
			instr_count++;
			// fprintf(out, "DU: Instr Number %" PRIu64 "\n", instr_count);

			//rob update
			*rob->entries[free_rob] = (rob_entry){
				.free = false,
				.completed = false,
				.sh_index = free_sh,
				.tag = instr_count,
				.areg = instr->dest_reg,
				.prev_preg = (instr->dest_reg == -1) ? -1 : rat[instr->dest_reg]->index
			};

			//sh queue entry
			*sh_queue[free_sh] = (scheduling_queue_entry){
				.valid = true,
				.fired = false,
				.free = false,
				.tag = instr_count,
				.op_code = (instr->op_code == -1) ? 1 : instr->op_code,
				.dest_preg = (instr->dest_reg == -1) ? -1 : free_preg,
				.src_reg[0] = (instr->src_reg[0] == -1) ? -1 : rat[instr->src_reg[0]]->index,
				.src_reg[1] = (instr->src_reg[1] == -1) ? -1 : rat[instr->src_reg[1]]->index,
				.rob_index = free_rob,
				.index = free_sh
			};

			//setup pregs if instr uses a dest reg
			if (instr->dest_reg != -1) {
				rat[instr->dest_reg]->index = free_preg;
				reg_file[free_preg]->ready = false;
				reg_file[free_preg]->free = false;
			}
		} else {
			if (rob_empty())
				done = true;
			return;
		}
	}
}

void schedule() {
	//look through sh queue and put all instr in buffer where both src regs ready
	for(int i=0; i<rs_size; i++) {
		//check sh entry is valid and not already fired
		if (sh_queue[i]->valid && !sh_queue[i]->fired) {
			//check if src regs both ready
			//int fu_index = find_free_fu(sh_queue[i]->op_code);
			if ((sh_queue[i]->src_reg[0] == -1 || reg_file[sh_queue[i]->src_reg[0]]->ready) &&
				(sh_queue[i]->src_reg[1] == -1 || reg_file[sh_queue[i]->src_reg[1]]->ready)) {
				add_to_sh_buffer(i, sh_queue[i]->tag);
				// issue(sh_queue[i], fu_index, i);
				// sh_queue[i]->fired = true;
				// fprintf(out, "SCHED: Instr Number %" PRIu64 "\n", sh_queue[i]->tag);
			}
		}
	}

	// fprintf(out, "SH BUFFER: ");
	// // for(int i=0; i<rs_size; i++) {
	// // 	if (sh_buffer[i]->valid)
	// 		fprintf(out, "%d ", sh_buffer[i]->tag);
	// // }
	// fprintf(out, "\n");

	//go through buffer and fire all possible
	for(int i=0; i<rs_size; i++) {
		if (sh_buffer[i]->valid && !sh_queue[sh_buffer[i]->sh_index]->fired) {
			int fu_index = find_free_fu(sh_queue[sh_buffer[i]->sh_index]->op_code);
			//check again that src regs ready
			if ((fu_index != -1) &&
				(sh_queue[sh_buffer[i]->sh_index]->src_reg[0] == -1 || reg_file[sh_queue[sh_buffer[i]->sh_index]->src_reg[0]]->ready) &&
				(sh_queue[sh_buffer[i]->sh_index]->src_reg[1] == -1 || reg_file[sh_queue[sh_buffer[i]->sh_index]->src_reg[1]]->ready)) {
				//issue now in program order
				issue(sh_queue[sh_buffer[i]->sh_index], fu_index, sh_buffer[i]->sh_index);
				sh_queue[sh_buffer[i]->sh_index]->fired = true;
				// fprintf(out, "SCHED: Instr Number %" PRIu64 "\n", sh_queue[sh_buffer[i]->sh_index]->tag);
			}
		}
	}
	//reset buffer
	for(int i=0; i<rs_size; i++) {
		sh_buffer[i]->valid = false;
	}
}

void execute() {
	//look thru FUs, if busy means was issued instr last cycle and is now done (all instr take 1 cycle to complete)
	for(int i=0; i<3; i++) {
		for(int j=0; j<funits[i]->size; j++) {
			if (funits[i]->group[j]->busy) {
				// fprintf(out, "EX: Type %d: Instr Number %" PRIu64 "\n", i, funits[i]->group[j]->tag);

				//free FU for more instructions and mark ROB entry as completed
				funits[i]->group[j]->busy = false;
				rob->entries[sh_queue[funits[i]->group[j]->sh_index]->rob_index]->completed = true;

				//dest preg ready now
				if (sh_queue[funits[i]->group[j]->sh_index]->dest_preg != -1)
					reg_file[sh_queue[funits[i]->group[j]->sh_index]->dest_preg]->ready = true;
			}
		} 
	}
}

void state_update() {
	//update ROB
	while (rob->entries[rob->oldest_index]->completed) {
		// fprintf(out, "RU: Instr Number %" PRIu64 "\n", rob->entries[rob->oldest_index]->tag);
		rob->entries[rob->oldest_index]->completed = false;

		//free sh queue slot
		sh_queue[rob->entries[rob->oldest_index]->sh_index]->free = true;
		sh_queue[rob->entries[rob->oldest_index]->sh_index]->valid = false;
		sh_queue[rob->entries[rob->oldest_index]->sh_index]->fired = false;

		//free prev preg
		if (rob->entries[rob->oldest_index]->prev_preg != -1)
			reg_file[rob->entries[rob->oldest_index]->prev_preg]->free = true;

		//free rob entry
		rob->entries[rob->oldest_index]->free = true;

		rob->oldest_index++;
		if (rob->oldest_index == rob_size)
			rob->oldest_index = 0;
	}
}

/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 *
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 * @ROB Number of ROB Entries
 * @PREG Number of registers in the PRF
 */
void setup_proc(uint64_t _k0, uint64_t _k1, uint64_t _k2, uint64_t _f, uint64_t _rob, uint64_t _preg) {
	// debug = fopen("debug.out", "w");
	// out = fopen("out.out", "w");

	k0 = _k0;
	k1 = _k1;
	k2 = _k2;
	f = _f;
	rob_size = _rob;
	preg = _preg;
	rs_size = 2 * (k0 + k1 + k2);
	instr_count = 0;
	instr = malloc(sizeof(proc_inst_t));

	sh_queue = new_sh(rs_size);
	sh_buffer = new_sh_buffer(rs_size);
	reg_file = new_reg_file(32 + preg);
	rat = new_rat(32);
	funits = new_funits(k0, k1, k2);
	rob = new_rob(rob_size);

	dispatch_done = sched_done = ex_done = done = false;
}

/**
 * Subroutine that simulates the processor.
 * The processor should fetch instructions as appropriate, until all instructions have executed
 *
 * @p_stats Pointer to the statistics structure
 */
void run_proc(proc_stats_t* p_stats) {
	while (!done) {
		clock++;
		// fprintf(out, "==============\n");
		// fprintf(out, "CYCLE %8" PRIu64 "\n", clock);
		// fprintf(out, "==============\n");
		state_update();
		execute();
		schedule();
		dispatch();
		//print_cycle();
	}
	//printf("%d %d %d %d\n", dispatch_done ? 1 : 0, ex_done ? 1 : 0, sched_done ? 1 : 0, done ? 1 : 0);

	//print_rs();
	// print_regs();
	//print_rat();
}

/**
 * Subroutine for cleaning up any outstandizng instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats) {
	p_stats->avg_inst_retired = (float)(instr_count) / (float)(clock);
	p_stats->avg_inst_fired = (float)(instr_count) / (float)(clock);
	p_stats->retired_instruction = instr_count;
	p_stats->cycle_count = clock;

	// fclose(debug);
	// fclose(out);
}
#ifndef PROCSIM_H
#define PROCSIM_H

#define DEFAULT_K0 3
#define DEFAULT_K1 2
#define DEFAULT_K2 1
#define DEFAULT_ROB 12
#define DEFAULT_F 4
#define DEFAULT_PREG 32

typedef struct _proc_inst_t {
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
} proc_inst_t;

typedef struct _sh_entry {
	bool valid, fired, free;
	uint64_t tag;
	int32_t op_code;
	int32_t dest_preg;
	int32_t src_reg[2];
	int rob_index;
	int index;
} scheduling_queue_entry;

typedef struct {
	bool valid;
	int sh_index;
	int tag;
} sh_buffer_entry;

typedef struct _rf_entry {
	bool ready, free;
	uint64_t value;
} reg_file_entry;

typedef struct _rat_entry {
	int32_t index;
} rat_entry;

typedef struct _fu {
	uint64_t tag;
	int sh_index;
	bool busy;
} functional_unit;

typedef struct _fu_g {
	int size;
	functional_unit** group;
} functional_unit_group;

typedef struct _rob_entry {
	bool free;
	bool completed;
	int sh_index;
	uint64_t tag;
	int32_t areg;
	int32_t prev_preg;
} rob_entry;

typedef struct {
	rob_entry** entries;
	int size;
	int oldest_index;
} _rob;

typedef struct _proc_stats_t {
    float avg_inst_retired;
    float avg_inst_fired;
    unsigned long retired_instruction;
    unsigned long cycle_count;
} proc_stats_t;

//drivers
bool read_instruction(proc_inst_t* p_inst);
void setup_proc(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t rob, uint64_t preg);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

//constructors
scheduling_queue_entry** new_sh(int32_t size);
sh_buffer_entry** new_sh_buffer(int size);
reg_file_entry** new_reg_file(int32_t size);
rat_entry** new_rat(int32_t size);
functional_unit_group** new_funits(uint64_t k0, uint64_t k1, uint64_t k2);
_rob* new_rob(uint64_t size);

//stages
void dispatch();
void schedule();
void execute();
void state_update();

//helpers
int find_free_sh_slot();
int find_free_preg();
int find_free_rob();
int find_free_fu(int32_t op);
bool sh_empty();
bool ex_empty();
bool rob_empty();
void add_to_sh_buffer(int sh_index, uint64_t tag);
void issue();
void print_cycle();

//global vars
// FILE* debug;
// FILE* out;
uint64_t k0, k1, k2, f, rob_size, preg, clock, instr_count;
int32_t rs_size;
bool dispatch_done, sched_done, ex_done, done;
proc_inst_t* instr;

//hardware components
scheduling_queue_entry** sh_queue;
sh_buffer_entry** sh_buffer;
reg_file_entry** reg_file;
rat_entry** rat;
_rob* rob;
functional_unit_group** funits;

#endif /* PROCSIM_H */
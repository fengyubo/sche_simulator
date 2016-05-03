#include "sched_bfair.h"
#include "sched_pfair.h"
#include "experiments.h"
#include "scheduler_dvfs_bfair.h"

int main() {
	experiments expm("config.cfg");
	expm.print_config();
	expm.run();
	return 0;
}
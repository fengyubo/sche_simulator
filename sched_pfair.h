#ifndef __SCHED__SCHED_PFAIR_H
#define __SCHED__SCHED_PFAIR_H
#include "sched.h"
// Pfair scheduler algoritm
class Pfair : public Scheduler {
public:
	typedef enum {
		VALID_SCHE,
		INVALID_SCHE,
		ERR_PARM,
		NOAVI_TASK,
		OVER_ALLOCATED,
		LACK_ALLOCATED,
	} Err_Code_Sche_Pfair;

	struct task_pfair : task_tpl{
		long long next_pd;
		long long accu_c;	/*accumulated allocation for the current period*/
		long long ou;		/*optional units during allocating [b_(k-1), b_k);*/
		long long pw;		/*pending work after allocating mandatory part: pw = (b_k - next_pd)*u - accu_c - mu;*/
		unsigned int ch;	/*character of next_pd;*/

		long long lastt;
		long long lag;
		long long next_punc;
	};

	Pfair(const long long stop_point, const int num_proc) : Scheduler(stop_point, num_proc) {}

private:
	int PFChar(const vector<task_pfair> &tasks, int i, long long t);
	int PFNaiveCompare(const vector<task_pfair> &tasks, int i, int j, long long t);
	int PFSelectNextTask(const vector<task_pfair> &tasks, long long t);
	int PFSelectTasksAndAllocation(vector<task_pfair> &tasks, long long t, FILE* fp = stderr);
	inline vector<Pfair::task_pfair> create_pfair_task_set(const TaskSet *ts);
	int exec_full_util_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp = stderr, EnergyModel *em = nullptr);
};
#endif
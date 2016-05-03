#ifndef __SCHED__SCHED_BFAIR_H
#define __SCHED__SCHED_BFAIR_H
#include "sched.h"
// Bfair scheduler algoritm
class Bfair : public Scheduler {
public:
	typedef enum{
		VALID_SCHE, // there is no error in the schedule table
		INVALID_SCHE,
		ERR_PARM,
		CONFLICT_TIME_SLOTS, // conflict found at some time slots for task
		NO_SUFF_TIME_ALL, // some task does not get enough time for some instance at time
		MUCH_TIME_ALL, // some task does not get enough time for some instance at time
		ERR_ALLOC,
	} Err_Code_Sche_Bfair; // error code for schedule table result check

	struct task_bfair : task_tpl{
		// bfair scheduler use data structure
		long long next_pd;	/*next period boundary*/
		long long last_pd;	/*next period boundary*/
		long long accu_c;	/*accumulated allocation for the current period*/

		long long mu;		/*mandatory units during allocating [b_(k-1), b_k);*/
		long long pw;		/*pending work after allocating mandatory part: pw = (b_k - next_pd)*u - accu_c - mu;*/
		long long ou;		/*optional units during allocating [b_(k-1), b_k);*/
		long long rw;		/*remaining work after allocating [b_(k-1), b_k): rw = pw - ou;*/

		double uf;			/*urgent factor; used by Boundary-fair scheduler*/
		unsigned int ch;	/*character of next_pd;*/

		int ipb;
		int index;
	};

	Bfair(const long long stop_point, const int num_proc);

private:
	const int elig_size = 3;
	vector<vector<int> > _eligibleTaskSet;
	vector<int> _numEligibleTasks;

	inline void init_eligibleTaskSet(int num_proc);
	inline void init_numEligibleTasks();
	int exec_full_util_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp = stderr, EnergyModel *em = nullptr);
	inline vector<task_bfair> create_bfair_task_set(const TaskSet *ts) const;
	
	// bfair help function
	long long GetAllSchedulingPoints(vector<Bfair::task_bfair> &tasks, vector<unsigned long long> &sps);
	int FastBoundaryCompare(vector<Bfair::task_bfair> &tasks, int a, int b);
	int BoundaryChar(Bfair::task_bfair cur_task, long long cpb, long long npb);
	double CalUF(Bfair::task_bfair cur_task, long long pb_ks, int counter);
	unsigned long long CalMU(long long start_t, long long end_t, Bfair::task_bfair *task);
};
#endif
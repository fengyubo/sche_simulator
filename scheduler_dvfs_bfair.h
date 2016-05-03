#ifndef __SCHED__SCHED_DVFS_BFAIR_H
#define __SCHED__SCHED_DVFS_BFAIR_H
#include "sched.h"
#include "sched_bfair.h"

class dvfs_Bfair : public Scheduler {
public:
	dvfs_Bfair(const long long stop_point, const int num_proc, const EnergyModel* em = nullptr, long long scale = 1): Scheduler(stop_point, num_proc, em), _scale(scale), _prev_k(0) {} 
	void init_sche(const long long stop_point, const int num_proc, EnergyModel *em = nullptr);
	Err_Code_Sche_Alg exec_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp = stderr, EnergyModel *em = nullptr);
	Err_Code_Sche_Tab check_val_sche(const TaskSet *ts, FILE* fp = stderr);

private:
	static bool cmp(const task_tpl &a, const task_tpl &b);
	long long _scale;
	int _prev_k;
	TaskSet _rest_tasks;
	vector<vector<int> > _rest_schdule;
};
#endif
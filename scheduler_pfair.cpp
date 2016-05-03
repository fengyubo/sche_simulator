#include "sched_pfair.h"

/* similar to Bfair, this method will be called on full uti task set, which means that the task set is 100% uti,
 * also, this function is isolated: which means if you know that ts pointing to a task set with 100% uti, then acutally you could call this 
 * function directly instead of call exec_sche
 * however, considering safe and keep the project as simple as possible, necessary copy work will occured here also.
 * "generate the schedule for 'm' tasks on 'n' processors within LCM"
*/
int Pfair::exec_full_util_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp, EnergyModel *em) {
	if(ts == nullptr || ts->get_uti() < (double)num_proc) return (int)Err_Code_Sche_Pfair::ERR_PARM;
	
	vector<Pfair::task_pfair> tasks = create_pfair_task_set(ts);

	// reset schedule table and speed vector
	_stop_point = min(ts->get_lcm(), stop_point);
	_num_proc = num_proc;
	reset_sche_tab(_stop_point, _num_proc);

	const int numTasks = tasks.size();
	for(long long t = 0l;  t < _stop_point; ++t) {
		long long tmpL = 0l;
		for(int i = 0; i < numTasks; ++i) {
			if ((t >= tasks[i].next_punc) && (i != numTasks-1)) {
			    tmpL = tasks[i].c;
			    ++tasks[i].next_punc;
			    while ( (tmpL%tasks[i].p) ) {
					++tasks[i].next_punc;
					tmpL += tasks[i].c;
				}
			}
			tasks[i].ou=0;
			tasks[i].ch = PFChar(tasks, i, t); 
		}
		if (PFSelectTasksAndAllocation(tasks, t, fp)) return (int)Err_Code_Sche_Pfair::NOAVI_TASK;
		for(int i = 0; i < numTasks; ++i) {
			tasks[i].lastt = t+1;
			tasks[i].lag = (tasks[i].lag + tasks[i].c)%tasks[i].p;

			if (!((t+1)%tasks[i].p)) {
				if (tasks[i].accu_c > tasks[i].c) {
					fprintf(fp, "error: time %lld task %d instance %d, OVER_AOOLCATED (%lld, %lld)\n", t, i, (int)(t/tasks[i].p), tasks[i].accu_c , tasks[i].c );
					return (int)Err_Code_Sche_Pfair::OVER_ALLOCATED;
				}else if (tasks[i].accu_c < tasks[i].c) {
					fprintf(fp, "error: time %lld task %d instance %d, NOT_ENOUGH (%lld, %lld)\n", t, i, (int)(t/tasks[i].p), tasks[i].accu_c , tasks[i].c);
					return (int)Err_Code_Sche_Pfair::LACK_ALLOCATED;
				}else {}

				tasks[i].next_pd = t;
				tasks[i].accu_c = 0; 
				tasks[i].pw = tasks[i].c;
			}else {
				tasks[i].pw += tasks[i].c;
			}

			if (tasks[i].lag == 0) tasks[i].pw =0;
		}
		// fill the schedule table
		for(int i = 0, proc = 0; i < numTasks; ++i) {
			if (tasks[i].ou && proc<num_proc) {
				(*_schedule)[proc][t] = i;
				++proc;
			}
		}
	}
	return (int)Err_Code_Sche_Pfair::VALID_SCHE;
}

/*
 * create the full uti task set, NOT change the previous task set at all
*/
inline vector<Pfair::task_pfair> Pfair::create_pfair_task_set(const TaskSet *ts) {
	vector<Pfair::task_pfair> tasks;
	if(ts == nullptr) return tasks;
	for(auto k : ts->get_task_set()) {
		Pfair::task_pfair t;
		// origianl struct copy
		t.c = k.c;
		t.p = k.p;
		t.u = k.u;
		// for sepcial data useage
		t.next_pd =0;
		t.accu_c =0; 
		t.ou=0; 
		t.pw=0; 
		t.lag = 0;
		t.lastt =0;
		t.next_punc = 0;
		tasks.push_back(t);
	}
	tasks.back().next_punc = tasks.back().p;
	return tasks;
}

// all following methods are help function, directly copied from last version of code,
// unless you know what you are doing, otherwise do not change things here

int Pfair::PFChar(const vector<task_pfair> &tasks, int i, long long t) {
	long long tmp = ((t+1 == tasks[i].next_punc) ? 0 : (tasks[i].c * (t+1) - ((tasks[i].c * t) / tasks[i].p) * tasks[i].p - tasks[i].p));
	return (tmp < 0 ? 0 : (tmp > 0 ? 2 : 1));
}

int Pfair::PFNaiveCompare(const vector<task_pfair> &tasks, int i, int j, long long t) {
	long long tmp = t+1;
	while ( (PFChar(tasks, i, tmp) == PFChar(tasks, j, tmp)) && (PFChar(tasks, i, tmp) != 1) ) ++tmp;
	return ( PFChar(tasks, i, tmp) > PFChar(tasks, j, tmp) ? 1 : 0); // i is more urgent
}

int Pfair::PFSelectNextTask(const vector<task_pfair> &tasks, long long t) {
	const int numTasks = tasks.size();
	int i = 0, index;

	while ( (i<numTasks) && ( (tasks[i].ou ==1) || ( (tasks[i].pw<0)&&(tasks[i].ch!=2) ) ) ) ++i;
	if (i == numTasks ) return -1;

	if ( (tasks[i].pw>0)&&(tasks[i].ch!=0) )	return i;
	else index = i;

	i = index+1;
	while ( (i<numTasks) && ( (tasks[i].ou ==1) || ( (tasks[i].pw<0)&&(tasks[i].ch!=2) ) ) ) ++i;
	if (i == numTasks ) return index;

	while (i < numTasks) {
		if ( (tasks[i].pw>0)&&(tasks[i].ch!=0) )
			return i;

		if ( PFNaiveCompare(tasks, i, index, t) ) 
			index =i;
		
		i++;
		while ( (i<numTasks) && ( (tasks[i].ou ==1) || ( (tasks[i].pw<0)&&(tasks[i].ch!=2) ) ) ) ++i;
		if (i == numTasks ) return index;
	}
	return index;
}

int Pfair::PFSelectTasksAndAllocation(vector<task_pfair> &tasks, long long t, FILE* fp) {
	for(int index = 0, tau = 0; _num_proc - tau > 0; ++tau ) {
		// here _num_proc used
		if ( (index = PFSelectNextTask(tasks, t)) == -1) {
			fprintf(fp, "error: no available task, at time %lld \n", t);
			return -1;
		}

		tasks[index].ou = 1;
		++tasks[index].accu_c;
		tasks[index].pw -= tasks[index].p;
	}
	return 0;
}
#include "sched_bfair.h"

Bfair::Bfair(const long long stop_point, const int num_proc) : Scheduler(stop_point, num_proc) {
	_numEligibleTasks.resize(elig_size);
	_eligibleTaskSet.resize(elig_size);
	_error_code = (int)Err_Code_Sche_Bfair::VALID_SCHE;
}

/* in this exec_full_util_sche, it should guarantee that "total utilization of task set = num_proc", which means here, "ts->uti == num_proc"
 * s.t, the actually schedule is for 100% uti model
 * Unless you know what you are doing, otherwise do not touch this part of the function
*/
int Bfair::exec_full_util_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp, EnergyModel *em) {
	if(ts == nullptr || ts->get_uti() < (double)num_proc) return (int)Err_Code_Sche_Bfair::ERR_PARM;
	
	vector<Bfair::task_bfair> tasks = create_bfair_task_set(ts);
	// init two helper data structure
	init_eligibleTaskSet(tasks.size());
	init_numEligibleTasks();

	// reset schedule table and speed vector
	_num_proc = num_proc;
	_stop_point = min(stop_point, ts->get_lcm());
	reset_sche_tab(_stop_point, _num_proc);

	vector<unsigned long long> sps(_stop_point, 0);
	long long num_sps = GetAllSchedulingPoints(tasks, sps);

	long long ipb = 0, t = sps.front();
	
	do {
		long long nnpb = sps[ipb+1];
		long long tau = 0, trw = 0;
		fill(_numEligibleTasks.begin(), _numEligibleTasks.end(), 0);

		for(int i=0; i < tasks.size(); ++i) {
			if (tasks[i].next_pd == t) {
				if (tasks[i].accu_c - tasks[i].c < -1) {
					fprintf(fp, "task-%d (%lld, %lld) is UNDER allocated %lld !!!\n", i, tasks[i].c, tasks[i].p, tasks[i].accu_c);
				}else if (tasks[i].accu_c - tasks[i].c > 1) {
					fprintf(fp, "task-%d (%lld, %lld) is OVER allocated %lld !!!\n", i, tasks[i].c, tasks[i].p, tasks[i].accu_c);	
				}
				
				tasks[i].next_pd += tasks[i].p;
				tasks[i].accu_c = 0; 
				tasks[i].mu =0;
				tasks[i].ou =0;
				tasks[i].pw =0;
				tasks[i].rw =0;
			}

			tasks[i].mu = CalMU(t, nnpb, &(tasks[i]));
			tasks[i].pw = tasks[i].rw;
			tasks[i].accu_c += tasks[i].mu;

			tau += tasks[i].mu;
			trw += tasks[i].rw;
			tasks[i].ou = 0;
			tasks[i].ch = BoundaryChar(tasks[i], nnpb, sps[ipb+2]);

			if (tasks[i].ch == 0) {
				tasks[i].uf = CalUF(tasks[i], nnpb, 0);	
			}else if(tasks[i].ch == 2) {
				tasks[i].uf = CalUF(tasks[i], nnpb, 1);
			}else {}

			if ((tasks[i].pw > 0) && (tasks[i].mu < nnpb - t))
			{
			    _eligibleTaskSet[tasks[i].ch][_numEligibleTasks[tasks[i].ch]] = i;
			    ++_numEligibleTasks[tasks[i].ch];
			}
		}

		int cur_cat = -1;
		bool stop = false;
		for(int j=2; (j >= 0) && !stop; --j) {
		    if ( (_num_proc * (nnpb-t) - tau) >= _numEligibleTasks[j] ) {
				/*every task in category j get one optional unit*/
				for (int i = 0; i < _numEligibleTasks[j]; ++i) {
				    tasks[_eligibleTaskSet[j][i]].ou = 1;
				    ++tasks[_eligibleTaskSet[j][i]].mu; 
				    tasks[_eligibleTaskSet[j][i]].rw = tasks[_eligibleTaskSet[j][i]].pw - tasks[_eligibleTaskSet[j][i]].p;
				    ++tasks[_eligibleTaskSet[j][i]].accu_c;
				
				    trw -= tasks[_eligibleTaskSet[j][i]].p;
				    ++tau;
				}
			}else{
				/* need to search in category j*/
				cur_cat = j;
				stop = true;
			}
		}

		for(bool exist_eligible_task = true; (_num_proc * (nnpb - t) - tau > 0) && (exist_eligible_task); ++tau ) {
			int i = 0, index = -1;
			for(; (i<_numEligibleTasks[cur_cat]) && ((tasks[_eligibleTaskSet[cur_cat][i]].pw - (tasks[_eligibleTaskSet[cur_cat][i]].ou * tasks[_eligibleTaskSet[cur_cat][i]].p) <= 0) || (tasks[_eligibleTaskSet[cur_cat][i]].accu_c == tasks[_eligibleTaskSet[cur_cat][i]].c)); ++i) {}
			if ( i < _numEligibleTasks[cur_cat]) {
				index = _eligibleTaskSet[cur_cat][i];
			}else {
				fprintf(fp, "no available task error\n");
				exist_eligible_task = false;
				break;
			}

			for( ++i; i < _numEligibleTasks[cur_cat]; ++i) {
				if ( (tasks[_eligibleTaskSet[cur_cat][i]].pw - (tasks[_eligibleTaskSet[cur_cat][i]].ou * tasks[_eligibleTaskSet[cur_cat][i]].p) > 0) && (tasks[_eligibleTaskSet[cur_cat][i]].mu < nnpb - t) && (FastBoundaryCompare(tasks, _eligibleTaskSet[cur_cat][i], index)) ) {
					index = _eligibleTaskSet[cur_cat][i]; 
				}
			}

			tasks[index].ou = 1;
			++tasks[index].mu; 
			tasks[index].rw = tasks[index].pw - tasks[index].p;
			++tasks[index].accu_c;
			trw -= tasks[index].p;
		}
			
		if (_num_proc * (nnpb-t) != tau) {
		    fprintf(fp, "allocation error for %lld --> %lld \n", t, nnpb);
			return (int)Err_Code_Sche_Bfair::ERR_ALLOC;
		}
		
		int proc = 0; 
		long long cur = t;

		/* Schedule the subtasks.*/
		for (int i = 0; i < tasks.size(); ++i) {
			while ( (cur < nnpb) && (tasks[i].mu > 0) ) {
				(*_schedule)[proc][cur] = i;
				--tasks[i].mu;
				++cur;
			}

			if( tasks[i].mu > 0 ) {
				++proc;
				cur = t;
				while ( (cur < nnpb) && (tasks[i].mu>0) ) {
					(*_schedule)[proc][cur] = i;
					--tasks[i].mu;
					++cur;
				}
			}
		}

		++ipb;
		t = sps[ipb];
	} while(sps[ipb + 1] < _stop_point && sps[ipb + 1] != 0);
	return (int)Err_Code_Sche_Bfair::VALID_SCHE;
}

long long Bfair::GetAllSchedulingPoints(vector<Bfair::task_bfair> &tasks, vector<unsigned long long> &sps) {
	long long curr_sps = 0;
	long long tmp_lcm = _stop_point;
	long long min_pd;
	long long t = 0;
	
	sps[curr_sps] = 0;

	for(int i=0; i<tasks.size(); i++)
		tasks[i].next_pd = tasks[i].p;

	while (t < tmp_lcm) {
		++curr_sps;
		
		min_pd = tasks[0].next_pd;
		for(int i = 1; i < tasks.size(); ++i) {
			min_pd = min(min_pd, tasks[i].next_pd);
		}
		
		t = min_pd;
		if (curr_sps < tmp_lcm)  sps[curr_sps] = t;

		for(int i = 0 ;i < tasks.size(); ++i) {
			if (tasks[i].next_pd == min_pd) {
				tasks[i].next_pd += tasks[i].p;	
			}
		}
	}

	return curr_sps + 1;
}

inline vector<Bfair::task_bfair> Bfair::create_bfair_task_set(const TaskSet *ts) const {
	vector<Bfair::task_bfair> ans;
	if(ts == nullptr) return ans;
	for(auto k : ts->get_task_set()) {
		task_bfair ttask;
		ttask.c = k.c;
		ttask.p = k.p;
		ttask.u = k.u;
		ttask.next_pd = k.p;
		ttask.index = ans.size();
		ttask.mu=0; 
		ttask.pw=0;	
		ttask.ou=0; 
		ttask.rw=0;
		ttask.accu_c = k.c;
		ans.push_back(ttask);
	}
	return ans;
}

inline void Bfair::init_eligibleTaskSet(int num_tasks) {
	for(int i = 0; i < elig_size; ++i) {
		if(_eligibleTaskSet[i].size() != num_tasks) {
			_eligibleTaskSet[i].resize(num_tasks);
		}
		fill(_eligibleTaskSet[i].begin(), _eligibleTaskSet[i].end(), 255);
	}
	return ;
}

inline void Bfair::init_numEligibleTasks() {
	fill(_numEligibleTasks.begin(), _numEligibleTasks.end(), 0);
	return ;
}


int Bfair::FastBoundaryCompare(vector<Bfair::task_bfair> &tasks, int a, int b) {
	if ( tasks[a].ch > tasks[b].ch ) return 1;	/*task-a has higher priority*/
	else if ( tasks[a].ch < tasks[b].ch ) return 0;	/*task-b has higher priority*/
	else if ( (tasks[a].ch == tasks[b].ch)&&(tasks[a].ch == 0) ) {
		/*both has a '-', compare urgent factor*/
		if (tasks[a].uf < tasks[b].uf) return 1;
		else return 0;
	}else if ( (tasks[a].ch==tasks[b].ch) && (tasks[a].ch ==2) ) {
		/*both has a '+', instead of looking ahead to future boundaries*/
		/* we compare urgent factor for the correspoinding counter-tasks u'= (1 - u)*/
		if (tasks[a].uf < tasks[b].uf) return 0;	/* a' > b' --> a <  b*/
		else return 1; /* a'<= b' --> a >= b*/
	}
	return 0;
}

int Bfair::BoundaryChar(Bfair::task_bfair cur_task, long long cpb, long long npb) {
	long long tmp = cur_task.c * npb - ((cpb * cur_task.c) / cur_task.p) * cur_task.p - (npb - cpb) * cur_task.p;
	if ( tmp < 0 ) {
		/* negtive  (-)*/
		return 0;
	}else if( tmp == 0) {
		return 1;
	}else {
		return 2;
	}
}

double Bfair::CalUF(Bfair::task_bfair cur_task, long long pb_ks, int counter) {
	/* Calculate the urgent factor for regular tasks. */
	if (!counter)
		return (double) (cur_task.p - (pb_ks * cur_task.c - ((pb_ks * cur_task.c) / cur_task.p) * cur_task.p)) / (double) cur_task.c;
	/* Calculate the urgent factor for counter tasks. */
	else
		return (double) (cur_task.p - (pb_ks * (cur_task.p - cur_task.c) - ((pb_ks * (cur_task.p - cur_task.c)) / cur_task.p) * cur_task.p)) / (double) (cur_task.p - cur_task.c);
}

unsigned long long Bfair::CalMU(long long start_t, long long end_t, Bfair::task_bfair *task) {
	long long gap = end_t - start_t;
	unsigned long long mu = 0;

	for(int i = 0; i < gap; ++i) {
		task->rw = task->rw + task->c;
		if (task->rw >= task->p) {		
			task->rw = task->rw - task->p;
			++mu;
		}		
	}
	return mu;
}

// test code for bfair algorithm
// int main() {
// 	const int numTasks = 20;
// 	TaskSet* ts = new TaskSet(numTasks);
// 	ts->set_prob_range();
// 	ts->set_period_range();
// 	ts->gen_task_set();
// 	ts->print_cur_task_set();
// 	const int stop_point = 1000;
// 	Bfair ans(stop_point, ts->get_min_core());
// 	cout<<"result:"<<ans.exec_sche(ts, stop_point, ts->get_min_core() + 10)<<endl;
// 	// ans.print_schedule();

// 	return 0;
// }


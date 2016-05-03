#include "sched.h"

Scheduler::Scheduler(const long long stop_point, const int num_proc, const EnergyModel* em) {
	_num_proc = min(num_proc, (em ? em->get_num_proc() : num_proc));
	_stop_point = stop_point;
	_schedule = new vector<vector<int> >(_num_proc, vector<int>(stop_point, -1));
	_speed_tab = new vector<vector<double> >(_num_proc, vector<double>(stop_point));
	set_offset(0);
	for(int i = 0; i < _num_proc; ++i) {
		fill((*_speed_tab)[i].begin(), (*_speed_tab)[i].end(), (em ? em->get_kdef_sp(i) : 1.0));
	}
}

Scheduler::~Scheduler() {
	if( _schedule != nullptr ) {
		delete _schedule;
	}
	if( _speed_tab != nullptr ) {
		delete _speed_tab;
	}
	_schedule = nullptr;
	_speed_tab = nullptr;
}

/*this method is used for reseting schedule table and speed vector, 
 * in the future if there is more consideration that needed to be applied on scheduler table
 * then please don't forget add corresponding init process here
 * NOTICE: if schedule table and speed vector is as expected, the space will not be rellocated
*/
bool Scheduler::reset_sche_tab(const long long stop_point, const int num_proc, const EnergyModel* em) {
	if(em && em->get_num_proc() < num_proc) return false;

	// here, reset_sche_tab if return true, then "em->get_num_proc() >= num_proc" is garanteed
	if(stop_point != _schedule->front().size() || num_proc != _schedule->size() || stop_point != _speed_tab->front().size() || num_proc != _speed_tab->size()) {
		_num_proc = num_proc;
		_stop_point = stop_point;
		if( _schedule != nullptr ) {
			delete _schedule;
			_schedule = nullptr;
		}
		_schedule = new vector<vector<int> >(_num_proc, vector<int>(_stop_point, -1));

		if( _speed_tab != nullptr ) {
			delete _speed_tab;
			_speed_tab = nullptr;
		}
		_speed_tab = new vector<vector<double> >(num_proc, vector<double>(stop_point));
		for(int i = 0; i < num_proc; ++i) {
			fill((*_speed_tab)[i].begin(), (*_speed_tab)[i].end(), (em ? em->get_kdef_sp(i) : 1.0));
		}
	}else {
		for(int i = 0; i < _num_proc; ++i) {
			fill((*_schedule)[i].begin(), (*_schedule)[i].end(), -1);
		}
		for(int i = 0; i < _num_proc; ++i) {
			fill((*_speed_tab)[i].begin(), (*_speed_tab)[i].end(), (em ? em->get_kdef_sp(i) : 1.0));
		}
	}
	return true;
}

/* the mask here is used for filter all those tasks that is not in the orginal task set
 * in the schdule table, however, here we do not consider speed table, since speed table
 * should be take care of by schduler algorithm and energy model, but for current table,
 * there is no such kind of requirement, therefore, in this version, we do not consider about this
 * so, to make things easiler, if there is any kind of requirement, please overload this function
 * in your own class
*/
void Scheduler::mask_sche_table(const TaskSet *ts_src, const EnergyModel* em, const int offset) {
	vector<vector<int> > &sche = *_schedule;
	vector<vector<double> > &sped = *_speed_tab;
	int upper = ts_src->get_num_tasks();

	for(int i = 0; i < sche.size(); ++i) {
		double d_sp = (em ? em->get_kdef_sp(i) : 1.0);
		for(int j = 0; j < sche[i].size(); ++j ) {
			int tmp = sche[i][j];
			sche[i][j] = ((tmp >= upper || tmp == -1) ? -1 : tmp + offset);
			double tmp_sp = sped[i][j];
			sped[i][j] = (tmp >= upper ? d_sp : tmp_sp);
		}
	}
	return ;
}

void Scheduler::set_offset(int base) {
	_offset = max(base, 0);
	return ;
}

Err_Code_Sche_Tab Scheduler::check_val_sche(const TaskSet *ts, const vector<vector<int> > &schedule, FILE* fp) {
	const int numTasks = ts->get_num_tasks();
	// const vector<vector<int> > &schedule = *_schedule;
	const vector<task_tpl> &tasks = ts->get_task_set();
	const int num_proc = schedule.size();

	// check conflict time slot
	for(int i = 0; i<_stop_point; ++i) {
		for(int j = 0; j<num_proc; ++j) {
			for (int k = 0; k<num_proc; ++k) {
				if ((j!=k) && schedule[k][i] != -1 && (schedule[j][i]==schedule[k][i])) {
					fprintf(fp, "conflict found at time %d for task %d \n", i, schedule[j][i]);
					return Err_Code_Sche_Tab::CONFILT_SLOT;
				}
			}
		}
	}
	// task get exact 'c' units within each of its period 'p'
	for(int i = 0; i < numTasks; ++i) {
		long long index = 0;
		long long t = 0;
		long long end = _stop_point - (_stop_point % tasks[i].p);
		
		while(t < _stop_point ) {
			long long allocate = 0, nextp = t + tasks[i].p;
			if( nextp >= end ) break;
			while(t < nextp) {
				for (int j = 0; j<num_proc; ++j) {
					if (schedule[j][t] == i + _offset) {
						++allocate;
					}
				}
				++t;
			}

			if (allocate < tasks[i].c) {
				fprintf(fp, "task %d does not get enough time for %lld instance at time: %lld-->%lld, allocate %lld\n", i, index, index*tasks[i].p, (index+1) * tasks[i].p, allocate);
				return Err_Code_Sche_Tab::BELOW_ALLOCATED;
			}else if(allocate > tasks[i].c) {
				fprintf(fp, "task %d get too much time for %lld instance at time: %lld-->%lld, allocate %lld\n", i, index, index*tasks[i].p, (index+1)*tasks[i].p, allocate);
				return Err_Code_Sche_Tab::OVER_ALLOCATED;
			}else {}

			++index;
		}
	}
	return Err_Code_Sche_Tab::VALID_SCHE;
}

void Scheduler::print_schedule(FILE* fp) {
	fprintf(fp, "cur schedule table:\n");
		fprintf(fp, "---------------------------------------------------\n");
		for(auto s : *_schedule) {
			for(auto k : s) {
				cout<<k<<" ";
			}
			cout<<endl;
		}
	fprintf(fp, "---------------------------------------------------\n");
	return ;
}












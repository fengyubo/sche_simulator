#include "scheduler_dvfs_bfair.h"
#include <stdio.h>

void dvfs_Bfair::init_sche(const long long stop_point, const int num_proc, EnergyModel *em) {
	_error_code = (int)Err_Code_Sche_Subtle::VALID_SCHE; //clear error code
	if(em && em->get_num_proc() < num_proc) {
		_error_code = (int)Err_Code_Sche_Subtle::INVALID_PROC; //clear error code
	}
	_prev_k = 0;
	_rest_schdule.clear();
	_rest_tasks.clear();
	return ;
}

Err_Code_Sche_Alg dvfs_Bfair::exec_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp, EnergyModel *em) {
	// specially, in dvfs, emergy model should not be nullptr, this is restricted
	if( !ts || !em || num_proc < ts->get_min_core() || (em && em->get_num_proc() < num_proc) || (em && em->get_num_proc() < ts->get_min_core()) ) {
		// here, we have to guarantee that "energy_procs >= num_proc >= ts->get_min_core"
		_error_code = (int)Err_Code_Sche_Subtle::INVALID_PROC;
		return INVALID_PARM;
	}
	_stop_point = min(ts->get_lcm(), stop_point);
	_num_proc = num_proc;
	reset_sche_tab(_stop_point, _num_proc, em);

	double t_uti = ts->get_uti();
	vector<task_tpl> t_ts(ts->get_task_set().begin(), ts->get_task_set().end());
	// first step sort all the tasks
	sort(t_ts.begin(), t_ts.end(), cmp);
	int k = 0;
	while((k < t_ts.size()) && (k < _num_proc) && (t_ts[k].u > t_uti/(double)(_num_proc - k))) {
		// find speed for current processor
		double t_sp = max(em->get_kmin_sp(k), t_ts[k].u);
		// assign speed table
		fill((*_speed_tab)[k].begin(), (*_speed_tab)[k].end(), t_sp);
		// assign schdule table
		fill((*_schedule)[k].begin(), (*_schedule)[k].end(), k);
		t_uti -= t_ts[k].u;
		++k;
	}

	// create a new task set to call b-fair algoirhm
	int rest_procs = _num_proc - k;
	_prev_k = k;
	/* exists two cases:
	 * no tasks left
	 * still some tasks and some proc left
	*/
	if(rest_procs > 0 && t_ts.size() > k) {
		double rest_sp = max(t_uti / (double)rest_procs, em->get_kmin_sp(k)); // set speed for the rest of the processors
		for(int i = k; i < _num_proc; ++i) {
			fill((*_speed_tab)[i].begin(), (*_speed_tab)[i].end(), rest_sp);
		}
		// all the task c will be adjust to new input for schdule table one
		vector<task_tpl> r_ts;
		for(int i = k; i < t_ts.size(); ++i) {
			task_tpl t;
			t.c = (long long)((double)t_ts[i].c / rest_sp * _scale);
			t.p = (long long)((double)t_ts[i].p * _scale);
			t.u = (double)t.c / (double)t.p;
			r_ts.push_back(t);
		}

		TaskSet rest_tasks(r_ts.begin(), r_ts.end(), ts->get_prob_range(), ts->get_period_range(), ts->get_p_dis(), ts->get_c_dis());
		// create a new bfair scheduler 
		rest_tasks.set_lcm(ts->get_lcm());
		_rest_tasks = rest_tasks;
		set_offset(k);

		Bfair ans(_stop_point, rest_procs);
		ans.init_sche(_stop_point, rest_procs);
		ans.set_offset(k);

		if(ans.get_error_code() != (int)Err_Code_Sche_Subtle::VALID_SCHE) {
			fprintf(fp, "error in rest of tasks: not enough processor in energy model for rest of the tasks");
			return INVALID;
		}else {
			// call sub tasks scheduling
			this->_error_code = ans.exec_sche(&rest_tasks, _stop_point, rest_procs, fp, nullptr);
			_rest_schdule = ans.get_schedule();
			for(int i = 0; i < ans.get_num_proc(); ++k, ++i) {
				(*_schedule)[k].assign(_rest_schdule[i].begin(), _rest_schdule[i].end());
			}
			return (this->_error_code == (int)Err_Code_Sche_Subtle::VALID_SCHE ? VALID : INVALID);
		}
	}else {
		if(k < t_ts.size()) {
			fprintf(fp, "not sufficient number of processors");
			return INVALID;
		}else {
			return VALID;
		}
	}
}

Err_Code_Sche_Tab dvfs_Bfair::check_val_sche(const TaskSet *ts, FILE* fp) {
	Err_Code_Sche_Tab ret =  Scheduler::check_val_sche(&_rest_tasks, _rest_schdule, fp);
	return ret;
}

bool dvfs_Bfair::cmp(const task_tpl &a, const task_tpl &b) {
	// notice: this algorithm required tasks will be sorted by decreasing order
	return (a.u > b.u);
}
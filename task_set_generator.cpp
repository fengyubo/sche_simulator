#include "sched.h"
#include <fstream>

TaskSet::TaskSet(string filename) {
	// ifstream ts_f(filename);
	// int n = 0;
	// MathHelper helper;
	// if(ts_f.is_open()) {
	// 	ts_f>>n;
	// 	long long lcm = 1l;
	// 	double ut = 0.0;
	// 	for(int i = 0; i < n; ++i) {
	// 		task_tpl t;
	// 		ts_f>>t.c;
	// 		ts_f>>t.p;
	// 		t.u = (double)t.c/(double)t.p;
	// 		lcm = helper.GetLCM(lcm, t.p);
	// 		ut += t.u;
	// 		_tasks.push_back(t);
	// 	}
	// 	_lcm = lcm;
	// 	_ut = ut;
	// 	_min_cores = ceil(ut);
	// }else {
	// 	fprintf(stderr, "wrong file: cannot open the file %s\n", filename.c_str());
	// }
}

TaskSet::TaskSet(int n, string dis_c, string dis_p) {
	_tasks.resize(n);
	_lcm = 1;
	_ut = 0;
	_min_cores = 0;
	_dis_c = dis_c;
	_dis_p = dis_p;
}

TaskSet::TaskSet(const vector<task_tpl>::iterator outer_task_set_begin, const vector<task_tpl>::iterator outer_task_set_end, const Range_Type &outer_prob_range, const Period_Type &outer_period_range, const string dis_p, const string dis_c) {
	long long lcm = 1l;
	double uti = 0.0;
	MathHelper helper;

	for(auto iter = outer_task_set_begin; iter != outer_task_set_end; iter = next(iter)) {
		lcm = helper.GetLCM(iter->p, lcm);
		uti += iter->u;
		_tasks.push_back(*iter);
	}

	_lcm = lcm;
	_ut = uti;
	_min_cores = ceil(uti);
	set_prob_range(outer_prob_range);
	set_period_range(outer_period_range);
	set_p_dis(dis_p);
	set_c_dis(dis_c);
}

void TaskSet::clear() {
	_tasks.clear();
	_lcm = 1l;
	_ut = 0;
	_min_cores = 0;
}

bool TaskSet::set_c_dis(const string dis) {
	MathHelper m_t;
	auto type_set = m_t.get_dis_type_set();
	if(type_set.find(dis) != type_set.end() ) {
		_dis_c = dis;
		return true;
	}
	return false;
}

bool TaskSet::set_p_dis(const string dis) {
	MathHelper m_t;
	auto type_set = m_t.get_dis_type_set();
	if(type_set.find(dis) != type_set.end() ) {
		_dis_p = dis;
		return true;
	}
	return false;
}

bool TaskSet::set_prob_range(Range_Type p) {
	if( p.first < 0. || p.first > 1. || p.second < 0. || p.second > 1. ) {
		return false;
	}else {
		_cur_prob_range = p;
		return true;
	}
}

bool TaskSet::set_period_range(Period_Type p) {
	if( p.first < 0 || p.second < 0 ) {
		return false;
	}else {
		_cur_peroid_range = p;
		return true;
	}
}

void TaskSet::gen_task_set() {
	long long lcm = 1;

	do {
		lcm = generate_task_set_pi();
	} while (lcm >= 1e16);
	
	generate_task_set_ci();

	double uti = 0.0;
	for(auto k : _tasks) {
		uti += (double)k.c / (double)k.p;
	}

	_lcm = lcm;
	_ut = uti;
	_min_cores = ceil(uti);
	return ;
}

void TaskSet::gen_task_set(double ut) {
	gen_task_set();
	while(ut > 1) ut /= 10;
	// add more cores to make uti as it makes
	_min_cores = ceil(_ut/ut);
}

long long TaskSet::generate_task_set_pi() {
	long long lcm = 1, tmp_lcm = 0, tmp_p = 0;
	MathHelper helper(_dis_p);
	for(int i = 0; i < _tasks.size(); ++i) {
		do{
			tmp_p = helper.gen_rd(_cur_peroid_range.first, _cur_peroid_range.second);
			tmp_lcm = helper.GetLCM(lcm, tmp_p);
		}while( tmp_lcm >= 4e9 );
		lcm = tmp_lcm;

		_tasks[i].p = tmp_p;
	}

	return lcm;
}

void TaskSet::set_lcm(const long long lcm) {
	// no lcm check, please know what you do when call this function
	_lcm = lcm;
}

// padding will not check cur uti is correct or not, instead, it will just padding,
// how many tasks to padding is controled by user
void TaskSet::padding_task(const task_tpl t) {
	MathHelper helper;
	_tasks.push_back(t);
	_ut += t.u;
	_lcm = (t.p != _lcm ? helper.GetLCM(_lcm, t.p) : _lcm);
	_min_cores = ceil(_ut);
	return ;
}

int TaskSet::padding_task_set(const int num_proc) {
	if( num_proc < _min_cores ) return -1;

	long long cur_c = 0, tmp_lcm = _lcm;
	int padding_count = 0;

	for(auto k : _tasks) {
		cur_c += tmp_lcm / k.p * k.c;
	}
	long long remain = (long long)num_proc * tmp_lcm - cur_c;
	while(remain - tmp_lcm >= 0) {
		task_tpl t;
		t.c = tmp_lcm; 
		t.p = tmp_lcm;
		t.u = 1.0;
		padding_task(t);
		++padding_count;
		remain -= tmp_lcm;
	}
	if(remain > 0) {
		task_tpl t;
		t.c = remain;
		t.p = tmp_lcm;
		t.u = (double)num_proc - _ut;
		++padding_count;
		padding_task(t);
	}
	return padding_count;
}

void TaskSet::generate_task_set_ci() {
	/* random generate task set computation time */
	MathHelper helper(_dis_c);
	for(int i = 0; i < _tasks.size(); ++i) {
		_tasks[i].c = (long long) helper.gen_rd( (int)(_cur_prob_range.first*_tasks[i].p)+1, (int) (_cur_prob_range.second*_tasks[i].p) );
		_tasks[i].u = (double) _tasks[i].c / (double) _tasks[i].p;
	}
	return ;
}

void TaskSet::print_cur_task_set(FILE* fp) {
	fprintf(fp, "cur task set:\n");
	fprintf(fp, "---------------------------------------------------\n");
	for(int i = 0; i < _tasks.size(); ++i) {
		cout<<i<<":\t";
		_tasks[i].print(fp);
	}
	fprintf(fp, "---------------------------------------------------\n");
}

void driver() {
	const int numTasks = 20;
	TaskSet* ts = new TaskSet(numTasks);
	ts->set_prob_range();
	ts->set_period_range();
	ts->gen_task_set();
	ts->print_cur_task_set();
	cout<<ts->get_uti()<<endl;
	cout<<ts->get_lcm()<<endl;
	cout<<ts->get_min_core()<<endl;
	cout<<"["<<ts->get_prob_range().first<<","<<ts->get_prob_range().second<<"]"<<endl;
	cout<<"["<<ts->get_period_range().first<<","<<ts->get_period_range().second<<"]"<<endl;
	delete ts;
}


// int main() {
// 	driver();
// 	return 0;
// }
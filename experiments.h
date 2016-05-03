#ifndef __SCHED__SCHED_EXP_H
#define __SCHED__SCHED_EXP_H
#include <iomanip>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <libconfig.h++>
#include "sched_bfair.h"
#include "sched_pfair.h"
#include "scheduler_dvfs_bfair.h"
using namespace std;
using namespace libconfig;

class experiments{
public:
	typedef enum{
		VALID,
		INVALID_FILE,
		INVALID_Attr,
		INVALID_FORAMT,
		INVALID_ASSIGN,
	} Err_Exp;

	experiments(string config_file);
	const vector<vector<double> >& get_vec_speed() const { return vec_speed; }
	void print_config();
	void run();

	void reset_error_code();
	Err_Exp get_error_code() { return error_code; }

private:
	void reg_scheduler();
	Scheduler* alloc_scheduler();
	TaskSet* alloc_taskset();
	EnergyModel* alloc_energymodel();

	const int default_proc = -1;

	Config cfg;
	int _num_tasks;
	int _num_proc;
	long long _stop_point;
	string _dis_c, _dis_p;
	string _sche_name;
	vector<vector<double> > vec_speed;
	Err_Exp error_code;
	pair<long long, long long> _period_range;
	pair<double, double> _prob_range;
	unordered_map<string, int> _schduler_map;
	string _proc_type;
};
#endif
#ifndef __SCHED__SCHED_H
#define __SCHED__SCHED_H
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <unordered_set>
#include <unordered_map>

using namespace std;

typedef enum{
	VALID_SCHE, 
	INVALID_SCHE,
	OVER_ALLOCATED,
	BELOW_ALLOCATED,
	CONFILT_SLOT,
} Err_Code_Sche_Tab; // error code for schedule table result check

typedef enum{
	VALID, // valid algorithm: no error occured
	INVALID, // invalid algorithm: there is error occured, but not parameters error
	INVALID_PARM, // parameter error: invalid input value
	INVALID_PROC_REG, // parameter error: wrong # of processors
} Err_Code_Sche_Alg;

// structure of task
typedef struct task_tpl{
	long long c;	/* the computation required for each period */
	long long p;	/* the period of the task has to be long long instead of unsigned long long */
	double u;		/* the utilization of the task: u = c/p */
	void print(FILE* fp = stdout) { fprintf(fp, "%lld\t%lld\t%lf\n", c, p, u); }
} Task;

/* this macro is used for debug:
 * e.g: __DEBUG__(__DEBUG_START_SIM， "here is message funtion", printf("%d\n", index))
 * this macro could be directly used to print out infos
*/
#define __DEBUG__(_msg, _stat) \
		do{ \
			fprintf(stderr, "/%s/{%s}[%d]: %s\n",  __FILE__, __func__, __LINE__, _msg); \
			_state;\
		} while(false); \

class MathHelper{
public:
	MathHelper(const string dis = "uniform");
	~MathHelper();

	// return a int random number between l and r 
	int gen_rd(int l, int r);

	static unsigned long GetGCD(const long long n1, const long long n2);
	static long long GetLCM(const long long n1, const long long n2);
	
	const string get_dis_type() const { return _dis_type; }
	const unordered_set<string> get_dis_type_set() const { return _dis_set; }

private:
	/* register for distrubtion types
	 * currently we have normal and even distribution
	*/
	void dis_reg();
	int uni_random(int l, int r);
	int norm_random(int l, int r);

	const double lower_b = 0.0, upper_b = 1.0; // uniform setting
	const double norm_mu = 0.5, sqrt_r = 0.19; // normal setting
	default_random_engine *gen;

	/*
	 * Following member varible are designed for different type of random functions
	 * however, all functions have the same input interface: left and right range, so
	 * here is a function pointer to do this
	*/
	string _dis_type;
	int (MathHelper::*gen_rd_dis)(int, int);
	unordered_map<string, int (MathHelper::*)(int, int)> _dis_map;
	unordered_set<string> _dis_set;

	uniform_real_distribution<double> *dis_uni;
	normal_distribution<double> *dis_norm;
};

// structure of task set
class TaskSet {
public: 
	typedef pair<double, double> Range_Type;
	typedef pair<long long, long long> Period_Type;

	// default setting is uniform for both computation time and period
	TaskSet(int n = 0, string dis_c = "unifrom", string dis_p = "unifrom");
	TaskSet(string filename);
	TaskSet(const vector<task_tpl>::iterator outer_task_set_begin, const vector<task_tpl>::iterator outer_task_set_end, const Range_Type &outer_prob_range, const Period_Type &outer_period_range, const string dis_p, const string dis_c);
	
	// compuation time and period distribution setting
	const string get_c_dis() const { return _dis_c; };
	const string get_p_dis() const { return _dis_p; };
	bool set_c_dis(const string dis = "uniform");
	bool set_p_dis(const string dis = "uniform");

	bool set_prob_range(Range_Type p = make_pair(0.001, 0.999));
	const Range_Type get_prob_range() const { return _cur_prob_range; };

	bool set_period_range(Period_Type p = make_pair(10, 100));
	const Period_Type get_period_range() const { return _cur_peroid_range; };

	void gen_task_set();
	void gen_task_set(double ut);
	const vector<task_tpl>& get_task_set() const { return _tasks; };
	void print_cur_task_set(FILE* fp = stdout);
	int padding_task_set(const int num_proc);
	void set_lcm(const long long lcm);
	void clear();

	const long long get_lcm() const { return _lcm; }
	const double get_uti() const { return _ut; }
	const int get_min_core() const { return _min_cores; }
	const int get_num_tasks() const { return _tasks.size(); } // get number of tasks in cur task set

private:
	long long generate_task_set_pi();
	void generate_task_set_ci();
	void padding_task(const task_tpl t);

	vector<task_tpl> _tasks;
	string _dis_c, _dis_p;
	Range_Type _cur_prob_range;
	Period_Type _cur_peroid_range;
	long long _lcm; // lcm
	double _ut; // utilization
	int _min_cores; // # of min processors that this task set required
};

// base class for energy model
class EnergyModel{
public:
	typedef enum {
		VALID_RET,
		INVALD_PARM_UNMATCH,
	} Err_Code_Energy;
	
	const double default_sp = 1.0;

	EnergyModel(int num_proc);
	EnergyModel(const vector<vector<double> > &speed);
	void reset_freq_vect(int num_proc);
	Err_Code_Energy cal_schedule_cost(const TaskSet* ts, const vector<vector<int> > &sche_tab, const vector<vector<double> > &speed_tab);

	const vector<vector<double> > get_proc_freq() const { return _proc_freq; }
	const long long get_free_slots() const { return _c_free_slots; }
	const long long get_cost_migration() const { return _c_mig; }
	const long long get_cost_switch() const { return _c_swt; }
	const long long get_cost_energy() const { return _c_energy; }
	const int get_num_proc() const { return  _num_proc; }
	// for universal usage
	const double get_kmin_sp(const int index) const { return _proc_freq[index].front(); }
	const double get_kmax_sp(const int index) const { return _proc_freq[index].back(); }
	const string get_proc_type() const { return _proc_type; }
	void set_proc_type(string str);
	virtual const double get_kdef_sp(const int index) const { return default_sp; }	// const double get_default_sp() const { return default_sp; }
	void print_energymodel();

private:
	double _default_min_freq, _default_max_freq;
	vector<vector<double> > _proc_freq;
	string _proc_type;
	long long _c_free_slots;
	long long _c_mig; // count of migration
	long long _c_swt; // count of context switch
	double _c_energy; // energy consuming
	int _num_proc;
};

// base class for simulators
class Scheduler{
public:
	typedef enum{
		VALID_SCHE, // valid algorithm: no error occured
		INVALID_SCHE, // invalid algorithm: there is error occured, but not parameters error
		INVALID_PROC,
		ERR_UNEXPECTED,
	} Err_Code_Sche_Subtle;

	Scheduler(const long long stop_point, const int num_proc, const EnergyModel* em = nullptr);
	~Scheduler();

	virtual void init_sche(const long long stop_point, const int num_proc, EnergyModel *em = nullptr) {
		_error_code = (int)Err_Code_Sche_Subtle::VALID_SCHE; //clear error code
		if(em && em->get_num_proc() < num_proc) {
			_error_code = (int)Err_Code_Sche_Subtle::INVALID_PROC; //clear error code
		}
		return ;
	}

	/* all schduler algorithm should inheirt from this virtual method, 
	 * the major duty of this method is not provided all stuff that any kind of algorihtm should follow,
	 * but a kind of frame work: in cur frame work, enenrgy consumtion is not considered; if energy is needed, by overloading the
	 * exec_sche method, and do energy stuff within your own exec_sche method.
	*/
	virtual Err_Code_Sche_Alg exec_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp = stderr, EnergyModel *em = nullptr) {
		if( !ts || num_proc < ts->get_min_core() || (em && em->get_num_proc() < num_proc) || (em && em->get_num_proc() < ts->get_min_core()) ) {
			// here, we have to guarantee that "energy_procs >= num_proc >= ts->get_min_core"
			_error_code = (int)Err_Code_Sche_Subtle::INVALID_PROC;
			return INVALID_PARM;
		}

		// padding the task set to call exec_full_util_sche
		TaskSet t_ts = *ts;
		if(t_ts.padding_task_set(num_proc) < 0) {
			_error_code = (int)Err_Code_Sche_Subtle::ERR_UNEXPECTED;
			return INVALID_PARM;
		}

		// set stop point and number of processors as properity of current schduler
		_stop_point = min(ts->get_lcm(), stop_point);
		_num_proc = num_proc;
		
		// actually, here should be some if statement like "if(em && em->get_num_proc() >= num_proc)"
		// to check if input is valid and adjust member variables
		// however, since we did check it in the begining, so there is no necessary to do that.
		reset_sche_tab(_stop_point, _num_proc, em);

		// calling exec_full_util_sche
		this->_error_code = this->exec_full_util_sche(&t_ts, _stop_point, _num_proc, fp, em);

		mask_sche_table(ts, em, _offset);
		// if there is any error occured, check error code which stores the error reason
		return (this->_error_code == (int)Err_Code_Sche_Subtle::VALID_SCHE ? VALID : INVALID);
	}

	virtual Err_Code_Sche_Tab check_val_sche(const TaskSet *ts, FILE* fp = stderr) {
		const vector<vector<int> > &schedule = *_schedule;
		return check_val_sche(ts, schedule, fp);
	}

	Err_Code_Sche_Tab check_val_sche(const TaskSet *ts, const vector<vector<int> > &schedule, FILE* fp = stderr);
	void print_schedule(FILE* fp = stdout);

	const vector<vector<int> >& get_schedule() { return *_schedule; };
	const vector<vector<double> >& get_speed_vec() { return *_speed_tab; };
	const int get_stop_point() { return _stop_point; }
	const int get_num_proc() { return _num_proc; }
	const int get_error_code() { return _error_code; }
	void set_offset(int base = 0);

protected:
	// reset the schdule table considering the stop_point and num_procs
	bool reset_sche_tab(const long long stop_point, const int num_proc, const EnergyModel* em = nullptr);

	// default energy model: if there is no energy model, then speed will default set to min(0.0, when no tasks)
	// or max(1.0, when this slot is assigned to some tasks)
	void mask_sche_table(const TaskSet *ts_src, const EnergyModel* em = nullptr, const int offset = 0);
	virtual int exec_full_util_sche(const TaskSet *ts, long long stop_point, const int num_proc, FILE* fp = stderr, EnergyModel *em = nullptr) { return (int)Err_Code_Sche_Subtle::VALID_SCHE; }

	vector<vector<int> >* _schedule;
	vector<vector<double> >* _speed_tab;
	long long _stop_point;
	int _num_proc;
	int _error_code;
	int _offset;
};
#endif
#include "sched.h"
#include "experiments.h"

experiments::experiments(string config_file) {
	reg_scheduler();
	reset_error_code();
	try{
		// open and parse configuration file 
    	cfg.readFile(config_file.c_str());
    	
    	// task set config load
    	// number of tasks
    	_num_tasks = cfg.lookup("experiment.task_set.num_tasks");
    	// distribution for computation time
   		_dis_c = (const char*)cfg.lookup("experiment.task_set.dis_c");
   		// distribution for period time
   		_dis_p = (const char*)cfg.lookup("experiment.task_set.dis_p");
   		
   		_sche_name = (const char*)cfg.lookup("experiment.scheduler.sche_name");
   		_stop_point = (int)cfg.lookup("experiment.scheduler.stop_point");

   		_period_range.first = (int)cfg.lookup("experiment.task_set.period_range.lower_period");
   		_period_range.second = (int)cfg.lookup("experiment.task_set.period_range.upper_period");

   		_prob_range.first = (double)cfg.lookup("experiment.task_set.prob_range.lower_pc");
   		_prob_range.second = (double)cfg.lookup("experiment.task_set.prob_range.upper_pc");

   		_proc_type = (const char*)cfg.lookup("experiment.energy_model.proc_type");
   		
   		try{
   			_num_proc = (int)cfg.lookup("experiment.scheduler.num_proc");
   		}catch(const SettingNotFoundException &nfex_sp) {
   			cerr<<"using default setting for # of processors (aks. min # of procs for task set)"<<endl;
   			_num_proc = default_proc;
   		}

   		// enery model task set
   		try{
   			// speed vector configuration
	   		const Setting& proc_setting = cfg.lookup("experiment.energy_model.proc_speeds");
	   		int len = proc_setting.getLength();
	   		for(int i = 0; i < len; ++i) {
	   			const Setting &cur = proc_setting[i];
	   			vector<double> tmp_vec;
	   			int cur_len = cur.getLength();
	   			for(int j = 0; j < cur_len; ++j) {
	   				tmp_vec.push_back((double)cur[j]);
	   			}
	   			vec_speed.push_back(tmp_vec);
	   		}
   		}catch(const SettingNotFoundException &nfex_sp) {
   			cerr<<"using default setting for speed vector"<<endl;
   		}
    }catch(const FileIOException &fioex) {
    	error_code = INVALID_FILE;
    	cerr << "I/O error while reading file." << endl;
    }catch(const SettingNotFoundException &nfex) {
    	error_code = INVALID_Attr;
    	cerr << "Attributes missing" << endl;
    }catch(const ParseException &pex) {
    	error_code = INVALID_FORAMT;
    	cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << endl;
  	}catch(const SettingTypeException &tex) {
  		error_code = INVALID_ASSIGN;
  		cerr << "assign error"<<endl;
  	}
  	return ;
}

// scheduler register vector, used for register all kind of schduler algorithm
void experiments::reg_scheduler() {
	_schduler_map["Bfair"] = 0;
	_schduler_map["Pfair"] = 1;
	_schduler_map["Bfair_dvfs"] = 2;
	return ;
}

void experiments::reset_error_code() {
	error_code = VALID;
	return ;
}

void experiments::run() {
	/*
	 * check if config is correct, otherwise abort
	*/ 
	if(get_error_code() != VALID) {
		cerr<<"error in configuration file, job abort"<<endl;
		return ;
	}

	/*
	 * init part, used for init 3 major part of the simulator
	*/ 
	TaskSet* ts = alloc_taskset();
	if(!ts) {
		cerr<<"error in ts alloc"<<endl;
		return ;
	}

	/* 
	 * here, do not check anything about if energy model could hanle current task set requst,
	 * I think this is not the duty of experiments class, intead, just pass the paramater to the other module
	*/
	_num_proc = (_num_proc == default_proc ? ts->get_min_core() : _num_proc);
	Scheduler* sche = alloc_scheduler();
	if(!sche) {
		cerr<<"error in sche alloc"<<endl;
		delete ts;
		return ;
	}

	EnergyModel* em = alloc_energymodel();

	/*
	 * main part of experiments
	 * init sche and exec
	*/ 
	sche->init_sche(_stop_point, _num_proc, em);
	Err_Code_Sche_Alg ret_exec = sche->exec_sche(ts, _stop_point, _num_proc, stderr, em);
	if(ret_exec != VALID) {
		cerr<<"error code for exec: "<<ret_exec<<endl;
	}else {
		// sche->print_schedule();
		Err_Code_Sche_Tab ret_check = sche->check_val_sche(ts);
		if(ret_check != VALID_SCHE) {
			cout<<"error code for sche tab: "<<ret_check<<endl;
		}else {
			// result report
			EnergyModel::Err_Code_Energy ret_em = em->cal_schedule_cost(ts, sche->get_schedule(), sche->get_speed_vec());
			if(ret_em != EnergyModel::VALID_RET) {
				cout<<"error code for sche tab: "<<ret_check<<endl;
			}else {
				cout<<endl;
				cout<<"free slots:"<<em->get_free_slots()<<endl;
				cout<<"# of migration:"<<em->get_cost_migration()<<endl;
				cout<<"# of switch:"<<em->get_cost_switch()<<endl;
				cout<<"energy cost:"<<em->get_cost_energy()<<endl;
			}
		}
	}

	if(ts) {
		delete ts;
	}

	if(sche) {
		delete sche;
	}
	if(em) {
		delete em;
	}

	return ;
} 


/*
 * some helper methods to help with allocation
 * this is for convinences
*/
TaskSet* experiments::alloc_taskset() {
	TaskSet* ts = new TaskSet(_num_tasks);
	if(ts) {
		ts->set_prob_range(_prob_range);
		ts->set_period_range(_period_range);
		ts->set_c_dis(_dis_c);
		ts->set_p_dis(_dis_p);
		ts->gen_task_set();	
		// ts->print_cur_task_set();	
	}else {
		ts = nullptr;
	}
	return ts;
}

EnergyModel* experiments::alloc_energymodel() {
	EnergyModel* em = nullptr;
	// if there is no speed config given, then default way will be used: <min = 0.0, max = 1.0>
	// otherwise, the speed vector will be used
	if(vec_speed.empty()) {
		em = new EnergyModel(_num_proc);
		em->set_proc_type(_proc_type);
	}else{
		em = new EnergyModel(vec_speed);
		em->set_proc_type(_proc_type);
	}
	return em;
}

Scheduler* experiments::alloc_scheduler() {
	if(_schduler_map.find(_sche_name) == _schduler_map.end()) return nullptr;
	Scheduler* sche = nullptr;
	switch(_schduler_map[_sche_name]) {
		case 0: 
			sche = new Bfair(_stop_point, _num_proc);
			break;
		case 1:
			sche = new Pfair(_stop_point, _num_proc);
			break;
		case 2:
			sche = new dvfs_Bfair(_stop_point, _num_proc);
			break;
		default:
			break;
	}
	return sche;
}

void experiments::print_config() {
	/*
	 * print current experiment setting
	*/
	cout<<"Task Set:"<<endl;
	cout<<"# of tasks:\t\t"<<_num_tasks<<endl;
	cout<<"Ci dis:\t\t\t"<<_dis_c<<endl;
	cout<<"Pi dis:\t\t\t"<<_dis_p<<endl;
	cout<<"P range:\t\t"<<"<"<<_period_range.first<<","<<_period_range.second<<">"<<endl;
	cout<<"Prob range:\t\t"<<"<"<<_prob_range.first<<","<<_prob_range.second<<">"<<endl;
	cout<<endl;

	cout<<"Schduler:"<<endl;
	cout<<"algoirthm:\t\t"<<_sche_name<<endl;
	cout<<"stop_point:\t\t"<<_stop_point<<endl;
	cout<<"# of procs:\t\t"<<_num_proc<<endl;
	cout<<endl;

	cout<<"Energy Model:\t\t"<<endl;
	cout<<"proc_type:\t\t"<<_proc_type<<endl;
	cout<<"---------------------------------------"<<endl;
	if(vec_speed.empty()) {
		cout<<"using default setting of energy model, please see manual about this"<<endl;
	}else {
		for(auto &v : vec_speed) {
			cout<<"[";
			cout<<v.front();
			for(int i = 1; i < v.size(); ++i) {
				cout<<", "<<v[i];
			}
			cout<<"]"<<endl;
		}
	}
	cout<<"---------------------------------------"<<endl;
	return ;
}
#include "sched.h"

EnergyModel::EnergyModel(int num_proc) {
	_default_min_freq = 0.0;
	_default_max_freq = 1.0;
	
	_proc_freq.resize(num_proc);
	_num_proc = num_proc;
	for(int i = 0; i < num_proc; ++i) {
		_proc_freq[i].push_back(_default_min_freq);// front is min
		_proc_freq[i].push_back(_default_max_freq);// back is max
	}
}

void EnergyModel::set_proc_type(string str) {
	_proc_type = str;
	return ;
}

EnergyModel::EnergyModel(const vector<vector<double> > &speed) {
	_proc_freq = speed;
	for(auto &p : _proc_freq) {
		sort(p.begin(), p.end());
		_default_min_freq = max(p.front(), _default_min_freq);
		_default_max_freq = min(p.back(), _default_max_freq);
	}
	_num_proc = _proc_freq.size();
}

void EnergyModel::reset_freq_vect(int num_proc) {
	_proc_freq.clear();
	_proc_freq.resize(num_proc);
	_num_proc = num_proc;
	for(int i = 0; i < num_proc; ++i) {
		_proc_freq[i].push_back(_default_min_freq);
		_proc_freq[i].push_back(_default_max_freq);
	}
	return ;
}

EnergyModel::Err_Code_Energy EnergyModel::cal_schedule_cost(const TaskSet* ts, const vector<vector<int> > &sche_tab, const vector<vector<double> > &speed_tab) {
	if(ts == nullptr && sche_tab.size() != speed_tab.size() && sche_tab.front().size() != speed_tab.front().size()) return EnergyModel::Err_Code_Energy::INVALD_PARM_UNMATCH;
	long long c_free_slots = 0l, c_mig = 0l, c_swt = 0l;
	double c_energy = 0.0;

	for(int i = 0; i < speed_tab.size(); ++i) {
		for(int j = 0; j < speed_tab.front().size(); ++j) {
			c_energy += speed_tab[i][j] * speed_tab[i][j];
			c_free_slots += (sche_tab[i][j] == -1 ? 1 : 0);
		}
	}

	// add idle energy for the rest of the processors
	for(int i = speed_tab.size(); i < _proc_freq.size(); ++i ) {
		c_energy += _proc_freq[i].front() * _proc_freq[i].front() * speed_tab.front().size();
	}

	for(int i = 0; i < sche_tab.size(); ++i) {
		for(int j = 0; j < sche_tab.front().size() - 1; ++j) {
			int cur = sche_tab[i][j], next = sche_tab[i][j+1];
			c_swt += ((cur != next && cur != -1 && next != -1 ) ? 1 : 0);
		}
	}
	
	vector<int> mig_tab(ts->get_num_tasks(), -1);
	for(int j = 0; j < sche_tab.front().size(); ++j) {
		for(int i = 0; i < sche_tab.size(); ++i) {
			int kt = sche_tab[i][j];
			if(kt >= 0) {
				c_mig += ((mig_tab[kt] >= 0 && mig_tab[kt] == i) ? 0 : 1);
				mig_tab[kt] = i;
			}
		}
	}

	_c_free_slots = c_free_slots;
	_c_energy = c_energy;
	_c_mig = c_mig;
	_c_swt = c_swt;

	return EnergyModel::Err_Code_Energy::VALID_RET;
}

void EnergyModel::print_energymodel() {
	for(auto &k : _proc_freq) {
		for(auto &p : k) {
			cout<<p<<",";
		}
		cout<<endl;
	}
	return ;
}







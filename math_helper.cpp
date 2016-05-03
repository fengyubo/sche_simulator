#include "sched.h"

MathHelper::MathHelper(const string dis) {
	dis_reg();
	gen = new default_random_engine(time(nullptr));
	// reg
	dis_uni = new uniform_real_distribution<double>(lower_b, upper_b);
	dis_norm = new normal_distribution<double>(norm_mu, sqrt_r);
	// default setting is uniform : even if input is wrong type
	_dis_type = (_dis_map.find(dis) != _dis_map.end() ? dis : "uniform");
	gen_rd_dis = _dis_map[_dis_type];
}

MathHelper::~MathHelper() {
	if(gen) delete gen;
	if(dis_uni) delete dis_uni;
	if(dis_norm) delete dis_norm;
	gen_rd_dis = nullptr;
}

int MathHelper::gen_rd(int l, int r) {
	return (this->*gen_rd_dis)(l, r);
}

int MathHelper::uni_random(int l, int r) {
    return ( l + (int)((r-l+1) * (*dis_uni)(*gen)) );
}

int MathHelper::norm_random(int l, int r) {
    return ( l + (int)((r-l+1) * (*dis_norm)(*gen)) );
}

long long MathHelper::GetLCM(const long long n1, const long long n2) {
	return ( n1 / GetGCD(n1, n2) * n2);
}

unsigned long MathHelper::GetGCD(const long long n1, const long long n2) {
	long long a = n1, b = n2;
	while(b) {
		long long t = b;
		b = a % b;
		a = t;
	}
	return a;
}

void MathHelper::dis_reg() {
	_dis_map["uniform"] = &MathHelper::uni_random;
	_dis_map["normal"] = &MathHelper::norm_random;

	_dis_set.insert("uniform");
	_dis_set.insert("normal");
}

// int main() {
// 	MathHelper uni_ans("uniform");
// 	MathHelper nom_ans("normal");
// 	vector<int> count(20, 0);
// 	const int times = 10000;
// 	const int base = 100;
// 	for(int i = 0; i < times; ++i) {
// 		int t = uni_ans.gen_rd(0,20);
// 		if(t>=0 && t < 20) ++count[t];
// 	}
// 	for(auto k : count) {
// 		int tmp = k/base;
// 		for(int i = 0; i < tmp; ++i) {
// 			cout<<"*";
// 		}
// 		cout<<endl;
// 	}
// 	cout<<"Normal:"<<endl;
// 	fill(count.begin(), count.end(), 0);
// 	for(int i = 0; i < times; ++i) {
// 		int t = nom_ans.gen_rd(0,20);
// 		if(t>=0 && t < 20) ++count[t];
// 	}
// 	for(auto k : count) {
// 		int tmp = k/base;
// 		for(int i = 0; i < tmp; ++i) {
// 			cout<<"*";
// 		}
// 		cout<<endl;
// 	}
// 	return 0;
// }

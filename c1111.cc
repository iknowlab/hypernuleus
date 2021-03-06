#include <cstdio>
#include <random>
#include <vector>
/* powered by T.Matsuo*/

using namespace std;

double f (double th) {
    return exp(-100*(1-cos(th)));
}


int main()
{
    double th_lim = 3.14/2;
    double eps = 0.0001;


    ////////////////
    // 乱数初期化
    std::mt19937 gen;


    ////////////////
    // 分布生成
    std::vector<double> i;
    std::vector<double> v;

    for(double x=-th_lim; x<th_lim; x+= eps) {
	i.push_back(x);
	v.push_back(f(x));
    }

    std::piecewise_linear_distribution<> dist(i.begin(), i.end(), v.begin());


    ////////////////
    // 乱数発生
    for(int j=0; j<1000000; j++) {
	printf("%g\n", dist(gen));
    }

    return 0;
}

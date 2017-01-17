#ifndef PR_SOLUTION_LEO
#define PR_SOLUTION_LEO

#include"stdafx.h"
using namespace std;
typedef unsigned char uchar;
class PRSolutionLEO: public PRSolution2
{
public:
    PRSolutionLEO() : PRSolution2()
    {};
	void  selectObservable(
		const Rinex3ObsData &rod,
		int iL1Code,
		int iL2Code,
		int iL1SNR,
		vector<SatID> & PRNs,
		vector<double> &L1PRs,
		vector<uchar> & SNRs
	);

    void prepare(
		const CommonTime &t,
		const vector<SatID> &IDs,
		const vector<double> &PRs,
		const XvtStore<SatID>& Eph,
		const IonoModelStore &iono,
		const double &elm,
		const double &Conv,
		Position PosRX,
		vector<bool> &UsedSat,
		Matrix<double> &SVP
	);

	int ajustParameters(
		const Matrix<double> &SVP,
		Vector<bool> &Use,
		double &Conv,
		int &nIter,
		Vector<double>& Sol,
		Matrix<double>& Cov,
		Vector<double>& Resid
	
	);


    uchar maskSNR;
    double maskEl;
    
};

#endif // !PR_SOLUTION_LEO
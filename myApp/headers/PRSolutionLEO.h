#ifndef PR_SOLUTION_LEO
#define PR_SOLUTION_LEO

#include"PRSolutionLEO.h"
using namespace std;
typedef unsigned char uchar;
class PRSolutionLEO: public PRSolution2
{
public:
    PRSolutionLEO() : PRSolution2(),
        maskEl(0.0), maskSNR(0.0), Sol(4), Conv(DBL_MAX), maxIter(10), ps()
    {};
	void  selectObservable(
		const Rinex3ObsData &rod,
		int iL1Code,
		int iL2Code,
		int iL1SNR,
		vector<SatID> & PRNs,
		vector<double> &L1PRs,
		vector<uchar> & SNRs,
       bool isApplyRCO
	);

    void prepare(
        const CommonTime &t,
        vector<SatID> &IDs,
        const vector<double> &PRs,
        const XvtStore<SatID>& Eph,
        const IonoModelStore &iono,
        vector<bool> &UsedSat,
        Matrix<double> &SVP
	);

    int ajustParameters(
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &Use,
        Matrix<double>& Cov,
        Vector<double>& Resid,
        IonoModelStore &iono,
        int &iter,
        bool isApplyIono
	
	);

    uchar maskSNR;
    double maskEl;
    double Conv;
    int maxIter;
    Vector< double> Sol;
    PowerSum ps;

    static double eps;
    
};

#endif // !PR_SOLUTION_LEO
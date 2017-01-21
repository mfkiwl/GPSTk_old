#ifndef PR_SOLUTION_LEO
#define PR_SOLUTION_LEO

#include"PRSolutionLEO.h"
using namespace std;
typedef unsigned char uchar;
class PRSolutionLEO: public PRSolution2
{
public:
    PRSolutionLEO() : PRSolution2(),
        maskEl(0.0), maskSNR(0.0), Sol(4), maxIter(10), ps()
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
        vector<bool> &useSat,
        Matrix<double> &SVP
	);

    int ajustParameters(
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &useSat,
        Matrix<double>& Cov,
        Vector<double>& Resid,
        IonoModelStore &iono,
        bool isApplyIono
	
	);
    
    string printSolution(const vector<bool> &UseSat);

    uchar maskSNR;
    double maskEl;
    int maxIter;
    int iter;
    Vector< double> Sol;
    PowerSum ps;
   
    double sigma;
    double RMS3D;
    double PDOP;

    ofstream dbg;

    static double eps;
protected :
    void calcStat(Vector<double> resid, Matrix<double> Cov);

    int PRSolutionLEO::recalc(
        int i_ex,
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &useSat,
        IonoModelStore &iono,
        bool isApplyIono);
};

#endif // !PR_SOLUTION_LEO
#ifndef PR_SOLVER_LEO
#define PR_SOLVER_LEO

#include"stdafx.h"

using namespace std;
typedef unsigned char uchar;
enum PRIonoCorrType { NONE = 0, Klobuchar, IF };
class PRSolverLEO
{
public:
    PRSolverLEO() :
        maskEl(0.0), maskSNR(0.0), Sol(4), maxIter(15), 
        sigmaMax(50), ionoType(IF), ps()
    {};
    virtual ~PRSolverLEO()
    {};

    void  selectObservations(
        const Rinex3ObsData &rod,
        int iL1Code,
        int iL2Code,
        int iL1SNR,
        vector<SatID> & PRNs,
        vector<double> &L1PRs,
        vector<uchar> & SNRs,
        bool isApplyRCO = false
	);

    void prepare(
        const CommonTime &t,
        vector<SatID> &IDs,
        const vector<double> &PRs,
        const XvtStore<SatID>& Eph,
        vector<bool> &useSat,
        Matrix<double> &SVP
	);

    int solve(
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &useSat,
        Matrix<double>& Cov,
        Vector<double>& Resid,
        IonoModelStore &iono
	);
    
    string printSolution(const vector<bool> &UseSat);
    
    PRIonoCorrType ionoType;

    uchar maskSNR;
    double maskEl;

    int maxIter;
    int iter;

    Vector< double> Sol;
    PowerSum ps;
   
    double sigma;
    double RMS3D;
    double PDOP;
    double sigmaMax;

    ofstream dbg;

    static double eps;

protected :
    void calcStat(Vector<double> resid, Matrix<double> Cov);

    int PRSolverLEO::catchSatByResid(
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &useSat,
        IonoModelStore &iono );

};

#endif // !PR_SOLVER_LEO
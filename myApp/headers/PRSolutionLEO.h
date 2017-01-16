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

    void Prepare(const CommonTime &t,
        const Vector<SatID> &IDs,
        const Vector<double> &PRs,
        const Vector<char> SNR,
        const XvtStore<SatID>& Eph,
        double Conv,
        Triple Pos,
        Vector<bool> &UsedSat,
        Matrix<double> &SVP);

    void ajustParameters();


    uchar maskSNR;
    uchar maskEl;
    
};

#endif // !PR_SOLUTION_LEO
#ifndef PR_SOLVER
#define PR_SOLVER

#include"stdafx.h"

using namespace std;
class PRSolver: public PRSolverBase
{
public:

    PRSolver(TropModel &tropo):PRSolverBase(), tropo(&tropo)
    {};
    virtual ~PRSolver()
    {
    };
    TropModel *tropo ;

    string virtual getName() override
    {
        return "PRSolver";
    };

    virtual int  solve(
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &useSat,
        Matrix<double>& Cov,
        Vector<double>& Resid,
        IonoModelStore &iono
    ) override;

protected:

    virtual int catchSatByResid(
        const CommonTime &t,
        const Matrix<double> &SVP,
        vector<bool> &useSat,
        IonoModelStore &iono) override;



private:

};



#endif // !PR_SOLVER
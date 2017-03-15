#ifndef PR_SOLVER_LEO
#define PR_SOLVER_LEO

#include"stdafx.h"
#include"PRSolverBase.h"
using namespace gpstk;
namespace POD
{

    class PRSolverLEO : public PRSolverBase
    {
    public:
        PRSolverLEO() : PRSolverBase()
        {};
        virtual ~PRSolverLEO()
        {};
        string virtual getName() override
        {
            return "PRSolverLEO";
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

    };
}
#endif // !PR_SOLVER_LEO
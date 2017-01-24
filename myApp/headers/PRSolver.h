#ifndef PR_SOLVER
#define PR_SOLVER

#include"stdafx.h"

using namespace std;
class PRSolver:PRSolverLEO
{
public:
    PRSolver(TropModel &tropo):PRSolverLEO(), tropo(&tropo)
    {};
    virtual ~PRSolver()
    {
    };
    TropModel *tropo ;
    int solve(

    );


private:

};



#endif // !PR_SOLVER
#include"PODSolution.h"
using namespace gpstk;
namespace POD
{
    PODSolution::PODSolution(ConfDataReader & confReader):
        PPPSolutionBase (confReader)
    {
        solverPR = new PRSolverLEO();
    }
}
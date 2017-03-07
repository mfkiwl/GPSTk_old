#include "..\headers\PPPSolution.h"
namespace POD
{
    PPPSolution::PPPSolution(ConfDataReader & confReader) 
        :PPPSolutionBase(confReader)
    {
        solverPR = new PRSolverLEO();
    }

}

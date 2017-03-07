#ifndef POD_SOLUTION
#define POD_SOLUTION

#include "stdafx.h"
using namespace gpstk;
namespace POD
{
    class PODSolution : public PPPSolutionBase
    {
    public:
        PODSolution(ConfDataReader & confReader);
    };
}


#endif // !1
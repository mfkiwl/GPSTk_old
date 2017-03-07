#ifndef PPP_SOLUTION
#define PPP_SOLUTION

#include "stdafx.h"
using namespace gpstk;
namespace POD
{
    class PPPSolution : public PPPSolutionBase
    {
    public:
        PPPSolution(ConfDataReader & confReader);
    };
}

#endif // !1
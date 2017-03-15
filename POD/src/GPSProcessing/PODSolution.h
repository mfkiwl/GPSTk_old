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
        virtual ~PODSolution(){};

    protected:
        virtual void PRProcess() override;
        virtual bool PPPprocess() override;
    };
}


#endif // !1
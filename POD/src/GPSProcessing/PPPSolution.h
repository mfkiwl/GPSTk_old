#ifndef POD_PPP_SOLUTION_H
#define POD_PPP_SOLUTION_H

#include "stdafx.h"
using namespace gpstk;
namespace POD
{
    class PPPSolution : public PPPSolutionBase
    {
    public:
        PPPSolution(ConfDataReader & confReader);
        virtual ~PPPSolution(){};

    protected:
        virtual void PRProcess() override;
        virtual bool PPPprocess() override;
        NeillTropModel tropModel;

    };
}

#endif // !POD_PPP_SOLUTION_H
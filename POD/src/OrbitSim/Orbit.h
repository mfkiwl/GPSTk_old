#ifndef ORBIT_H
#define ORBIT_H
#include"stdafx.h"
#include"ForceModelList.hpp"
#include"EquationOfMotion.hpp"

using namespace gpstk;
namespace POD
{
    class Orbit : public EquationOfMotion
    {
    protected:
        EarthRotation *eop;
        ForceModelList fList;

    public:

        virtual Vector<double> getDerivatives(const double& t, const Vector<double>& y) override;

    private:
        CommonTime refT;

    };
}
#endif // !ORBIT_H

#ifndef POD_ORBIT_MODEL_H
#define POD_ORBIT_MODEL_H
#include"stdafx.h"
#include"ForceModelList.hpp"
#include"EquationOfMotion.hpp"

using namespace gpstk;
namespace POD
{
    class OrbitModel : public EquationOfMotion
    {
    protected:
        EarthRotation *eop;

		ForceModelList fList;
		
		CommonTime refT;

    public:
        virtual Vector<double> getDerivatives(const double& t, const Vector<double>& y) override;



    };
}
#endif // !POD_ORBIT_MODEL_H

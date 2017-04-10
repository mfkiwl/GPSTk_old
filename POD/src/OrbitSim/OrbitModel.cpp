#include"OrbitModel.h"

#include"GravityModel.h"
#include"CentralMassModel.h"
#include"SphericalHarmonicsModel.h"
#include"ForceList.h"

namespace POD
{

    OrbitModel::OrbitModel(const ForceModelData& fmc)
    {
        reset(fmc);
    }

    Vector<double> OrbitModel::getDerivatives(const double& t, const Vector<double>& y)
    {
        // import the state vector to sc
        sc.setStateVector(y);

        CommonTime tf = t0;
        tf += t;
        return forceList.getDerivatives(tf, sc);
    }

    OrbitModel& OrbitModel::reset(const ForceModelData& fmc)
    {
        this->forceList.clear();

        if (!fmc.gData.isModelLoaded)
        {
            InvalidParameter e("Gravity model doesn't loaded");
            GPSTK_THROW(e);
        }

        // GeoEarth
        if (fmc.gData.desiredDegree<2)
            this->forceList.addForce(ForceUniquePtr(new  CentralMassModel(fmc.gData)));
        else
            this->forceList.addForce(ForceUniquePtr(new  SphericalHarmonicsModel(fmc.gData)));

        ////sun gravity
        //if (fmc.useGravSun)
        //    this->forceList.addForce(new SunForce());

        ////moon gravity
        //if (fmc.useGravSun)
        //    this->forceList.addForce(new MoonForce());

        ////relativity effects
        //if (fmc.useRelEffect)
        //    this->forceList.addForce(new RelativityEffect());

        return(*this);

    }  // End of method 'OrbitModel::reset(const ForceModelData& fmc)'
}
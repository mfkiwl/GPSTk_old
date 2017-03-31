#include"OrbitModel.h"

#include "JGM3GravityModel.hpp"
#include "EGM96GravityModel.hpp"
#include "HarrisPriesterDrag.hpp"
#include "Msise00Drag.hpp"
#include "CiraExponentialDrag.hpp"

namespace POD
{
    Vector<double> OrbitModel::getDerivatives(const double& t, const Vector<double>& y)
    {
        if (fmlPrepared == false)
        {
            createFMObjects(forceConfig);
        }

        // import the state vector to sc
        sc.setStateVector(y);

        CommonTime tf = t0;
        tf += t;
        return forceList.getDerivatives(tf, earthBody, sc);
    }

    void OrbitModel::init()
    {
        setSpacecraftData("sc-test01", 1000.0, 20.0, 20.0, 1.0, 2.2);

        enableGeopotential( 1, 1, false, false, false);
        enableThirdBodyPerturbation(false, false);
        enableAtmosphericDrag(AM_HarrisPriester, false);
        enableSolarRadiationPressure(false);
        enableRelativeEffect(false);

    }  // End of method 'OrbitModel::init()'


    OrbitModel& OrbitModel::setSpacecraftData(std::string name,
        const double& mass,
        const double& area,
        const double& areaSRP,
        const double& Cr,
        const double& Cd)
    {
        sc.setName(name);
        sc.setDryMass(mass);
        sc.setDragArea(area);
        sc.setSRPArea(areaSRP);
        sc.setDragCoeff(Cd);
        sc.setReflectCoeff(Cr);

        return (*this);

    }  // End of method 'OrbitModel::setSpacecraftData()'

    OrbitModel& OrbitModel::setSpaceData(double dayF107,
        double aveF107,
        double dayKp)
    {
        forceConfig.dailyF107 = dayF107;
        forceConfig.averageF107 = aveF107;
        forceConfig.dailyKp = dayKp;

        return (*this);
    }

    OrbitModel& OrbitModel::enableGeopotential(
        const int& maxDegree,
        const int& maxOrder,
        const bool& solidTide,
        const bool& oceanTide,
        const bool& poleTide)
    {
        // DONOT allow to change the configuration 
        if (fmlPrepared) return (*this);

        forceConfig.geoEarth = true;


        forceConfig.grvDegree = maxDegree;
        forceConfig.grvOrder = maxOrder;

        forceConfig.solidTide = solidTide;
        forceConfig.oceanTide = oceanTide;
        forceConfig.poleTide = poleTide;

        return (*this);

    }  // End of method 'OrbitModel::enableGeopotential()'


    OrbitModel& OrbitModel::enableThirdBodyPerturbation(const bool& bsun,
        const bool& bmoon)
    {
        // DONOT allow to change the configuration 
        if (fmlPrepared) return (*this);

        forceConfig.geoSun = bsun;
        forceConfig.geoMoon = bmoon;

        return (*this);

    }  // End of method 'OrbitModel::enableThirdBodyPerturbation()'


    OrbitModel& OrbitModel::enableAtmosphericDrag(OrbitModel::AtmosphericModel model,
        const bool& bdrag)
    {
        // DONOT allow to change the configuration 
        if (fmlPrepared) return (*this);

        forceConfig.atmModel = model;
        forceConfig.atmDrag = bdrag;

        return (*this);

    }  // End of method 'OrbitModel::enableAtmosphericDrag()'


    OrbitModel& OrbitModel::enableSolarRadiationPressure(bool bsrp)
    {
        // DONOT allow to change the configuration 
        if (fmlPrepared) return (*this);

        forceConfig.solarPressure = bsrp;

        return (*this);

    }  // End of method 'OrbitModel::enableSolarRadiationPressure()'


    OrbitModel& OrbitModel::enableRelativeEffect(const bool& brel)
    {
        // DONOT allow to change the configuration 
        if (fmlPrepared) return (*this);

        forceConfig.relEffect = brel;

        return (*this);

    }  // End of method 'OrbitModel::enableRelativeEffect()' 

    void OrbitModel::createFMObjects(FMCData& fmc)
    {
        // First, we release the memory
        deleteFMObjects(fmc);

        // GeoEarth

        fmc.pGeoEarth = new GravityModel(fmc.grvDegree,fmc.grvOrder);

        // GeoSun
        fmc.pGeoSun = new SunForce();

        // GeoMoon
        fmc.pGeoMoon = new MoonForce();

        // AtmDrag
        if (fmc.atmModel == AM_HarrisPriester)
        {
            fmc.pAtmDrag = new HarrisPriesterDrag();
        }
        else if (fmc.atmModel == AM_MSISE00)
        {
            fmc.pAtmDrag = new Msise00Drag();
        }
        else if (fmc.atmModel == AM_CIRA)
        {
            fmc.pAtmDrag = new CiraExponentialDrag();
        }
        else
        {
            // Unexpected, never go here
        }

        // SRP
        fmc.pSolarPressure = new SolarRadiationPressure();

        // Rel
        fmc.pRelEffect = new RelativityEffect();

        // Now, it's time to check if we create these objects successfully
        if (!fmc.pGeoEarth || !fmc.pGeoSun || !fmc.pGeoMoon ||
            !fmc.pAtmDrag || !fmc.pSolarPressure || !fmc.pRelEffect)
        {
            // deallocate allocated memory
            deleteFMObjects(fmc);

            Exception e("Failed to allocate memory for force models !");
            GPSTK_THROW(e);
        }

        // At last, prepare the force model list
        fmc.pGeoEarth->setDesiredDegree(fmc.grvDegree, fmc.grvOrder);
        fmc.pGeoEarth->enableSolidTide(fmc.solidTide);
        fmc.pGeoEarth->enableOceanTide(fmc.oceanTide);
        fmc.pGeoEarth->enablePoleTide(fmc.poleTide);

        fmc.pAtmDrag->setSpaceData(fmc.dailyF107,
            fmc.averageF107, fmc.dailyKp);

        forceList.clear();

        if (fmc.geoEarth) forceList.addForce(fmc.pGeoEarth);
        if (fmc.geoSun) forceList.addForce(fmc.pGeoSun);
        if (fmc.geoMoon) forceList.addForce(fmc.pGeoMoon);
        if (fmc.atmDrag) forceList.addForce(fmc.pAtmDrag);
        if (fmc.solarPressure) forceList.addForce(fmc.pSolarPressure);
        if (fmc.relEffect) forceList.addForce(fmc.pRelEffect);

        // set the flag
        fmlPrepared = true;

    }  // End of method 'OrbitModel::createFMObjects()'


    void OrbitModel::deleteFMObjects(FMCData& fmc)
    {
        // GeoEarth
        if (fmc.pGeoEarth)
        {
            delete fmc.pGeoEarth;
            fmc.pGeoEarth = NULL;
        }

        // GeoSun
        if (fmc.pGeoSun)
        {
            delete fmc.pGeoSun;
            fmc.pGeoSun = NULL;
        }

        // GeoMoon
        if (fmc.pGeoMoon)
        {
            delete fmc.pGeoMoon;
            fmc.pGeoMoon = NULL;
        }

        // AtmDrag
        if (fmc.pAtmDrag)
        {
            if (fmc.atmModel == AM_HarrisPriester)
            {
                delete (HarrisPriesterDrag*)fmc.pAtmDrag;
                fmc.pAtmDrag = NULL;
            }
            else if (fmc.atmModel == AM_MSISE00)
            {
                delete (Msise00Drag*)fmc.pAtmDrag;
                fmc.pAtmDrag = NULL;
            }
            else if (fmc.atmModel == AM_CIRA)
            {
                delete (CiraExponentialDrag*)fmc.pAtmDrag;
                fmc.pAtmDrag = NULL;
            }
            else
            {
                delete fmc.pAtmDrag;
                fmc.pAtmDrag = NULL;
            }
        }

        // SRP
        if (fmc.pSolarPressure)
        {
            delete fmc.pSolarPressure;
            fmc.pSolarPressure = NULL;
        }

        // Rel
        if (fmc.pRelEffect)
        {
            delete fmc.pRelEffect;
            fmc.pRelEffect = NULL;
        }



        // set the flag
        fmlPrepared = false;

    }  // End of method 'OrbitModel::uninstallForceModelList()'


}
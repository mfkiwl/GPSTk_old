#ifndef POD_ORBIT_MODEL_H
#define POD_ORBIT_MODEL_H

#include"EarthRotation.h"

#include"ForceModelList.hpp"
#include"EquationOfMotion.hpp"
#include"GravityModel.h"
#include"SunForce.hpp"
#include"MoonForce.hpp"
#include"AtmosphericDrag.hpp"
#include"SolarRadiationPressure.hpp"
#include"RelativityEffect.hpp"

using namespace gpstk;
namespace POD
{
    class OrbitModel : public EquationOfMotion
    {
    public:
        /// Valid gravity models
        enum GravityModelType
        {
            GM_JGM3,
            GM_EGM96
        };

        /// Valid atmospheric models
        enum AtmosphericModel
        {
            AM_HarrisPriester,
            AM_MSISE00,
            AM_CIRA
        };

        /// Struct to hold force model setting data
        struct FMCData
        {
            bool geoEarth;
            bool geoSun;
            bool geoMoon;
            bool atmDrag;
            bool relEffect;
            bool solarPressure;

            int grvDegree;
            int grvOrder;

            bool solidTide;
            bool oceanTide;
            bool poleTide;

            AtmosphericModel atmModel;

            // We'll allocate memory in the heap for some of the models are memory
            // consuming

            GravityModel* pGeoEarth;

            SunForce* pGeoSun;

            MoonForce* pGeoMoon;

            AtmosphericDrag* pAtmDrag;

            SolarRadiationPressure* pSolarPressure;

            RelativityEffect* pRelEffect;

            double dailyF107;
            double averageF107;
            double dailyKp;

            FMCData()
            {
                geoEarth = true;
                geoSun = geoMoon = false;
                atmDrag = false;
                relEffect = false;
                solarPressure = false;

                grvDegree = 1;
                grvOrder = 1;

                solidTide = oceanTide = poleTide = false;

                atmModel = AM_HarrisPriester;

                pGeoEarth = NULL;
                pGeoSun = NULL;
                pGeoMoon = NULL;
                pAtmDrag = NULL;
                pSolarPressure = NULL;
                pRelEffect = NULL;

                dailyF107 = 150.0;
                averageF107 = 150.0;
                dailyKp = 3.0;
            }
        };

        /// Default constructor
        OrbitModel() : fmlPrepared(false)
        {
            reset();
        }

        /// Default destructor
        virtual ~OrbitModel()
        {
            deleteFMObjects(forceConfig);
        }


        virtual Vector<double> getDerivatives(const double&t, const Vector<double>& y);

        /// Restore the default setting
        OrbitModel& reset()
        {
            deleteFMObjects(forceConfig); fmlPrepared = false; init(); return(*this);
        }

        /// set reference epoch
        OrbitModel& setRefEpoch(CommonTime utc)
        {
            t0 = utc; return (*this);
        }

        /// get reference epoch
        CommonTime getRefEpoch() const
        {
            return t0;
        }


        /// set spacecraft physical parameters
        OrbitModel& setSpacecraftData(std::string name = "sc-test01",
            const double& mass = 1000.0,
            const double& area = 20.0,
            const double& areaSRP = 20.0,
            const double& Cr = 1.0,
            const double& Cd = 2.2);

        /// set space data
        OrbitModel& setSpaceData(double dayF107 = 150.0,
            double aveF107 = 150.0,
            double dayKp = 3.0);


        // Methods to config the orbit perturbation force models
        // call 'reset()' before call these methods

        OrbitModel& enableGeopotential(
            const int& maxDegree = 1,
            const int& maxOrder = 1,
            const bool& solidTide = false,
            const bool& oceanTide = false,
            const bool& poleTide = false);

        OrbitModel& enableThirdBodyPerturbation(const bool& bsun = false,
            const bool& bmoon = false);


        OrbitModel& enableAtmosphericDrag(OrbitModel::AtmosphericModel model
            = OrbitModel::AM_HarrisPriester,
            const bool& bdrag = false);


        OrbitModel& enableSolarRadiationPressure(bool bsrp = false);


        OrbitModel& enableRelativeEffect(const bool& brel = false);


        /// For POD , and it's will be improved later
        void setForceModelType(std::set<ForceModel::ForceModelType> fmt)
        {
            forceList.setForceModelType(fmt);
        }
    protected:

        virtual void init();

        void createFMObjects(FMCData& fmc);

        void deleteFMObjects(FMCData& fmc);

        /**
        * Adds a generic force to the list
        */
        void addForce(ForceModel* pForce)
        {
            forceList.addForce(pForce);
        }

        /// Reference epoch
        CommonTime t0;

        /// Spacecraft object
        Spacecraft sc;

        ///  Earth Body
        EarthBody  earthBody;

        /// Object holding force model consiguration
        FMCData forceConfig;

        /// Flag indicate if the ForceModelList has been prepared
        /// 'forceConfig' can't be change when 'fmlPrepared' is true
        bool fmlPrepared;

        /// Force Model List
        ForceModelList forceList;
    };
}
#endif // !POD_ORBIT_MODEL_H

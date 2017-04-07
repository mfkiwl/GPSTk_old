#ifndef POD_ORBIT_MODEL_H
#define POD_ORBIT_MODEL_H

#include"EarthRotation.h"

#include"ForceModelData.h"

#include"ForceModelList.hpp"
#include"EquationOfMotion.hpp"


using namespace gpstk;
namespace POD
{
    class OrbitModel : public EquationOfMotion
    {
    public:

        /// Default constructor
        OrbitModel(const ForceModelData& fmc);

        /// Default destructor
        virtual ~OrbitModel()
        {
            forceList.clear();
        }

        virtual Vector<double> getDerivatives(const double&t, const Vector<double>& y);

        /// Restore the default setting
        OrbitModel& reset(const ForceModelData& fmc);

        /// set reference epoch
        OrbitModel& setRefEpoch(const CommonTime & utc)
        {
            t0 = utc; 
            return (*this);
        }

        /// get reference epoch
        CommonTime getRefEpoch() const
        {
            return t0;
        }


    protected:

        /// Reference epoch
        CommonTime t0;

        /// Spacecraft object
        Spacecraft sc;

        /// Force Model List
        ForceModelList forceList;
    };
}
#endif // !POD_ORBIT_MODEL_H

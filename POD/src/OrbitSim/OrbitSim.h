#ifndef POD_ORBIT_SIM_H
#define POD_ORBIT_SIM_H


#include"EOPDataStore.hpp"
#include"EarthRotation.h"
#include"SatOrbit.hpp"
#include"RungeKuttaFehlberg.hpp"
#include"Integrator.hpp"

using namespace gpstk;

namespace POD
{
    class OrbitSim
    {
    private:

        RungeKuttaFehlberg rkfIntegrator;
        SatOrbit defOrbitModel;

    protected:
  
        /// Pointer to an ode solver default is RungeKutta78
        Integrator*   pIntegrator;

        /// Pointer to the Equation Of Motion of a satellite
        SatOrbit*   pOrbit;

    public:

        static  EarthRotation erp;

        void IntegrateTo(const CommonTime & te);
        
        void IntegrateTo(const double & dt);


        //   void test();
        static void runTest();

    };
}

#endif // !POD_ORBIT_SIM_H


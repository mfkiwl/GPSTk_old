#ifndef ORBIT_SIM_HPP
#define ORBIT_SIM_HPP

#include"stdafx.h"
#include"EOPDataStore.hpp"
#include"EarthRotation.h"

using namespace gpstk;

namespace POD
{
    class OrbitSim
    {
    private:
        RungeKuttaFehlberg rkfIntegrator;
        SatOrbit defOrbitModel;

    protected:
        EarthRotation erp;

        /// Pointer to an ode solver default is RungeKutta78
        Integrator*   pIntegrator;

        /// Pointer to the Equation Of Motion of a satellite
        SatOrbit*   pOrbit;

    public:

        void IntegrateTo(const CommonTime & te);
        
        void IntegrateTo(const double & dt);


        //   void test();
        static void runTest();

    };
}

#endif // !ORBIT_SIM_HPP


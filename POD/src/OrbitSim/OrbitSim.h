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
    protected:
        EarthRotation erp;

    public:


        //   void test();
        static void runTest();

    };
}

#endif // !ORBIT_SIM_HPP


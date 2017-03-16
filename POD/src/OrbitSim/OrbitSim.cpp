#include"OrbitSim.h"

#include"KeplerOrbit.hpp"
#include"SatOrbit.hpp"
#include"SatOrbitPropagator.hpp"
#include"IERS.hpp"

//using namespace gpstk;

namespace POD
{




    void OrbitSim:: runTest()
    {


        ofstream os("Integr_test.out");

        SatOrbitPropagator op;

        SatOrbit orb;
        Vector<double> elts(6, 0.0);
        double T(5200.0), step ( 1.0),tt (864000.0*7);

        elts(0) = 6487264.0502067700; //A, m
        elts(1) = 0.001;              //ecc
        elts(2) = 1.0;                //i, rad
        elts(3) = 2.0;                //OMG, rad
        elts(4) = 3.0;                //omg, rad
        elts(4) = 0.0;                //omg, rad
        
        UTCTime utc(2013, 01, 30, 0, 0, 0.0);
        // mu = 3.98600441500e+14;
        Vector<double> sv = KeplerOrbit::State(3.98600441500e+14, elts, 0);

        op.setInitState(utc, sv);

        os << fixed << setw(12) << setprecision(5);
        os << op.getCurTime() <<" "<< op.getCurState() <<endl;
        
        //
        double t = 0;
        //
        while (t < tt)
        {
            if (!op.integrateTo(t + step))
            {
                cout << "failed to integrate\n";
                break;
            }
            t += step;
            if (fmod(t, T) == 0)
                os << op.getCurTime() << " " << op.getCurState() << endl;
        }
        os.close();
    }
}
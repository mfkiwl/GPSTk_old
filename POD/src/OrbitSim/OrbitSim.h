#ifndef POD_ORBIT_SIM_H
#define POD_ORBIT_SIM_H


#include"EOPDataStore.hpp"
#include"EarthRotation.h"
#include"OrbitModel.h"
#include"RungeKuttaFehlberg.hpp"
#include"Integrator.hpp"

#include"OrbitModel.h"
using namespace gpstk;

namespace POD
{
    //typedef unique_ptr<Integrator> IntegratorUniqPtr;
    //typedef unique_ptr<OrbitModel> OrbitModelUniqPtr;
    class OrbitSim
    {

    public:
        
        ///
        static  EarthRotation erp;

        /// Default constructor
        OrbitSim();

        /// Default destructor
        virtual ~OrbitSim();

        /* set force model setting
        */
        //void setForceModel(ForceModelSetting& fms);


        /// set integrator, default is Rungge-Kutta 78
        OrbitSim& setIntegrator(Integrator* pIntg)
        {
            pIntegrator  pIntg; return (*this);
        }


        /// set the integrator to the default one
        OrbitSim& setDefaultIntegrator()
        {
            pIntegrator = srkfIntegrator; return (*this);
        }

        /// set equation of motion of the orbit
        OrbitSim& setOrbit(OrbitModel* porbit)
        {
            pOrbit = porbit; return (*this);
        }


        /// set the orbit to the default one
        OrbitSim& setDefaultOrbit()
        {
            pOrbit = &defOrbitModel; return (*this);
        }

        /// set step size of the integrator
        OrbitSim& setStepSize(double step_size = 10.0)
        {
            pIntegrator->setStepSize(step_size); return (*this);
        }

        /**set init state
        * @param utc0   init epoch
        * @param rv0    init state
        * @return
        */
        OrbitSim& setInitState(CommonTime utc0, Vector<double> rv0);


        /** Take a single integration step.
        * @param tf    next time
        * @return      state of integration
        */
        virtual bool integrateTo(double tf);


        /// return the position and velocity , the dimension is 6
        Vector<double> rvState(bool bJ2k = true);

        /// return the rv state transition matrix 6*6
        Matrix<double> transitionMatrix()
        {
            return phiMatrix;
        }

        /// return the sensitivity matrix 6*np
        Matrix<double> sensitivityMatrix()
        {
            return sMatrix;
        }

        /// return the current epoch
        CommonTime getCurTime()
        {
            CommonTime utc = pOrbit->getRefEpoch(); utc += curT; return utc;
        }

        /// return the current state
        Vector<double> getCurState()
        {
            return curState;
        }

        /// get numble of force model parameters
        int getNP() const
        {
            return (curState.size() - 42) / 6;
        }

        /// get the pointer to the satellite orbit object
        OrbitModel* getOrbitModelPointer()
        {
            return pOrbit;
        }

        /// write curT curState to a file
        void writeToFile(std::ostream& s) const;

        /*
        * try to integrate ephemeris and print it to a file
        * just for compare it with stk8.1
        *
        * yanwei,Sep 19 th,2009
        */
        //void makeSatEph(OrbitSetting& os,string fileName);
        //void makeRefSatEph(OrbitSetting& os,string fileName);

        /* For Testing and Debuging...
        */


        //void IntegrateTo(const CommonTime & te);
        //
        //void IntegrateTo(const double & dt);


          void test();
        static void runTest();


    protected:

        /** Take a single integration step.
        *
        * @param x     time or independent variable
        * @param y     containing needed inputs (usually the state)
        * @param tf    next time
        * @return      containing the new state
        */
        virtual Vector<double> integrateTo(double t, Vector<double> y, double tf);

        /* set initial state of the the integrator
        *
        *  v      3
        * dr_dr0    3*3
        * dr_dv0   3*3
        * dr_dp0   3*np
        * dv_dr0   3*3
        * dv_dv0   3*3
        * dv_dp0   3*np
        */
        void setState(Vector<double> state);

        /// set reference epoch
        void setRefEpoch(CommonTime utc)
        {
            pOrbit->setRefEpoch(utc);
        }

        /// update phiMatrix sMatrix and rvState from curState
        void updateMatrix();


        /// Pointer to an ode solver default is RungeKutta78
        Integrator*   pIntegrator;

        /// Pointer to the Equation Of Motion of a satellite
        OrbitModel*  pOrbit;

    private:

        /// The default integrator is RKF78
        RungeKuttaFehlberg rkfIntegrator;

        /// The default orbit is kepler orbit
        OrbitModel  defOrbitModel;

        /// current time since reference epoch
        double curT;

        /// current state
        // r        3
        // v        3
        // dr_dr0   3*3
        // dr_dv0   3*3
        // dr_dp0   3*np
        // dv_dr0   3*3
        // dv_dv0   3*3
        // dv_dp0   3*np
        Vector<double> curState;         // 42+6*np

        /// the position and velocity
        Vector<double>   rvVector;      // 6

        /// state transition matrix
        Matrix<double> phiMatrix;      // 6*6

        /// the sensitivity matrix 
        Matrix<double> sMatrix;         // 6*np


    };
}

#endif // !POD_ORBIT_SIM_H


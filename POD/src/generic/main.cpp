#include "Solution.h"
#include "EarthRotation.h"
using namespace POD;
int main(int argc, char *argv[])
{
    //POD:: Solution sol(argv[1]);
    //sol.process();
    //POD::OrbitSim::runTest();
    //POD::SatOrbitPropagator2  op;
    //op.test();
    EarthRotation ERP;
    cout << "ERP loading...";
    cout<<ERP.loadEOP("finals2000A.data",EOPDataStore::EOPSource::IERS)<<endl;
    ERP.test();
    /*CivilTime ct(2012, 10, 10, 2, 3, 1, TimeSystem::GPS);
    CommonTime epoch = (CommonTime)ct;
    Matrix<double> C2E = ERP.getJ2k2ECEF(epoch);
    cout << C2E << endl;*/

    return 0;
}

#include"stdafx.h"
#include"EarthRotation.h"
#include"EOPDataStore.hpp"

using namespace gpstk;
using namespace std;

namespace POD
{
    EarthRotation::EarthRotation()
    {

    }
    EarthRotation::EarthRotation(const EOPDataStore & eop): eopData(eop)
    {

    }

    bool EarthRotation:: loadEOP(const string &  file, EOPDataStore::EOPSource source)
    {
        try
        {
            this->eopData.loadFile(file,source);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    Matrix<double> getJ2k2ECEF(const CommonTime & utc)
    {

    }
    
    Matrix<double> getECEF2J2k(const CommonTime & utc);


}

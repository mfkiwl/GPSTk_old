#ifndef EARTH_ROTATION_HPP
#define EARTH_ROTATION_HPP

#include"stdafx.h"
#include"sofa.h"
using namespace gpstk;
using namespace std;

namespace POD
{

    class EarthRotation
	{
    public:
        
        EarthRotation();
        EarthRotation(const EOPDataStore & eop);
        bool loadEOP(const string &  file, EOPDataStore::EOPSource source)
            throw(InvalidRequest,FileMissingException);
        Matrix<double> getJ2k2ECEF(const CommonTime & utc);
        Matrix<double> getECEF2J2k(const CommonTime & utc);

    protected:
        EOPDataStore eopData;
		
    private:
       
    };
}
#endif // !EARTH_ROTATION_HPP
#ifndef EARTH_ROTATION_HPP
#define EARTH_ROTATION_HPP

#include"stdafx.h"
using namespace gpstk;
using namespace std;

namespace POD
{

    class EarthRotation
	{
    public:
        
        EarthRotation();
        EarthRotation(const EOPDataStore & eop);
        bool loadEOP(const string &  file, EOPDataStore::EOPSource source= EOPDataStore::EOPSource::IERS)
            throw(InvalidRequest,FileMissingException);
        Matrix<double> getJ2k2ECEF(const CommonTime & t);
        Matrix<double> getECEF2J2k(const CommonTime & t);
        Vector<double> J2k_2_ECEF(const CommonTime & t, const Vector<double> pos);
        Vector<double> ECEF_2_J2k(const CommonTime & t, const Vector<double> pos);
        virtual ~EarthRotation()
        {
            eopData.clear();
        }

        bool test();
    protected:
        EOPDataStore eopData;
		
    private:
		static CivilTime toTAI(const CommonTime & t) throw(InvalidParameter);
    };
}
#endif // !EARTH_ROTATION_HPP
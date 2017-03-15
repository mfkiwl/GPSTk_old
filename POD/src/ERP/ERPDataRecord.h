#ifndef ERP_DATA_RECORD_HPP
#define ERP_DATA_RECORD_HPP

#include"stdafx.h"
using namespace gpstk;
namespace POD
{
    class EOPDataRecord
    {
    public:
        EOPDataRecord() : xp(0.0), yp(0.0), UT1mUTC(0.0), dPsi(0.0), dEps(0.0)
        {}

        EOPDataRecord(double x, double y, double ut1_utc, double dpsi = 0.0, double deps = 0.0)
            : xp(x), yp(y), UT1mUTC(ut1_utc), dPsi(dpsi), dEps(deps)
        {}
        double xp;        /// arcseconds
        double yp;        /// arcseconds
        double UT1mUTC;   /// seconds
        double dPsi;      /// arcseconds
        double dEps;      /// arcseconds
    };
}

#endif // !ERP_DATA_RECORD_HPP

#ifndef EOP_DATA_STORE_HPP
#define EOP_DATA_STORE_HPP

#include"stdafx.h"
using namespace gpstk;
using namespace std;
namespace POD
{

    struct EOPDataRecord
    {
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

    class EOPDataStore
    {
        typedef std::map<CommonTime, EOPDataRecord> ERPDataTable;

        ERPDataTable table;
    public:
        bool loadIGSData(const list<string> & files);
        bool loadIGSData(const string & files);



    };
}
#endif // !EOP_DATA_STORE_HPP
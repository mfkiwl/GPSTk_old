#include"EOPDataStore.h"
#include <iostream>

using namespace gpstk;
using namespace std;

namespace POD
{
    bool EOPDataStore::loadIGSData(const list<string> & files)
    {
        table.clear();
        for (auto it : files)
            loadIGSData(it);
    }
    bool EOPDataStore::loadIGSData(const string & igsFile)
    {
        ifstream inpf(igsFile.c_str());
        if (!inpf)
        {
            FileMissingException fme("Could not open IERS file " + igsFile);
            GPSTK_THROW(fme);
        }

        // first we skip the header section
        // skip the header

        //version 2
        //EOP  SOLUTION
        //  MJD         X        Y     UT1-UTC    LOD   Xsig   Ysig   UTsig LODsig  Nr Nf Nt     Xrt    Yrt  Xrtsig Yrtsig   dpsi    deps
        //               10**-6"        .1us    .1us/d    10**-6"     .1us  .1us/d                10**-6"/d    10**-6"/d        10**-6

        string temp;
        getline(inpf, temp);
        getline(inpf, temp);
        getline(inpf, temp);
        getline(inpf, temp);

        bool ok(true);
        while (!inpf.eof() && inpf.good())
        {
            string line;
            getline(inpf, line);
            StringUtils::stripTrailing(line, '\r');
            if (inpf.eof()) break;

            // line length is actually 185
            if (inpf.bad() || line.size() < 120) { ok = false; break; }

            istringstream istrm(line);

            double mjd(0.0), xp(0.0), yp(0.0), UT1mUTC(0.0), dPsi(0.0), dEps(0.0);

            istrm >> mjd >> xp >> yp >> UT1mUTC;

            for (int i = 0; i<12; i++) istrm >> temp;

            istrm >> dPsi >> dEps;

            xp *= 1e-6;
            yp *= 1e-6;
            UT1mUTC *= 1e-7;

            dPsi *= 1e-6;
            dEps *= 1e-6;
            
            addEOPData(MJD(mjd, TimeSystem::UTC), EOPData(xp, yp, UT1mUTC,0,0, dPsi, dEps));
    }
}

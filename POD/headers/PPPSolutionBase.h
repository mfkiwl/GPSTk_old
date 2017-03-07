#ifndef PPP_SOLUTION_BASE
#define PPP_SOLUTION_BASE


#include "stdafx.h"
#include"PRSolverLEO.h"
using namespace gpstk;
namespace POD
{
    class PPPSolutionBase 
    {
    public:
        static PPPSolutionBase * Factory(bool isSpaceborne, ConfDataReader & confReader);

        PPPSolutionBase(ConfDataReader & confReader);

        virtual ~PPPSolutionBase()
        {
            delete solverPR;
        }

        bool LoadData();
        bool loadEphemeris();
        bool loadIono();
        bool loadClocks();

        void checkObservable(const string & path);

        void process();
        virtual void PPPprocess()=0;

    protected:

        string genFilesDir;

        uchar maskSNR;
        double maskEl;

        const char * L1CCodeID = "C1" ;
        const char * L1PCodeID = "C1W";
        const char * L2CodeID  = "C2W";
        const char * L1CNo     = "S1C";
        //
        int DoY = 0;
        //
        Position nominalPos;

        // Configuration file reader
        ConfDataReader* confReader;

        // object to handle precise ephemeris and clocks
        SP3EphemerisStore SP3EphList;
        //
        PRSolverBase *solverPR;
        IonoModelStore ionoStore;
        list<string> rinexObsFiles;
        map<CommonTime, Xvt, std::less<CommonTime>> apprPos;
    };
}
#endif // !1
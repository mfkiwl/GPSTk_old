#ifndef AUTONOMUS
#define AUTONOMUS
#include"stdafx.h"
#include"PRSolverLEO.h"

using namespace gpstk;
typedef  unsigned char uchar;


class Autonomus :public BasicFramework
{

public :
    Autonomus(char* arg0, char * discr);
    virtual ~Autonomus()
    {
        delete solverPR;
    }
    
    bool loadConfig(char* path);
    bool loadEphemeris();
	bool loadIono();
    bool loadClocks();
    void swichToSpaceborn();
    void swichToGroundBased(TropModel & tModel);
	void checkObservable(const string & path);

    virtual void process()
    {};
    void initProcess();
    void PRprocess();
    void PPPprocess();

protected:
    bool PPPprocessGB();
    bool PPPprocessLEO();
    bool loadApprPos(std::string path);

    void printModel(ofstream& modelfile,
                         const gnssRinex& gData,
                         int   precision);

    void printSolution(ofstream& outfile,
                       const SolverLMS& solver,
                       const CommonTime& time0,
                       const CommonTime& time,
                       const ComputeDOP& cDOP,
                       bool  useNEU,
                       int   numSats,
                       double dryTropo,
                       vector<PowerSum> &stats,
                       int   precision,
                       const Position &nomXYZ);

    void printSolutionLEO(ofstream& outfile,
                       const SolverLMS& solver,
                       const CommonTime& time0,
                       const CommonTime& time,
                       const ComputeDOP& cDOP,
                       bool  useNEU,
                       int   numSats,
                       vector<PowerSum> &stats,
                       int   precision,
                       const Position &nomXYZ);

    void printStats(ofstream& outfile, const vector<PowerSum> &stats);
    
    //
    int getNumCS(const gnssRinex& gdata);

    bool isSpace = false;
    string genFilesDir;

    uchar maskSNR;
    double maskEl;
     
	 const char * L1CCodeID = "C1";
     const char * L1PCodeID = "C1W";
     const char * L2CodeID = "C2W";
     const char * L1CNo =    "S1C";
    //
     int DoY = 0;
    //
     Position nominalPos;
    // This field represents an option at command line interface (CLI)
    CommandOptionWithArg confFile;

    // Configuration file reader
    ConfDataReader confReader;

    // object to handle precise ephemeris and clocks
    SP3EphemerisStore SP3EphList;
	//
    PRSolverBase *solverPR;
	IonoModelStore ionoStore;
    list<string> rinexObsFiles;
    map<CommonTime, Xvt,std::less<CommonTime>> apprPos;

};

#endif // !1
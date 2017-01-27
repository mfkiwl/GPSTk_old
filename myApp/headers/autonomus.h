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

    virtual void process()
    {};
    void PRprocess();
    void PPPprocess();
    bool PPPprocess2();
    
protected:
    
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
                       int   precision);

    void Autonomus::printStats(ofstream& outfile, const vector<PowerSum> &stats);
    string genFilesDir;
private:

	 const char * L1CCodeID = "C1";
     const char * L1PCodeID = "C1";
     const char * L2CodeID = "C2W";
     const char * L1CNo =    "S1C";

    // This field represents an option at command line interface (CLI)
    CommandOptionWithArg confFile;

    // If you want to share objects and variables among methods, you'd
    // better declare them here

    // Configuration file reader
    ConfDataReader confReader;

    // object to handle precise ephemeris and clocks
    SP3EphemerisStore SP3EphList;
	//
    PRSolverBase *solverPR;
	IonoModelStore ionoStore;
    list<string> rinesObsFiles;
    map<CommonTime, Xvt> apprPos;

};

#endif // !1
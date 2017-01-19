#ifndef AUTONOMUS
#define AUTONOMUS
#include"stdafx.h"

using namespace gpstk;

class Autonomus :public BasicFramework
{

public :
    Autonomus(char* arg0, char * discr);

    bool loadConfig(char* path);
    bool loadEphemeris();
	bool loadIono();
    bool loadClocks();

    virtual void process();

protected:
     
private:

	 const  char * L1CCodeID = "C1";
     const char * L1PCodeID = "C1";
     const char * L2CodeID = "C2W";
     const char * L1CNo =    "S1C";

    // This field represents an option at command line interface (CLI)
    CommandOptionWithArg confFile;

    // If you want to share objects and variables among methods, you'd
    // better declare them here

    // Configuration file reader
    ConfDataReader confReader;

    //Input observation file stream
    Rinex3ObsStream rin;
    // object to handle precise ephemeris and clocks
    SP3EphemerisStore SP3EphList;
	//
    PRSolutionLEO solverLEO;
	IonoModelStore ionoStore;
    list<string> rinesObsFiles;
};

#endif // !1
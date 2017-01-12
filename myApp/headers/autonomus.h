#ifndef AUTONOMUS
#define AUTONOMUS
#include"stdafx.h"
using namespace gpstk;

class Autonomus :public BasicFramework
{

public :

    Autonomus(char* arg0, char * discr);
    void loadConfig(char* path);
    bool loadEphemeris();
    bool loadClocks();
    bool checkObsFile();
    virtual void process();

protected:
     
private:

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
};

#endif // !1
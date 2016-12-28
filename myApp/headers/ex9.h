#ifndef EX9_HPP
#define EX9_HPP

#include"stdafx.h"

class ex9 : public BasicFramework
{
public:

    // Constructor declaration
    ex9(char* arg0);


protected:


    // Method that will take care of processing
    virtual void process();

    bool pocessStataion(const string & station, const string & outName);

    bool checkObsFile(const string & station);

    bool loadSatsData(const string & station);

    // Method that hold code to be run BEFORE processing
    virtual void spinUp();

private:

    // This field represents an option at command line interface (CLI)
    CommandOptionWithArg confFile;

    // If you want to share objects and variables among methods, you'd
    // better declare them here

    // Configuration file reader
    ConfDataReader confReader;

    //Input observation file stream
    RinexObsStream rin;
    // object to handle precise ephemeris and clocks
    SP3EphemerisStore SP3EphList;

    // Declare our own methods to handle output


    // Method to print solution values
    void printSolution(ofstream& outfile,
        const  SolverLMS& solver,
		const  CommonTime& time0,
        const  CommonTime& time,
        const  ComputeDOP& cDOP,
        bool   useNEU,
        int    numSats,
        double dryTropo,
		vector<PowerSum> &stats,
		int    precision=3
	);
	void printStats(ofstream& outfile, 
		const vector<PowerSum> &statistic);


    // Method to print model values
    void printModel(ofstream& modelfile,
        const gnssRinex& gData,
        int   precision = 4);


}; // End of 'ex9' class declaration

#endif // !EX9_HPP

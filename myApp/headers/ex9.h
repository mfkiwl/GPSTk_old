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

    // Method that hold code to be run BEFORE processing
    virtual void spinUp();

private:

    // This field represents an option at command line interface (CLI)
    CommandOptionWithArg confFile;

    // If you want to share objects and variables among methods, you'd
    // better declare them here

    // Configuration file reader
    ConfDataReader confReader;


    // Declare our own methods to handle output


    // Method to print solution values
    void printSolution(ofstream& outfile,
        const  SolverLMS& solver,
        const  CommonTime& time,
        const  ComputeDOP& cDOP,
        bool   useNEU,
        int    numSats,
        double dryTropo,
        int    precision = 3);


    // Method to print model values
    void printModel(ofstream& modelfile,
        const gnssRinex& gData,
        int   precision = 4);


}; // End of 'ex9' class declaration

#endif // !EX9_HPP

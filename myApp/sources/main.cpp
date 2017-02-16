// myApp.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
using namespace std;
int main(int argc, char *argv[])
{

    Autonomus  proc(argv[0], "autonomus processing");
    
    //data loading part
    cout << "Config Loading... ";
    cout << proc.loadConfig(argv[1]) << endl;
    cout << "Ephemeris Loading... ";
    cout << proc.loadEphemeris() << endl;
    cout << "Clocks Loading... "  ;
    cout << proc.loadClocks() << endl;
    cout << "IonoModel Loading... ";
    cout << proc.loadIono() << endl;
    
    //processing part
	//proc.checkObservable("checkObs.out");
    //proc.initProcess();
    //proc.PRprocess();
      proc.PPPprocess();
}
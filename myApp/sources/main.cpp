// myApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


using namespace std;

int main(int argc, char *argv[])
{

    Autonomus  proc(argv[0],"autonomus processing");

   cout<< "Config Loading "<< proc.loadConfig(argv[1])<<endl;
   cout << "Ephemeris Loading " << proc.loadEphemeris() << endl;
 //  cout << "Clocks Loading " << proc.loadClocks() << endl;
   cout << "IonoModel Loading" << proc.loadIono() << endl;
   cout << "Check files with obs " << proc.checkObsFile() << endl;
   proc.process2();
}
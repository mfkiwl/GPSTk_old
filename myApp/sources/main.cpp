// myApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


using namespace std;

int main(int argc, char *argv[])
{
    vector<string> files;
    cout << auxiliary::getAllFiles("subdir1", files) << endl;
    for (auto it : files)
        cout << it << endl;

    Autonomus  proc(argv[0],"autonomus processing");

   cout<< "Config Loading "<< proc.loadConfig(argv[1])<<endl;
   cout << "Ephemeris Loading " << proc.loadEphemeris() << endl;
 //  cout << "Clocks Loading " << proc.loadClocks() << endl;
   cout << "IonoModel Loading" << proc.loadIono() << endl;
   cout << "Check files with obs " << proc.checkObsFile() << endl;
   proc.process();
}
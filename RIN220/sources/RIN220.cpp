
// myApp.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

using namespace std;
using namespace gpstk;

int main(int argc, char *argv[])
{

    for (size_t i = 1; i < argc; i++){
        
        string s = argv[i];
        cout << s << endl;

        Rinex3ObsStream rin220(s);

        //rin220 >> rin220.header;
        //cout << rin220.header.version<<endl;
        //rin.open(path, std::ios::in);
        gnssRinex gRin;
        Rinex3ObsHeader roh;
            rin220 >> roh;

        while (rin220 >> gRin)
        {
            //rin220.header.dump(cout);
            CommonTime time(gRin.header.epoch);
            cout << static_cast<YDSTime>(time);
            cout << " "<< gRin.numSats() ;
            auto vect = gRin.getVectorOfSatID();

            for each (auto it in vect)
            {
                cout << it << " ";
            }
            cout << endl;
           
        }        rin220.header.dump(cout);
    }
    system("pause");
}
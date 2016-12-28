
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

        RinexObsStream rin220(s);

       // rin220 >> rin220.header;
       // cout << rin220.header.version<<endl;
        //rin.open(path, std::ios::in);
        gnssRinex gRin;

        while (rin220 >> gRin)
        {
            CommonTime time(gRin.header.epoch);
            cout << time << endl;
        }
    }
    system("pause");
}
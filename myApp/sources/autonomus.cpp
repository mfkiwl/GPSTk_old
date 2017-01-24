#include"autonomus.h"
#include<list>

Autonomus:: Autonomus(char* arg0, char * discr )
    :
    BasicFramework(arg0,
        discr),
    // Option initialization. "true" means a mandatory option
    confFile(CommandOption::stdType,
        'c',
        "conffile",
        " [-c|--conffile]    Name of configuration file ('config.txt' by default).",
        false)
{

    // This option may appear just once at CLI
    confFile.setMaxCount(1);

}  // End of 'ex9::ex9

bool Autonomus::loadConfig(char* path)
{
    // Check if the user provided a configuration file name
    if (confFile.getCount() > 0) {

        // Enable exceptions
        confReader.exceptions(ios::failbit);

        try {

            // Try to open the provided configuration file
            confReader.open(confFile.getValue()[0]);

        }
        catch (...) {

            cerr << "Problem opening file "
                << confFile.getValue()[0]
                << endl;
            cerr << "Maybe it doesn't exist or you don't have proper "
                << "read permissions." << endl;

            exit(-1);

        }  // End of 'try-catch' block

    }
    else {

        try {
            // Try to open default configuration file
            confReader.open(path);
        }
        catch (...) {

            cerr << "Problem opening default configuration file 'pppconf_my.txt'"
                << endl;
            cerr << "Maybe it doesn't exist or you don't have proper read "
                << "permissions. Try providing a configuration file with "
                << "option '-c'."
                << endl;

            exit(-1);

        }  // End of 'try-catch' block

    }  // End of 'if ( confFile.getCount() > 0 )'


       // If a given variable is not found in the provided section, then
       // 'confReader' will look for it in the 'DEFAULT' section.
    confReader.setFallback2Default(true);

    return true;
}
//
bool Autonomus::loadEphemeris()
{
    // Set flags to reject satellites with bad or absent positional
    // values or clocks
    SP3EphList.rejectBadPositions(true);
    SP3EphList.rejectBadClocks(true);
     
    list<string> files;
    string subdir = confReader.fetchListValue("EphemerisDir");
    auxiliary::getAllFiles(subdir, files);

    for (auto file : files)
    {
        // Try to load each ephemeris file
        try {

            SP3EphList.loadFile(file);
        }
        catch (FileMissingException& e) {
            // If file doesn't exist, issue a warning
            cerr << "SP3 file '" << file << "' doesn't exist or you don't "
                << "have permission to read it. Skipping it." << endl;

            return false;
        }
    }
    return true;
}
//reading clock
bool Autonomus::loadClocks()
{
    list<string> files;
    string subdir = confReader.fetchListValue("RinexClockDir");
    auxiliary::getAllFiles(subdir, files);

    for (auto file : files)
    {
        // Try to load each ephemeris file
        try {
            SP3EphList.loadRinexClockFile(file);
        }
        catch (FileMissingException& e) {
            // If file doesn't exist, issue a warning
            cerr << "Rinex clock file '" << file << "' doesn't exist or you don't "
                << "have permission to read it. Skipping it." << endl;
            return false;
        }
    }//   while ((ClkFile = confReader.fetchListValue("rinexClockFiles", station)) != "")
    return true;
}

bool  Autonomus:: loadIono()
{
    list<string> files;
    string subdir = confReader.fetchListValue("RinexNavFilesDir");
    auxiliary::getAllFiles(subdir, files);

    for (auto file : files)
    {
        try
        {
            IonoModel iMod;
            Rinex3NavStream rNavFile;
            Rinex3NavHeader rNavHeader;

            rNavFile.open(file.c_str(), std::ios::in);
            rNavFile >> rNavHeader;

            long week = rNavHeader.mapTimeCorr["GPUT"].refWeek;
            GPSWeekSecond gpsws = GPSWeekSecond(week,0 );
            CommonTime refTime = gpsws.convertToCommonTime();

            if (rNavHeader.valid & Rinex3NavHeader::validIonoCorrGPS)
            {
                // Extract the Alpha and Beta parameters from the header
                double* ionAlpha = rNavHeader.mapIonoCorr["GPSA"].param;
                double* ionBeta = rNavHeader.mapIonoCorr["GPSB"].param;

                // Feed the ionospheric model with the parameters
                iMod.setModel(ionAlpha, ionBeta);
            }
            else
            {
                cerr << "WARNING: Navigation file " << file
                    << " doesn't have valid ionospheric correction parameters." << endl;
            }

            ionoStore.addIonoModel(refTime, iMod);
        }
        catch (...)
        {
            cerr << "Problem opening file " << file << endl;
            cerr << "Maybe it doesn't exist or you don't have proper read "
                << "permissions." << endl;
            exit(-1);
        }
    }
    return true;
}
void Autonomus::process()
{
    int badSol = 0;
    solverLEO.maskEl = confReader.fetchListValueAsDouble("ElMask");
    solverLEO.maskSNR = confReader.fetchListValueAsInt("SNRmask");
    solverLEO.ionoType =  (PRIonoCorrType)confReader.fetchListValueAsInt("PRionoCorrType");
     
    cout << "mask El " << solverLEO.maskEl << endl;
    cout << "mask SNR " <<(int) solverLEO.maskSNR << endl;

    string subdir = confReader.fetchListValue("RinesObsDir");
    auxiliary::getAllFiles(subdir, rinesObsFiles);
    ofstream os;
    os.open("output.txt");
    for (auto obsFile : rinesObsFiles)
    {
        cout << obsFile << endl;

        try {

            //Input observation file stream
            Rinex3ObsStream rin;
            // Open Rinex observations file in read-only mode
            rin.open(obsFile, std::ios::in);

            rin.exceptions(ios::failbit);
            Rinex3ObsHeader roh;
            Rinex3ObsData rod;

            //read the header
            rin >> roh;

#pragma region init observables indexes

            int indexC1 = roh.getObsIndex(L1CCodeID);
            int indexCNoL1;
            try
            {
                indexCNoL1 = roh.getObsIndex(L1CNo);
            }
            catch (...)
            {
                cerr << "The observation file doesn't have L1 C/No" << endl;
                continue;
            }
            int indexP1;
            try
            {
                indexP1 = roh.getObsIndex(L1PCodeID);
                cout << "L1 PRange from " << L1PCodeID <<  endl;
            }
            catch (...)
            {
                cerr << "The observation file doesn't have L1 pseudoranges." << endl;
                continue;
            }

            int indexP2;
            try
            {
                indexP2 = roh.getObsIndex(L2CodeID);
                cout << "L1 PRange from " << L2CodeID <<  endl;
            }
            catch (...)
            {
                indexP2 = -1;
                if(solverLEO.ionoType==PRIonoCorrType::IF) 
                    cout << "The observation file doesn't have L2 pseudoranges" <<  endl;
                continue;
            }

#pragma endregion

            // Let's process all lines of observation data, one by one
            while (rin >> rod)
            {
                int GoodSats = 0;
                int res = 0;

                // prepare for iteration loop
                Vector<double> Resid;

                vector<SatID> prnVec;
                vector<double> rangeVec;
                vector<uchar> SNRs;
                vector<bool> UseSat;

                solverLEO.selectObservations(rod, indexC1, indexP2, indexCNoL1, prnVec, rangeVec, SNRs, true);
                solverLEO.Sol = 0.0;

                for (size_t i = 0; i < prnVec.size(); i++)
                {
                    if (SNRs[i] >= solverLEO.maskSNR)
                    {
                        UseSat.push_back(true);
                        GoodSats++;
                    }
                    else
                        UseSat.push_back(false);
                }
                os << setprecision(12) << static_cast<YDSTime> (rod.time) << " ";
                if (GoodSats > 4)
                {
                    Matrix<double> SVP(prnVec.size(), 4), Cov(4, 4);

                    solverLEO.prepare(rod.time, prnVec, rangeVec, SP3EphList,  UseSat, SVP);
                    res = solverLEO.solve(rod.time, SVP, UseSat, Cov, Resid, ionoStore);
                    os << res;
                
                }
                else
                    res = 0;

                os << solverLEO.printSolution(UseSat) << endl;
                if (res != 1) badSol++;

            }
            rin.close();
        }
        catch (Exception& e)
        {
            cerr << e << endl;
        }
        catch (...)
        {
            cerr << "Caught an unexpected exception." << endl;
        }

    }
    os.close();
    cout << "bad Solutions " << badSol<<endl;
}


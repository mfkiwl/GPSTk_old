#include"autonomus.h"

typedef  unsigned char uchar;

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

bool Autonomus::loadEphemeris()
{
    // Set flags to reject satellites with bad or absent positional
    // values or clocks
    SP3EphList.rejectBadPositions(true);
    SP3EphList.rejectBadClocks(true);
     
    string sp3File;
    while ((sp3File = confReader.fetchListValue("SP3List")) != "") {

        // Try to load each ephemeris file
        try {

            SP3EphList.loadFile(sp3File);
        }
        catch (FileMissingException& e) {
            // If file doesn't exist, issue a warning
            cerr << "SP3 file '" << sp3File << "' doesn't exist or you don't "
                << "have permission to read it. Skipping it." << endl;

            return false;
        }
    }
    return true;
}
//
bool Autonomus::loadClocks()
{
    //reading clock
    string ClkFile;
    while ((ClkFile = confReader.fetchListValue("rinexClockFiles" )) != "") 
    {
        // Try to load each ephemeris file
        try {
            SP3EphList.loadRinexClockFile(ClkFile);
        }
        catch (FileMissingException& e) {
            // If file doesn't exist, issue a warning
            cerr << "Rinex clock file '" << ClkFile << "' doesn't exist or you don't "
                << "have permission to read it. Skipping it." << endl;
            return false;
        }
    }//   while ((ClkFile = confReader.fetchListValue("rinexClockFiles", station)) != "")
    return true;
}

//
bool Autonomus::checkObsFile()
{
    // Enable exceptions
   // rin.exceptions(ios::failbit);
    // Try to open Rinex observations file
    try {
        string path = confReader("rinexObsFile");
        cout << path << endl;
        // Open Rinex observations file in read-only mode
        rin.open(path, std::ios::in);
        return true;
    }
    catch (...) {

        cerr << "Problem opening file '"
            << confReader.getValue("rinexObsFile")
            << "'." << endl;

        cerr << "Maybe it doesn't exist or you don't have "
            << "proper read permissions."
            << endl;

        // Close current Rinex observation stream
        rin.close();

        return false;

    }  // End of 'try-catch' block
}

bool  Autonomus:: loadIono()
{
	// Activate failbit to enable exceptions
    string ionoFile;
    while ((ionoFile = confReader.fetchListValue("IonoModelFiles")) != "")
    {
        try
        {
            IonoModel iMod;
            Rinex3NavStream rNavFile;
            Rinex3NavHeader rNavHeader;

            rNavFile.open(ionoFile.c_str(), std::ios::in);
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
                cerr << "WARNING: Navigation file " << ionoFile
                    << " doesn't have valid ionospheric correction parameters." << endl;
            }

            ionoStore.addIonoModel(refTime, iMod);
        }
        catch (...)
        {
            cerr << "Problem opening file " <<ionoFile << endl;
            cerr << "Maybe it doesn't exist or you don't have proper read "
                << "permissions." << endl;
            exit(-1);
        }
    }
    return true;
}
void Autonomus::process()
{
    solverLEO.maskEl = confReader.fetchListValueAsDouble("Elmask");
    solverLEO.maskSNR = confReader.fetchListValueAsInt("SNRmask");

    ofstream os;
    os.open("output.txt");

    try {

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
            exit(1);
        }
        int indexP1;
        try
        {
            indexP1 = roh.getObsIndex(L1PCodeID);
            cout << "L1 PRange from " << L1PCodeID << " was used" << endl;
        }
        catch (...)
        {
            cerr << "The observation file doesn't have P1 pseudoranges." << endl;
            exit(1);
        }

        int indexP2;
        try
        {
            indexP2 = roh.getObsIndex(L2CodeID);
            cout << "iono-free with " << L2CodeID << " was used" << endl;
        }
        catch (...)
        {
            indexP2 = -1;
        }

#pragma endregion

        // Let's process all lines of observation data, one by one
        while (rin >> rod)
        {
            vector<int> GoodIndexes;
            // prepare for iteration loop
            Vector<double> Resid;
            int iter = 0;
            vector<SatID> prnVec;
            vector<double> rangeVec;
            vector<uchar> SNRs;
            vector<bool> UseSat;

            solverLEO.selectObservable(rod, indexC1, indexP2, indexCNoL1, prnVec, rangeVec, SNRs, true);
            solverLEO.Sol = 0.0;
            solverLEO.Conv = DBL_MAX;

            for (size_t i = 0; i < prnVec.size(); i++)
            {
                if (SNRs[i] >= solverLEO.maskSNR)
                    UseSat.push_back(true);
                else
                    UseSat.push_back(false);
            }

            Matrix<double> SVP(prnVec.size(), 4), Cov(4, 4);

            solverLEO.prepare(rod.time, prnVec, rangeVec, SP3EphList, ionoStore, UseSat, SVP);
            solverLEO.ajustParameters(rod.time, SVP, UseSat, Cov, Resid, ionoStore, iter, true);

            Position pos(solverLEO.Sol(0), solverLEO.Sol(1), solverLEO.Sol(2));

            double RMS3D = 0;

            double variance = solverLEO.ps.variance();
            for (size_t i = 0; i < 3; i++)
                RMS3D += variance*Cov(i, i);
            RMS3D = sqrt(RMS3D);

            os << setprecision(12) << static_cast<YDSTime> (rod.time) << " " << pos[0] << " " << pos[1] << " " << pos[2] << " " << iter << " " << prnVec.size() << " " << sqrt(variance)/*solverLEO.ps.size() */<< " " << RMS3D << endl;
        }
    }
    catch (Exception& e)
    {
        cerr << e << endl;
    }
    catch (...)
    {
        cerr << "Caught an unexpected exception." << endl;
    }
    os.close();

}


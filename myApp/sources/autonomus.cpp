#include"autonomus.h"
#include<list>
#include <direct.h>
#include<windows.h>
Autonomus:: Autonomus(char* arg0, char * discr )
    :
    BasicFramework(arg0,
        discr),
    // Option initialization. "true" means a mandatory option
    confFile(CommandOption::stdType,
        'c',
        "conffile",
        " [-c|--conffile]    Name of configuration file ('config.txt' by default).",
        false),
    solverPR(new PRSolverLEO())

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
            confReader.open(path);

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
            confReader.open("config.txt");
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


       //get application dir
    char current_work_dir[_MAX_FNAME];
    _getcwd(current_work_dir, sizeof(current_work_dir));
    string s_dir(current_work_dir);
    //set generic files direcory 
    string subdir = confReader.fetchListValue("GenericFilesDir");
    genFilesDir = s_dir + "\\" + subdir + "\\";

       // If a given variable is not found in the provided section, then
       // 'confReader' will look for it in the 'DEFAULT' section.
    confReader.setFallback2Default(true);

    return true;
}

void Autonomus::swichToSpaceborn()
{
    delete solverPR;
    solverPR = new PRSolverLEO();
}
//
void Autonomus::swichToGroundBased(TropModel & tModel)
{
    delete solverPR;
    solverPR = new PRSolver(tModel);
}
//
bool Autonomus::loadEphemeris()
{
    // Set flags to reject satellites with bad or absent positional
    // values or clocks
    SP3EphList.clear();
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

void Autonomus::PRprocess()
{
    int badSol = 0;
    apprPos.clear();
    NeillTropModel NeillModel;
    bool isSpace = confReader.fetchListValueAsBoolean("IsSpaceborneRcv");
    
    if (isSpace)
        swichToSpaceborn();
    else
    {
        double xapp(confReader.fetchListValueAsDouble("nominalPosition"));
        double yapp(confReader.fetchListValueAsDouble("nominalPosition"));
        double zapp(confReader.fetchListValueAsDouble("nominalPosition"));
        Position apprPos(xapp, yapp, zapp);
        int doy = confReader.fetchListValueAsInt("doy");
        NeillModel = NeillTropModel(apprPos.getAltitude(),apprPos.getGeodeticLatitude(), doy);

        swichToGroundBased(NeillModel);
    }

    solverPR->maskEl = confReader.fetchListValueAsDouble("ElMask");
    solverPR->maskSNR = confReader.fetchListValueAsInt("SNRmask");
    solverPR->ionoType =  (PRIonoCorrType)confReader.fetchListValueAsInt("PRionoCorrType");
     
    cout << "mask El " << solverPR->maskEl << endl;
    cout << "mask SNR " <<(int) solverPR->maskSNR << endl;

    string subdir = confReader.fetchListValue("RinesObsDir");
    auxiliary::getAllFiles(subdir, rinesObsFiles);
    ofstream os;
    os.open("outputPR.txt");
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
                cout << "L1 C/No from " << L2CodeID << endl;
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
                cout << "L2 PRange from " << L2CodeID <<  endl;
            }
            catch (...)
            {
                indexP2 = -1;
                if(solverPR->ionoType==PRIonoCorrType::IF)
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

                solverPR->selectObservables(rod, indexC1, indexP2, indexCNoL1, prnVec, rangeVec, SNRs);
                solverPR->Sol = 0.0;

                for (size_t i = 0; i < prnVec.size(); i++)
                {
                    if (SNRs[i] >= solverPR->maskSNR)
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

                    solverPR->prepare(rod.time, prnVec, rangeVec, SP3EphList,  UseSat, SVP);
                    res = solverPR->solve(rod.time, SVP, UseSat, Cov, Resid, ionoStore);
                    os << res;
                
                }
                else
                    res = 1;

                os << solverPR->printSolution(UseSat) << endl;
                if (res == 0)
                {
                    Xvt xvt;
                    xvt.x = Triple(solverPR->Sol(0), solverPR->Sol(1), solverPR->Sol(2));
                    xvt.clkbias = solverPR->Sol(3);
                    apprPos.insert(pair<CommonTime, Xvt>(rod.time, xvt));
                }
                else
                {
                    badSol++;
                } 

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
}
//
void Autonomus::PPPprocess()
{
    //PRprocess();
    PPPprocess2();
}

bool Autonomus:: PPPprocess2()
{

    string stationName = confReader.fetchListValue("stationName");
    // Load station nominal position
    double xn(confReader.fetchListValueAsDouble("nominalPosition"));
    double yn(confReader.fetchListValueAsDouble("nominalPosition"));
    double zn(confReader.fetchListValueAsDouble("nominalPosition"));

    // The former peculiar code is possible because each time we
    // call a 'fetchListValue' method, it takes out the first element
    // and deletes it from the given variable list.
    Position nominalPos(xn, yn, zn);


    // Create a 'ProcessingList' object where we'll store
    // the processing objects in order
    ProcessingList pList;

    // This object will check that all required observables are present
    RequireObservables requireObs;
    requireObs.addRequiredType(TypeID::P2);
    requireObs.addRequiredType(TypeID::L1);
    requireObs.addRequiredType(TypeID::L2);

    // This object will check that code observations are within
    // reasonable limits
    SimpleFilter pObsFilter;
    pObsFilter.setFilteredType(TypeID::P2);

    requireObs.addRequiredType(TypeID::P1);
    pObsFilter.addFilteredType(TypeID::P1);

    // Add 'requireObs' to processing list (it is the first)
    pList.push_back(requireObs);

    // IMPORTANT NOTE:
    // It turns out that some receivers don't correct their clocks
    // from drift.
    // When this happens, their code observations may drift well beyond
    // what it is usually expected from a pseudorange. In turn, this
    // effect causes that "SimpleFilter" objects start to reject a lot of
    // satellites.
    // Thence, the "filterCode" option allows you to deactivate the
    // "SimpleFilter" object that filters out C1, P1 and P2, in case you
    // need to.
    bool filterCode(confReader.getValueAsBoolean("filterCode"));

    // Check if we are going to use this "SimpleFilter" object or not
    if (filterCode)
    {
        pList.push_back(pObsFilter);       // Add to processing list
    }

    // This object defines several handy linear combinations
    LinearCombinations comb;

    // Object to compute linear combinations for cycle slip detection
    ComputeLinear linear1;

        linear1.addLinear(comb.pdeltaCombination);
        linear1.addLinear(comb.mwubbenaCombination);
    
    linear1.addLinear(comb.ldeltaCombination);
    linear1.addLinear(comb.liCombination);
    pList.push_back(linear1);       // Add to processing list

                                    // Objects to mark cycle slips
    LICSDetector2 markCSLI2;         // Checks LI cycle slips
    pList.push_back(markCSLI2);      // Add to processing list
    MWCSDetector  markCSMW;          // Checks Merbourne-Wubbena cycle slips
    pList.push_back(markCSMW);       // Add to processing list

                                     // Object to keep track of satellite arcs
    SatArcMarker markArc;
    markArc.setDeleteUnstableSats(true);
    markArc.setUnstablePeriod(151.0);
    pList.push_back(markArc);       // Add to processing list


                                    // Object to decimate data
    double newSampling(confReader.getValueAsDouble("decimationInterval"));

    Decimate decimateData(
        newSampling,
        confReader.getValueAsDouble("decimationTolerance"),
        SP3EphList.getInitialTime());
    pList.push_back(decimateData);       // Add to processing list

                                         // Declare a basic modeler
    BasicModel basic(nominalPos, SP3EphList);

    // Set the minimum elevation
    basic.setMinElev(confReader.getValueAsDouble("ElMask"));


    basic.setDefaultObservable(TypeID::P1);


    // Add to processing list
    pList.push_back(basic);

    // Object to remove eclipsed satellites
    EclipsedSatFilter eclipsedSV;
    pList.push_back(eclipsedSV);       // Add to processing list


                                       // Object to compute gravitational delay effects
    GravitationalDelay grDelay(nominalPos);
    pList.push_back(grDelay);       // Add to processing list


                                    // Vector from monument to antenna ARP [UEN], in meters
    double uARP(confReader.fetchListValueAsDouble("offsetARP"));
    double eARP(confReader.fetchListValueAsDouble("offsetARP"));
    double nARP(confReader.fetchListValueAsDouble("offsetARP"));
    Triple offsetARP(uARP, eARP, nARP);


    // Declare some antenna-related variables
    Triple offsetL1(0.0, 0.0, 0.0), offsetL2(0.0, 0.0, 0.0);
    AntexReader antexReader;
    Antenna receiverAntenna;

        // Feed Antex reader object with Antex file
    string afile = genFilesDir;
    afile += confReader.getValue("antexFile");

        antexReader.open(afile);

        // Get receiver antenna parameters
        receiverAntenna =
            antexReader.getAntenna(confReader.getValue("antennaModel"));
 
    // Object to compute satellite antenna phase center effect
    ComputeSatPCenter svPcenter(nominalPos);

   // Feed 'ComputeSatPCenter' object with 'AntexReader' object
   svPcenter.setAntexReader(antexReader);
    

    pList.push_back(svPcenter);       // Add to processing list

                                      // Declare an object to correct observables to monument
    CorrectObservables corr(SP3EphList);

    corr.setMonument(offsetARP);

    // Check if we want to use Antex patterns
    bool usepatterns(confReader.getValueAsBoolean("usePCPatterns"));
    if (usepatterns)
    {
        corr.setAntenna(receiverAntenna);

        // Should we use elevation/azimuth patterns or just elevation?
        corr.setUseAzimuth(confReader.getValueAsBoolean("useAzim"));
    }
   

    pList.push_back(corr);       // Add to processing list

                                 // Object to compute wind-up effect
    ComputeWindUp windup(SP3EphList,
                         nominalPos,
                         genFilesDir +confReader.getValue("satDataFile"));
    pList.push_back(windup);       // Add to processing list


                                   // Declare a NeillTropModel object, setting its parameters
    NeillTropModel neillTM(nominalPos.getAltitude(),
                           nominalPos.getGeodeticLatitude(),
                           confReader.getValueAsInt("dayOfYear"));

    // We will need this value later for printing
    double drytropo(neillTM.dry_zenith_delay());

    // Object to compute the tropospheric data
    ComputeTropModel computeTropo(neillTM);
    pList.push_back(computeTropo);       // Add to processing list

                                         // Object to compute ionosphere-free combinations to be used
                                         // as observables in the PPP processing
    ComputeLinear linear2;
    linear2.addLinear(comb.pcCombination);
    linear2.addLinear(comb.lcCombination);
    pList.push_back(linear2);       // Add to processing list

                                    // Declare a simple filter object to screen PC
    SimpleFilter pcFilter;
    pcFilter.setFilteredType(TypeID::PC);

    // IMPORTANT NOTE:
    // Like in the "filterCode" case, the "filterPC" option allows you to
    // deactivate the "SimpleFilter" object that filters out PC, in case
    // you need to.
    bool filterPC(true);

    // Check if we are going to use this "SimpleFilter" object or not
    if (filterPC)
    {
        pList.push_back(pcFilter);       // Add to processing list
    }

    // Object to align phase with code measurements
    PhaseCodeAlignment phaseAlign;
    pList.push_back(phaseAlign);       // Add to processing list


                                       // Object to compute prefit-residuals
    ComputeLinear linear3(comb.pcPrefit);
    linear3.addLinear(comb.lcPrefit);
    pList.push_back(linear3);       // Add to processing list

                                    // Declare a base-changing object: From ECEF to North-East-Up (NEU)
    XYZ2NEU baseChange(nominalPos);
    // We always need both ECEF and NEU data for 'ComputeDOP', so add this
    pList.push_back(baseChange);

    // Object to compute DOP values
    ComputeDOP cDOP;
    pList.push_back(cDOP);       // Add to processing list

                                 // Get if we want results in ECEF or NEU reference system
    bool isNEU(false);

    // Declare solver objects
    SolverPPP   pppSolver(isNEU);
    SolverPPPFB fbpppSolver(isNEU);

    // Get if we want 'forwards-backwards' or 'forwards' processing only
    int cycles(confReader.getValueAsInt("forwardBackwardCycles"));

    // Get if we want to process coordinates as white noise
    bool isWN(true);

    // White noise stochastic model
    WhiteNoiseModel wnM(100.0);      // 100 m of sigma
    
    // Decide what type of solver we will use for this station
    if (cycles > 0)
    {
        // In this case, we will use the 'forwards-backwards' solver

        // Check about coordinates as white noise
        if (isWN)
        {
            // Reconfigure solver
            fbpppSolver.setCoordinatesModel(&wnM);
        }

        // Add solver to processing list
        pList.push_back(fbpppSolver);
    }
    else
    {

        // In this case, we will use the 'forwards-only' solver

        // Check about coordinates as white noise
        if (isWN)
        {
            // Reconfigure solver
            pppSolver.setCoordinatesModel(&wnM);
        }

        // Add solver to processing list
        pList.push_back(pppSolver);

    }  // End of 'if ( cycles > 0 )'


    // Object to compute tidal effects
    #pragma region Tides
    SolidTides solid;

    // Configure ocean loading model
    OceanLoading ocean;
    ocean.setFilename(genFilesDir+ confReader.getValue("oceanLoadingFile"));

    // Numerical values (xp, yp) are pole displacements (arcsec).
    double xp(confReader.fetchListValueAsDouble("poleDisplacements"));
    double yp(confReader.fetchListValueAsDouble("poleDisplacements"));
    // Object to model pole tides
    PoleTides pole;
    pole.setXY(xp, yp);

#pragma endregion

    // This is the GNSS data structure that will hold all the
    // GNSS-related information
    gnssRinex gRin;

    #pragma region Output strams

    // Prepare for printing
    int precision(4);

    ofstream outfile;
    outfile.open("ppp_out.txt", ios::out);

    // Let's check if we are going to print the model
    bool printmodel(confReader.getValueAsBoolean("printModel"));

    string modelName;
    ofstream modelfile;

    // Prepare for model printing
    if (printmodel)
    {
        modelName = confReader.getValue("modelFile");
        modelfile.open(modelName.c_str(), ios::out);
    }
    #pragma endregion

    //// *** Now comes the REAL forwards processing part *** ////
    for (auto obsFile : rinesObsFiles)
    {
        cout << obsFile << endl;
        //Input observation file stream
        Rinex3ObsStream rin;
        // Open Rinex observations file in read-only mode
        rin.open(obsFile, std::ios::in);

        rin.exceptions(ios::failbit);
        Rinex3ObsHeader roh;
        Rinex3ObsData rod;

        //read the header
        rin >> roh;

        // Loop over all data epochs
        while (rin >> gRin)
        {
            // Store current epoch
            CommonTime time(gRin.header.epoch);

            // Compute solid, oceanic and pole tides effects at this epoch
            Triple tides(solid.getSolidTide(time, nominalPos) + ocean.getOceanLoading(stationName, time) + pole.getPoleTide(time, nominalPos));

            // Update observable correction object with tides information
            corr.setExtraBiases(tides);
            corr.setNominalPosition(nominalPos);

            try
            {
                // Let's process data. Thanks to 'ProcessingList' this is
                // very simple and compact: Just one line of code!!!.
                gRin >> pList;

            }
            catch (DecimateEpoch& d)
            {
                // If we catch a DecimateEpoch exception, just continue.
                return false;;
            }
            catch (Exception& e)
            {
                cerr << "Exception for receiver '" << stationName <<
                    "' at epoch: " << time << "; " << e << endl;
                return false;;
            }
            catch (...)
            {
                cerr << "Unknown exception for receiver '" << stationName <<
                    " at epoch: " << time << endl;
                return false;;
            }




            // Check what type of solver we are using
            if (cycles < 1)
            {

                // This is a 'forwards-only' filter. Let's print to output
                // file the results of this epoch
                //           printSolution(outfile,
                //               pppSolver,
                //time0,
                //               time,
                //               cDOP,
                //               isNEU,
                //               gRin.numSats(),
                //               drytropo,
                //stats,
                //               precision);

            }  // End of 'if ( cycles < 1 )'

               // The given epoch hass been processed. Let's get the next one

               // Ask if we are going to print the model
            if (printmodel)
            {
                printModel(modelfile, gRin,4);
            }
        }  // End of 'while(rin >> gRin)'

        rin.close();
    }

       //print statistic
       //printStats(outfile, stats);
       // Close current Rinex observation stream



    // If we printed the model, we must close the file
    if (printmodel)
    {
        // Close model file for this station
        modelfile.close();
    }

    //// *** Forwards processing part is over *** ////


    // Now decide what to do: If solver was a 'forwards-only' version,
    // then we are done and should continue with next station.
    if (cycles < 1)
    {

        // Close output file for this station
        outfile.close();

        // We are done with this station. Let's show a message
        cout << "Processing finished for station: '" << stationName << endl;

        // Go process next station
        return true;;

    }

    //// *** If we got here, it is a 'forwards-backwards' solver *** ////

    int i_c = 0;
    // Now, let's do 'forwards-backwards' cycles
    try
    {
        cout << "cycle # " << ++i_c << endl;
        fbpppSolver.ReProcess(cycles);

    }
    catch (Exception& e)
    {

        // If problems arose, issue an message and skip receiver
        cerr << "Exception at reprocessing phase: " << e << endl;
        cerr << "Skipping receiver '" << stationName << "'." << endl;

        // Close output file for this station
        outfile.close();

        // Go process next station
        return false;

    }  // End of 'try-catch' block

       //statistics for coorinates and tropo delay
    vector<PowerSum> stats(4);
    CommonTime time0;
    bool b = true;
    // Reprocess is over. Let's finish with the last processing		
    // Loop over all data epochs, again, and print results
    while (fbpppSolver.LastProcess(gRin))
    {

        CommonTime time(gRin.header.epoch);
        if (b)
        {
            time0 = time;

            b = false;
        }
        printSolution(outfile,
                      fbpppSolver,
                      time0,
                      time,
                      cDOP,
                      isNEU,
                      gRin.numSats(),
                      drytropo,
                      stats,
                      precision
        );

    }  // End of 'while( fbpppSolver.LastProcess(gRin) )'

       //print statistic
    printStats(outfile, stats);

    // We are done. Close and go for next station

    // Close output file for this station
    outfile.close();

}
// Method to print model values
void Autonomus::printModel(ofstream& modelfile,
                           const gnssRinex& gData,
                           int   precision)
{
    // Prepare for printing
    modelfile << fixed << setprecision(precision);

    // Get epoch out of GDS
    CommonTime time(gData.header.epoch);

    // Iterate through the GNSS Data Structure
    for (satTypeValueMap::const_iterator it = gData.body.begin();
         it != gData.body.end();
         it++)
    {

        // Print epoch
        modelfile << static_cast<YDSTime>(time).year << "  ";    // Year          #1
        modelfile << static_cast<YDSTime>(time).doy << "  ";    // DayOfYear      #2
        modelfile << static_cast<YDSTime>(time).sod << "  ";    // SecondsOfDay   #3

                                                                // Print satellite information (Satellite system and ID number)
        modelfile << (*it).first << " ";             // System         #4
                                                     // ID number      #5

                                                     // Print model values
        for (typeValueMap::const_iterator itObs = (*it).second.begin();
             itObs != (*it).second.end();
             itObs++)
        {
            // Print type names and values
            modelfile << (*itObs).first << " ";
            modelfile << (*itObs).second << " ";

        }  // End of 'for( typeValueMap::const_iterator itObs = ...'

        modelfile << endl;

    }
}

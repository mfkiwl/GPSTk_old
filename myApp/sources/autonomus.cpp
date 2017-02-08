#include"autonomus.h"
#include<list>
#include <direct.h>
#include<windows.h>
#include<regex>
#include"PPPSolverLEO.h"
#define DBG = 0
//
void Autonomus::PRprocess()
{
    //
    NeillTropModel  NeillModel;
    if (!isSpace)
    {
        NeillModel = NeillTropModel(nominalPos.getAltitude(), nominalPos.getGeodeticLatitude(), DoY);
        swichToGroundBased(NeillModel);
    }

    int badSol = 0;
    apprPos.clear();

    solverPR->maskEl = maskEl;
    solverPR->maskSNR = maskSNR;
    solverPR->ionoType =  (PRIonoCorrType)confReader.fetchListValueAsInt("PRionoCorrType");

    ofstream os;
    os.open("solutionPR.out");
    for (auto obsFile : rinexObsFiles)
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
                cout << "L1 C/No from " << L1CNo << endl;
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
                os << setprecision(17) <<  (rod.time) << " ";
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
        cout << "Number of bad solutions " << badSol << endl;
    }
}
//
void Autonomus::PPPprocess()
{
    initProcess();

#ifdef DBG 

    cout << "Approximate Positions loading... ";
    cout << loadApprPos("nomPos.in") << endl;
#else
    PRprocess();
#endif
 
    if (isSpace)
        PPPprocessLEO();
    else
        PPPprocessGB();
}

bool Autonomus:: PPPprocessGB()
{
    string stationName = confReader.fetchListValue("stationName");

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
    //BasicModel basic(Position(0.0, 0.0, 0.0), SP3EphList);
    BasicModel basic(nominalPos, SP3EphList);
    // Set the minimum elevation
    basic.setMinElev(maskEl);
    
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
   
    pList.push_back(corr);

    // Object to compute wind-up effect
    ComputeWindUp windup(SP3EphList, nominalPos, genFilesDir +confReader.getValue("satDataFile"));
    pList.push_back(windup);       // Add to processing list


                                   // Declare a NeillTropModel object, setting its parameters
    NeillTropModel neillTM(nominalPos.getAltitude(), nominalPos.getGeodeticLatitude(), DoY);

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
    pList.push_back(linear2);      
    
    // Add to processing list
    // Declare a simple filter object to screen PC
    SimpleFilter pcFilter;
    pcFilter.setFilteredType(TypeID::PC);

    // IMPORTANT NOTE:
    // Like in the "filterCode" case, the "filterPC" option allows you to
    // deactivate the "SimpleFilter" object that filters out PC, in case
    // you need to.

    pList.push_back(pcFilter);       // Add to processing list
    

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
    pList.push_back(cDOP);

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
    outfile.open("PPP_sol.out", ios::out);

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

    //statistics for coorinates and tropo delay
    vector<PowerSum> stats(4);
    CommonTime time0;
    bool b = true;

    //// *** Now comes the REAL forwards processing part *** ////
    for (auto obsFile : rinexObsFiles)
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
            nominalPos = apprPos.at(time);

            basic.rxPos = nominalPos;
            grDelay.setNominalPosition(nominalPos);
            svPcenter.setNominalPosition(nominalPos);
            baseChange = XYZ2NEU(nominalPos);

            // Compute solid, oceanic and pole tides effects at this epoch
            Triple tides(solid.getSolidTide(time, nominalPos) + ocean.getOceanLoading(stationName, time) + pole.getPoleTide(time, nominalPos));

            // Update observable correction object with tides information
            corr.setExtraBiases(tides);
            corr.setNominalPosition(nominalPos);

            try
            {
                gRin >> requireObs
                    >> pObsFilter
                    >> linear1
                    >> markCSLI2
                    >> markCSMW
                    >> markArc
                    >> decimateData
                    >> basic
                    >> eclipsedSV
                    >> grDelay
                    >> svPcenter
                    >> corr
                    >> windup
                    >> computeTropo
                    >> linear2
                    >> pcFilter
                    >> phaseAlign
                    >> linear3
                    >> baseChange
                    >> cDOP
                    >> pppSolver;
                // Let's process data. Thanks to 'ProcessingList' this is
                // very simple and compact: Just one line of code!!!.
               // gRin >> pList;

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

                CommonTime time(gRin.header.epoch);
                if (b)
                {
                    time0 = time;

                    b = false;
                }
                // This is a 'forwards-only' filter. Let's print to output
                // file the results of this epoch
                           printSolution(outfile,
                               pppSolver,
                               time0,
                               time,
                               cDOP,
                               isNEU,
                               gRin.numSats(),
                               drytropo,
                               stats,
                               precision,
                               nominalPos);

            }  // End of 'if ( cycles < 1 )'

               // Ask if we are going to print the model
            if (printmodel)
            {
                printModel(modelfile, gRin, 4);
            }
        }  // End of 'while(rin >> gRin)'

        rin.close();
    }

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
        return true;
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
        nominalPos = apprPos[time];
        printSolution(outfile,
                      fbpppSolver,
                      time0,
                      time,
                      cDOP,
                      isNEU,
                      gRin.numSats(),
                      drytropo,
                      stats,
                      precision,
                      nominalPos
        );

    }  // End of 'while( fbpppSolver.LastProcess(gRin) )'

       //print statistic
    printStats(outfile, stats);

    // We are done. Close and go for next station

    // Close output file for this station
    outfile.close();

}

bool Autonomus::PPPprocessLEO()
{
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


    // This object defines several handy linear combinations
    LinearCombinations comb;

    // Object to compute linear combinations for cycle slip detection
    ComputeLinear linear1;

    linear1.addLinear(comb.pdeltaCombination);
    linear1.addLinear(comb.mwubbenaCombination);

    linear1.addLinear(comb.ldeltaCombination);
    linear1.addLinear(comb.liCombination);

                                    // Objects to mark cycle slips
    LICSDetector2 markCSLI2;         // Checks LI cycle slips
    MWCSDetector  markCSMW;          // Checks Merbourne-Wubbena cycle slips

                                     // Object to keep track of satellite arcs
    SatArcMarker markArc;
    markArc.setDeleteUnstableSats(true);
    markArc.setUnstablePeriod(151.0);

    // Object to decimate data
    double newSampling(confReader.getValueAsDouble("decimationInterval"));

    Decimate decimateData(
        newSampling,
        confReader.getValueAsDouble("decimationTolerance"),
        SP3EphList.getInitialTime());
   
                                         // Declare a basic modeler
                                         //BasicModel basic(Position(0.0, 0.0, 0.0), SP3EphList);
    BasicModel basic(nominalPos, SP3EphList);
    // Set the minimum elevation
    basic.setMinElev(maskEl);

    basic.setDefaultObservable(TypeID::P1);

    // Object to remove eclipsed satellites
    EclipsedSatFilter eclipsedSV;

    // Object to compute gravitational delay effects
    GravitationalDelay grDelay(nominalPos);

    // Vector from monument to antenna ARP [UEN], in meters
    double uARP(confReader.fetchListValueAsDouble("offsetARP"));
    double eARP(confReader.fetchListValueAsDouble("offsetARP"));
    double nARP(confReader.fetchListValueAsDouble("offsetARP"));
    Triple offsetARP(uARP, eARP, nARP);

    AntexReader antexReader;
    Antenna receiverAntenna;

    // Feed Antex reader object with Antex file
    string afile = genFilesDir;
    afile += confReader.getValue("antexFile");

    antexReader.open(afile);

    // Get receiver antenna parameters
    receiverAntenna =
        antexReader.getAntenna(confReader.getValue("antennaModel"));
    cout << "antenna loading succes!!!\n";
    // Object to compute satellite antenna phase center effect
    ComputeSatPCenter svPcenter(nominalPos);

    // Feed 'ComputeSatPCenter' object with 'AntexReader' object
    svPcenter.setAntexReader(antexReader);

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


    // Object to compute wind-up effect
    ComputeWindUp windup(SP3EphList, nominalPos, genFilesDir + confReader.getValue("satDataFile"));


    // Object to compute ionosphere-free combinations to be used
    // as observables in the PPP processing
    ComputeLinear linear2;
    linear2.addLinear(comb.pcCombination);
    linear2.addLinear(comb.lcCombination);

    // Add to processing list
    // Declare a simple filter object to screen PC
    SimpleFilter pcFilter;
    pcFilter.setFilteredType(TypeID::PC);

    // IMPORTANT NOTE:
    // Like in the "filterCode" case, the "filterPC" option allows you to
    // deactivate the "SimpleFilter" object that filters out PC, in case
    // you need to.

    // Object to align phase with code measurements
    PhaseCodeAlignment phaseAlign;


     // Object to compute prefit-residuals
    ComputeLinear linear3(comb.pcPrefit);
    linear3.addLinear(comb.lcPrefit);

    // Declare a base-changing object: From ECEF to North-East-Up (NEU)
    XYZ2NEU baseChange(nominalPos);

    // Object to compute DOP values
    ComputeDOP cDOP;

    // Get if we want results in ECEF or NEU reference system
    bool isNEU(false);

    // White noise stochastic model
    WhiteNoiseModel wnM(100.0);      // 100 m of sigma
    // Declare solver objects
    PPPSolverLEO   pppSolver(isNEU);
    pppSolver.setCoordinatesModel(&wnM);
    SolverPPPFB fbpppSolver(isNEU);
    fbpppSolver.setCoordinatesModel(&wnM);

    // Get if we want 'forwards-backwards' or 'forwards' processing only
    int cycles(confReader.getValueAsInt("forwardBackwardCycles"));
    
    // This is the GNSS data structure that will hold all the
    // GNSS-related information
    gnssRinex gRin;

#pragma region Output strams

    // Prepare for printing
    int precision(4);

    ofstream outfile;
    outfile.open("PPP_sol.out", ios::out);

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

    //statistics for coorinates and tropo delay
    vector<PowerSum> stats(4);
    CommonTime time0;
    bool b = true;

    //// *** Now comes the REAL forwards processing part *** ////
    for (auto obsFile : rinexObsFiles)
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
        auto it = apprPos.begin();
        rin >> roh;

        // Loop over all data epochs
        while (rin >> gRin)
        {
            // Store current epoch
            CommonTime time(gRin.header.epoch);
#ifdef DBG
            nominalPos = it->second;
            it++;
#else
            nominalPos = apprPos.at(time);
#endif // DEBUG

            

            basic.rxPos = nominalPos;
            grDelay.setNominalPosition(nominalPos);
            svPcenter.setNominalPosition(nominalPos);
            baseChange = XYZ2NEU(nominalPos);

            corr.setNominalPosition(nominalPos);

            try
            {
                gRin >> requireObs
                    >> pObsFilter
                    >> linear1
                    >> markCSLI2
                    >> markCSMW
                    >> markArc
                    >> decimateData
                    >> basic
                    >> eclipsedSV
                    >> grDelay
                    >> svPcenter
                    >> corr
                    >> windup
                    >> linear2
                    >> pcFilter
                    >> phaseAlign
                    >> linear3
                    >> baseChange
                    >> cDOP
                    >> pppSolver;
                // Let's process data. Thanks to 'ProcessingList' this is
                // very simple and compact: Just one line of code!!!.
                // gRin >> pList;

            }
            catch (DecimateEpoch& d)
            {
                // If we catch a DecimateEpoch exception, just continue.
                return false;
            }
            catch (Exception& e)
            {

                return false;
            }
            catch (...)
            {
                return false;
            }

            // Check what type of solver we are using
            if (cycles < 1)
            {

                CommonTime time(gRin.header.epoch);
                if (b)
                {
                    time0 = time;

                    b = false;
                }
                // This is a 'forwards-only' filter. Let's print to output
                // file the results of this epoch
                printSolutionLEO(outfile,
                              pppSolver,
                              time0,
                              time,
                              cDOP,
                              isNEU,
                              gRin.numSats(),
                              stats,
                              precision,
                              nominalPos);

            }  // End of 'if ( cycles < 1 )'

               // The given epoch hass been processed. Let's get the next one

               // Ask if we are going to print the model
            if (printmodel)
            {
                printModel(modelfile, gRin, 4);
            }
        }  // End of 'while(rin >> gRin)'

        rin.close();
    }

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

        // Go process next station
        return true;
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

        // Close output file for this station
        outfile.close();

       return false;

    } 

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
        nominalPos = apprPos[time];
        printSolutionLEO(outfile,
                      fbpppSolver,
                      time0,
                      time,
                      cDOP,
                      isNEU,
                      gRin.numSats(),
                      stats,
                      precision,
                      nominalPos
        );

    }  // End of 'while( fbpppSolver.LastProcess(gRin) )'

       //print statistic
    printStats(outfile, stats);

    // We are done. Close and go for next station

    // Close output file for this station
    outfile.close();
}

bool Autonomus::loadApprPos(std::string path)
{
    apprPos.clear();
    try
    {
        ifstream file(path);
        if (file.is_open())
        {
            long JD(0), msec(0);
            double fmsec(0.0),x(0.0),y(0.0),z(0.0),rco(0.0);
            int solType(0);
            string sTS;

            string line;
            while (file >> JD >> msec >> fmsec >> sTS >> solType >> x >> y >> z>> rco)
            {
                if (!solType)
                {
                    CommonTime time;
                    time.setInternal(JD, msec, fmsec, TimeSystem::GPS);
                    //
                    Xvt xvt;
                    xvt.x = Triple(x, y, z);
                    xvt.clkbias = rco;
                    apprPos.insert(pair<CommonTime, Xvt>(time, xvt));
                }
                string line;
                getline(file, line);
            }
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception& e)
    {
        cout << e.what()<<endl;
        return false;
    }
    return true;
}


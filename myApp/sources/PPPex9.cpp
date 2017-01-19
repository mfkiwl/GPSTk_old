#include"stdafx.h"
#include"actions.h"

int Actions::runEx9(int argc, char *argv[])
{


    try{

        ex9 program(argv[0]);

        // We are disabling 'pretty print' feature to keep
        // our description format
        if (!program.initialize(argc, argv, false)){
            return 0;
        }

        if (!program.run()){
            return 1;
        }

        return 0;

    }
    catch (Exception& e){

        cerr << "Problem: " << e << endl;

        return 1;

    }
    catch (...){

        cerr << "Unknown error." << endl;

        return 1;

    }

    return 0;
}

int Actions::CalcPPP( char * arg0)
{

    /////////////////// INITIALIZATION PART /////////////////////
    cout << "initialization started\n";

    ofstream output("PPP.out");
    //  ofstream dbg("dbg.out");
    output << fixed << setprecision(3);   // Set a proper output format
    string fname = "coco1360.16o";                                  // Create the input observation file stream
    RinexObsStream rin(fname);
    cout << fname << endl;
    Rinex3ObsHeader roh;
    rin >> roh;
    cout << "approx position " << roh.antennaBsightXYZ << endl;
    //  station nominal position
    Position nominalPos(-741950.3850, 6190961.6631, - 1337768.2263);

    // Declare an object to process the data using PPP. It is set
    // to use a NEU system
    SolverPPP pppSolver(true);

    // The real test for a PPP processing program is to handle coordinates
    // as white noise. In such case, position error should be about 0.25 m or
    // better. Uncomment the following couple of lines to test this.
    //WhiteNoiseModel wnM(100.0);            // 100 m of sigma
    //pppSolver.setCoordinatesModel(&wnM);
    pppSolver.setKinematic();
    // Declare a "SP3EphemerisStore" object to handle precise ephemeris
    SP3EphemerisStore rawSP3, SP3EphList;

    // Set flags to reject satellites with bad or absent positional
    // values or clocks
    //SP3EphList.rejectBadPositions(true);
    //SP3EphList.rejectBadClocks(true);

    rawSP3.useSP3ClockData();
    rawSP3.rejectBadPositions(true);
    rawSP3.rejectBadClocks(true);
    // CNQ- enableDataGapCheck and enableIntervalCheck are not SP3EphemerisStore member functions.
    //      // Set flags to check for data gaps and too wide interpolation intervals.
    //      // Default values for "gapInterval" (901.0 s) and "maxInterval"
    //      // (8105.0 s) will be used.
    //   SP3EphList.enableDataGapCheck();
    //   SP3EphList.enableIntervalCheck();

    //Load all the SP3 ephemerides files
    rawSP3.loadFile("COD18966.EPH");
    rawSP3.loadFile("COD18970.EPH");
    rawSP3.loadFile("COD18971.EPH");
    rawSP3.loadRinexClockFile("COD18966.CLK");
    rawSP3.loadRinexClockFile("COD18970.CLK");
    rawSP3.loadRinexClockFile("COD18971.CLK");
    cout << "Data Loaded\n";
    //SP3EphList.loadFile("COD18966.EPH");
    //SP3EphList.loadFile("COD18970.EPH");
    //SP3EphList.loadFile("COD18971.EPH");
    // SP3EphList = rawSP3;

    int deci = 30;

    // Declare a NeillTropModel object, setting the defaults
    NeillTropModel neillTM(nominalPos.getAltitude(),
        nominalPos.getGeodeticLatitude(), 136);

    // This is the GNSS data structure that will hold all the
    // GNSS-related information
    gnssRinex gRin;

    // Declare a base-changing object: From ECEF to North-East-Up (NEU)
    XYZ2NEU baseChange(nominalPos);

    // Declare an object to check that all required observables are present
    RequireObservables requireObs(TypeID::P1);
    requireObs.addRequiredType(TypeID::P2);
    requireObs.addRequiredType(TypeID::L1);
    requireObs.addRequiredType(TypeID::L2);

    // Declare a simple filter object to screen PC
    SimpleFilter pcFilter;
    pcFilter.setFilteredType(TypeID::MWubbena);

    // Declare a basic modeler
    BasicModel basic(nominalPos, SP3EphList);
    
    // Objects to mark cycle slips
    LICSDetector2 markCSLI;     // Checks LI cycle slips
    MWCSDetector markCSMW;      // Checks Merbourne-Wubbena cycle slips


    // Object to compute tidal effects
    SolidTides  solid;

    // Ocean loading model
    OceanLoading ocean("JUN2016.BLQ");

    // Numerical values are x,y pole displacements for May/15/2016 (arcsec).
    PoleTides   pole(0.060489, 0.488845);

    // Vector from ONSA antenna ARP to L1 phase center [UEN] (AOAD/M_B)
    Triple offsetL1(0.110, 0.000, 0.000);   // Units in meters

    // Vector from ONSA antenna ARP to L2 phase center [UEN] (AOAD/M_B)
    Triple offsetL2(0.128, 0.0000, 0.000);  // Units in meters

    // Vector from monument to antenna ARP [UEN] for ONSA station
    Triple offsetARP(0.004, 0.0, 0.0);    // Units in meters


    // Declare an object to correct observables to monument
    CorrectObservables corr(SP3EphList);
    ((corr.setNominalPosition(nominalPos)).setL1pc(offsetL1)).setL2pc(offsetL2);
    corr.setMonument(offsetARP);


    // Object to compute wind-up effect
    ComputeWindUp windup(SP3EphList, nominalPos, "PRN_GPS");


    // Object to compute satellite antenna phase center effect
    ComputeSatPCenter svPcenter(nominalPos);


    // Object to compute the tropospheric data
    ComputeTropModel computeTropo(neillTM);


    // This object defines several handy linear combinations
    LinearCombinations comb;


    // Object to compute linear combinations for cycle slip detection
    ComputeLinear linear1(comb.pdeltaCombination);
    linear1.addLinear(comb.ldeltaCombination);
    linear1.addLinear(comb.mwubbenaCombination);
    linear1.addLinear(comb.liCombination);


    // This object computes the ionosphere-free combinations to be used
    // as observables in the PPP processing
    ComputeLinear linear2(comb.pcCombination);
    linear2.addLinear(comb.lcCombination);


    // Let's use a different object to compute prefit residuals
    ComputeLinear linear3(comb.pcPrefit);
    linear3.addLinear(comb.lcPrefit);

    // Object to keep track of satellite arcs
    SatArcMarker markArc;
    markArc.setDeleteUnstableSats(true);
    markArc.setUnstablePeriod(31);

    // Object to compute gravitational delay effects
    GravitationalDelay grDelay(nominalPos);

    // Object to align phase with code measurements
    PhaseCodeAlignment phaseAlign;

    // Object to compute DOP values
    ComputeDOP cDOP;

    // Object to remove eclipsed satellites
    EclipsedSatFilter eclipsedSV;

    // Object to decimate data. This is important because RINEX observation
    // data is provided with a 30 s sample rate, whereas SP3 files provide
    // satellite clock information with a 900 s sample rate.
    Decimate decimateData(deci, 1.0, SP3EphList.getInitialTime());

    // When you are printing the model, you may want to comment the previous
    // line and uncomment the following one, generating a 30 s model
      // Decimate decimateData(30.0, 1.0, SP3EphList.getInitialTime());

    // Statistical summary objects
    PowerSum errorVectorStats;



    /////////////////// PROCESING PART /////////////////////


    // Use this variable to select between position printing or model printing
    bool printPosition(true);     // By default, print position and associated
                                  // parameters


    cout << "processing started\n";
    // Loop over all data epochs
    while (rin >> gRin){

        CommonTime time(gRin.header.epoch);

        // Compute the effect of solid, oceanic and pole tides
        Triple tides(solid.getSolidTide(time, nominalPos) +
            ocean.getOceanLoading("COCO", time) +
            pole.getPoleTide(time, nominalPos));

        // Update observable correction object with tides information
        corr.setExtraBiases(tides);

        try{

            // The following lines are indeed just one line
            gRin >> requireObs      // Check if required observations are present
                >> linear1         // Compute linear combinations to detect CS
                >> markCSLI        // Mark cycle slips: LI algorithm
                >> markCSMW        // Mark cycle slips: Melbourne-Wubbena
                >> markArc         // Keep track of satellite arcs
                >> decimateData    // If not a multiple of 900 s, decimate
                >> basic           // Compute the basic components of model
                >> eclipsedSV      // Remove satellites in eclipse
                >> grDelay         // Compute gravitational delay
                >> svPcenter       // Compute the effect of satellite phase center
                >> corr            // Correct observables from tides, etc.
                >> windup          // Compute wind-up effect
                >> computeTropo    // Compute tropospheric effect
                >> linear2         // Compute ionosphere-free combinations
                >> pcFilter        // Filter out spurious data
                >> phaseAlign      // Align phases with codes
                >> linear3         // Compute prefit residuals
                >> baseChange      // Prepare to use North-East-UP reference frame
                >> cDOP            // Compute DOP figures
                >> pppSolver;      // Solve equations with a Kalman filter

        }
        catch (DecimateEpoch& d){
            // If 'decimateData' object detects that this epoch is not a
            // multiple of 900 s, it issues a "DecimateEpoch" exception. Here
            // we catch such exception, and just continue to process next epoch.
            continue;
        }
        catch (Exception& e){
            cerr << "Exception at epoch: " << time << "; " << e << endl;
            continue;
        }
        catch (...){
            cerr << "Unknown exception at epoch: " << time << endl;
            continue;
        }


        // Check if we want to print model or position
        if (printPosition){
            // Print here the position results
            // Triple dX = Triple(pppSolver.getSolution(TypeID::dx), pppSolver.getSolution(TypeID::dy), pppSolver.getSolution(TypeID::dz));
            Triple dX = Triple(pppSolver.getSolution(TypeID::dLat), pppSolver.getSolution(TypeID::dLon), pppSolver.getSolution(TypeID::dH));

            output << static_cast<YDSTime>(time).sod << "  ";     // Epoch - Output field #1
            output << "dX " << dX[0] << "  ";    // dLat  - #2
            output << dX[1] << "  ";    // dLon  - #3
            output << dX[2] << "  ";      // dH    - #4
            Position pos = nominalPos + Position(dX);
            output << "H " << (nominalPos.height() + dX[2]) << " ";
            /* cout << "X " << pos[0]<<" " << pos[1] <<" "<< pos[2] << " ";
            cout <<"BLH "<< pos.geodeticLatitude() << " " << pos.longitude() << " " << pos.height() << " ";*/

            //cout << pppSolver.solution  /*+ nominalPos.getX()*/ << "  ";    // dLat  - #2
            //cout << pppSolver.solution[1] /*+ nominalPos.getY()*/ << "  ";    // dLon  - #3
            //         cout << pppSolver.solution[2] /*+ nominalPos.getZ()*/ << "  ";      // dH   - #4
            output << "Tropo_del " << pppSolver.getSolution(TypeID::wetMap) << "  ";  // Tropo - #5

                                                                                      //cout << pppSolver.getVariance(TypeID::dLat) << "  "; // Cov dLat - #6
                                                                                      //cout << pppSolver.getVariance(TypeID::dLon) << "  "; // Cov dLon - #7
                                                                                      //cout << pppSolver.getVariance(TypeID::dH) << "  ";   // Cov dH   - #8
                                                                                      //cout <<"Tropo_del "<< pppSolver.getVariance(TypeID::wetMap) << "  ";//Cov Tropo- #9

            output << gRin.numSats() << "  ";     // Visible satellites - #10


            output << endl;

            // For statistical purposes we discard the first two hours of data
            if (static_cast<YDSTime>(time).sod > 7200.0){
                // Statistical summary
                double errorV(pppSolver.solution[1] * pppSolver.solution[1] +
                    pppSolver.solution[2] * pppSolver.solution[2] +
                    pppSolver.solution[3] * pppSolver.solution[3]);

                // Get module of position error vector
                errorV = std::sqrt(errorV);

                // Add to statistical summary object
                errorVectorStats.add(errorV);
            }

        }  // End of position printing
        else
        {
            // Print here the model results
            // First, define types we want to keep
            TypeIDSet types;
            types.insert(TypeID::L1);
            types.insert(TypeID::L2);
            types.insert(TypeID::P1);
            types.insert(TypeID::P2);
            types.insert(TypeID::PC);
            types.insert(TypeID::LC);
            types.insert(TypeID::rho);
            types.insert(TypeID::dtSat);
            types.insert(TypeID::rel);
            types.insert(TypeID::gravDelay);
            types.insert(TypeID::tropo);
            types.insert(TypeID::dryTropo);
            types.insert(TypeID::dryMap);
            types.insert(TypeID::wetTropo);
            types.insert(TypeID::wetMap);
            types.insert(TypeID::tropoSlant);
            types.insert(TypeID::windUp);
            types.insert(TypeID::satPCenter);
            types.insert(TypeID::satX);
            types.insert(TypeID::satY);
            types.insert(TypeID::satZ);
            types.insert(TypeID::elevation);
            types.insert(TypeID::azimuth);
            types.insert(TypeID::satArc);
            types.insert(TypeID::prefitC);
            types.insert(TypeID::prefitL);
            types.insert(TypeID::dx);
            types.insert(TypeID::dy);
            types.insert(TypeID::dz);
            types.insert(TypeID::dLat);
            types.insert(TypeID::dLon);
            types.insert(TypeID::dH);
            types.insert(TypeID::cdt);

            gRin.keepOnlyTypeID(types);   // Delete the types not in 'types'

                                          // Iterate through the GNSS Data Structure
            satTypeValueMap::const_iterator it;
            for (it = gRin.body.begin(); it != gRin.body.end(); it++){

                // Print epoch
                cout << static_cast<YDSTime>(time).year << " ";
                cout << static_cast<YDSTime>(time).doy << " ";
                cout << static_cast<YDSTime>(time).sod << " ";



                                                 // Print satellite information (system and PRN)
                cout << (*it).first << " ";

                // Print model values
                typeValueMap::const_iterator itObs;
                for (itObs = (*it).second.begin();
                    itObs != (*it).second.end();
                    itObs++){
                    bool printNames(true);  // Whether print types' names or not
                    if (printNames){
                        cout << (*itObs).first << " ";
                    }

                    cout << (*itObs).second << " ";

                }  // End for( itObs = ... )

                cout << endl;

            }  // End for (it = gRin.body.begin(); ... )

        }  // End of model printing

    }  // End of 'while(rin >> gRin)...'

    cout << "processing finished\n";

    // Print statistical summary in cerr
    if (printPosition){
        cerr << "Module of error vector: Average = "
            << errorVectorStats.average() << " m    Std. dev. = "
            << std::sqrt(errorVectorStats.variance()) << " m" << endl;
    }

    output.close();
    //system("pause");
    exit(0);       // End of program

}

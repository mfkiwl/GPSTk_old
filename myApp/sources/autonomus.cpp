#include"autonomus.h"

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
    while ((ClkFile = confReader.fetchListValue("rinexClockFiles" )) != "") {

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

 void Autonomus::process()
{
     // Declaration of objects for storing ephemerides and handling RAIM
    
     PRSolution2 raimSolver;

     // Object for void-type tropospheric model (in case no meteorological
     // RINEX is available)
     ZeroTropModel noTropModel;

     // Object for GG-type tropospheric model (Goad and Goodman, 1974)
     // Default constructor => default values for model
     GGTropModel ggTropModel;

     // Pointer to one of the two available tropospheric models. It points
     // to the void model by default
     TropModel *tropModelPtr = &noTropModel;

     // This verifies the ammount of command-line parameters given and
   

     // Let's compute an useful constant (also found in "GNSSconstants.hpp")
     const double gamma = (L1_FREQ_GPS / L2_FREQ_GPS)*(L1_FREQ_GPS / L2_FREQ_GPS);

     char * L1CodeID = "C1W";
     char * L2CodeID = "C2W";
     try
     {
       // In order to throw exceptions, it is necessary to set the failbit
         rin.exceptions(ios::failbit);

         Rinex3ObsHeader roh;
         Rinex3ObsData rod;

         // Let's read the header
         rin >> roh;

         // The following lines fetch the corresponding indexes for some
         // observation types we are interested in. Given that old-style
         // observation types are used, GPS is assumed.
         int indexP1;
         try
         {
             indexP1 = roh.getObsIndex(L1CodeID);
             cout<< "L1 PRange from " << L1CodeID << " was used" << endl;
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
             cout << "iono-free with "<< L2CodeID<<" was used" << endl;
         }
         catch (...)
         {
             indexP2 = -1;
         }
        // raimSolver.Debug = true;

         // Let's process all lines of observation data, one by one
         while (rin >> rod)
         {

                // Apply editing criteria
             if (rod.epochFlag == 0 || rod.epochFlag == 1)  // Begin usable data
             {

                 vector<SatID> prnVec;
                 vector<double> rangeVec;

                 // Define the "it" iterator to visit the observations PRN map. 
                 // Rinex3ObsData::DataMap is a map from RinexSatID to
                 // vector<RinexDatum>:
                 //      std::map<RinexSatID, vector<RinexDatum> >
                 Rinex3ObsData::DataMap::const_iterator it;

                 // This part gets the PRN numbers and ionosphere-corrected
                 // pseudoranges for the current epoch. They are correspondly fed
                 // into "prnVec" and "rangeVec"; "obs" is a public attribute of
                 // Rinex3ObsData to get the map of observations
                 for (it = rod.obs.begin(); it != rod.obs.end(); it++)
                 {
                     // The RINEX file may have P1 observations, but the current
                     // satellite may not have them.
                     double P1(0.0);
                     try
                     {
                         P1 = rod.getObs((*it).first, indexP1).data;
                     }
                     catch (...)
                     {
                         // Ignore this satellite if P1 is not found
                         continue;
                     }

                     double ionocorr(0.0);

                     // If there are P2 observations, let's try to apply the
                     // ionospheric corrections
                     if (indexP2 >= 0)
                     {

                         // The RINEX file may have P2 observations, but the
                         // current satellite may not have them.
                         double P2(0.0);
                         try
                         {
                             P2 = rod.getObs((*it).first, indexP2).data;
                         }
                         catch (...)
                         {
                             // Ignore this satellite if P1 is not found
                             continue;
                         }

                         // Vector 'vecData' contains RinexDatum, whose public
                         // attribute "data" indeed holds the actual data point
                         ionocorr = 1.0 / (1.0 - gamma) * (P1 - P2);

                     }

                     // Now, we include the current PRN number in the first part
                     // of "it" iterator into the vector holding the satellites.
                     // All satellites in view at this epoch that have P1 or P1+P2
                     // observations will be included.
                     prnVec.push_back((*it).first);

                     // The same is done for the vector of doubles holding the
                     // corrected ranges
                     rangeVec.push_back(P1 - ionocorr);

                     // WARNING: Please note that so far no further correction
                     // is done on data: Relativistic effects, tropospheric
                     // correction, instrumental delays, etc.

                 }  // End of 'for( it = rod.obs.begin(); it!= rod.obs.end(); ...'

                    // The default constructor for PRSolution2 objects (like
                    // "raimSolver") is to set a RMSLimit of 6.5. We change that
                    // here. With this value of 3e6 the solution will have a lot
                    // more dispersion.
                 raimSolver.RMSLimit = 3e8;

                

                 // In order to compute positions we need the current time, the
                 // vector of visible satellites, the vector of corresponding
                 // ranges, the object containing satellite ephemerides, and a
                 // pointer to the tropospheric model to be applied
                 raimSolver.RAIMCompute(rod.time,
                     prnVec,
                     rangeVec,
                     SP3EphList,
                     tropModelPtr);

                 // Note: Given that the default constructor sets public
                 // attribute "Algebraic" to FALSE, a linearized least squares
                 // algorithm will be used to get the solutions.
                 // Also, the default constructor sets ResidualCriterion to true,
                 // so the rejection criterion is based on RMS residual of fit,
                 // instead of RMS distance from an a priori position.

                 // If we got a valid solution, let's print it
                 cout << setprecision(12) << static_cast<YDSTime> (rod.time) << " ";
                 if (raimSolver.isValid())
                 {
                     // Vector "Solution" holds the coordinates, expressed in
                     // meters in an Earth Centered, Earth Fixed (ECEF) reference
                     // frame. The order is x, y, z  (as all ECEF objects)
                    
                     cout << setprecision(12) << raimSolver.Solution[0] << " ";
                     cout << raimSolver.Solution[1] << " ";
                     cout << raimSolver.Solution[2];
                     cout << raimSolver.Solution[3];
                     cout << endl;

                 }  // End of 'if( raimSolver.isValid() )'
                 else                     {
                     cout << "0 0 0" << endl;
                 }
             } // End of 'if( rod.epochFlag == 0 || rod.epochFlag == 1 )'

         }  // End of 'while( roffs >> rod )'

     }
     catch (Exception& e)
     {
         cerr << e << endl;
     }
     catch (...)
     {
         cerr << "Caught an unexpected exception." << endl;
     }

     exit(0);

 }  // End of 'main()'


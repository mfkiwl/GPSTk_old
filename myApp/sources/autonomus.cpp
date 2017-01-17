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

bool  Autonomus:: loadIono()
{
	Rinex3NavStream rNavFile;

	// Activate failbit to enable exceptions
	rNavFile.exceptions(ios::failbit);

	// Read nav file and store unique list of ephemerides
	try
	{
		rNavFile.open(navFile.getValue()[0].c_str(), std::ios::in);
	}
	catch (...)
	{
		cerr << "Problem opening file " << navFile.getValue()[0].c_str() << endl;
		cerr << "Maybe it doesn't exist or you don't have proper read "
			<< "permissions." << endl;

		exit(-1);
	}

	// We will need to read ionospheric parameters (Klobuchar model) from
	// the file header
	rNavFile >> rNavHeader;

	// Let's feed the ionospheric model (Klobuchar type) from data in the
	// navigation (ephemeris) file header. First, we must check if there are
	// valid ionospheric correction parameters in the header
	if (rNavHeader.valid & Rinex3NavHeader::validIonoCorrGPS)
	{
		// Extract the Alpha and Beta parameters from the header
		double* ionAlpha = rNavHeader.mapIonoCorr["GPSA"].param;
		double* ionBeta = rNavHeader.mapIonoCorr["GPSB"].param;

		// Feed the ionospheric model with the parameters
		ioModel.setModel(ionAlpha, ionBeta);
	}
	else
	{
		cerr << "WARNING: Navigation file " << navFile.getValue()[0].c_str()
			<< " doesn't have valid ionospheric correction parameters." << endl;
	}

	// WARNING-WARNING-WARNING: In this case, the same model will be used
	// for the full data span
	ionoStore.addIonoModel(CommonTime::BEGINNING_OF_TIME, ioModel);
}

void Autonomus::process()
{

    ofstream os;
    os.open("output.txt");
    // Declaration of objects for storing ephemerides and handling RAIM

    PRSolution2 raimSolver;
    PRSolution2::isERCorr = false;

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

	try{
        // In order to throw exceptions, it is necessary to set the failbit
        rin.exceptions(ios::failbit);

        Rinex3ObsHeader roh;
        Rinex3ObsData rod;

        // Let's read the header
        rin >> roh;

        // The following lines fetch the corresponding indexes for some
        // observation types we are interested in. Given that old-style
        // observation types are used, GPS is assumed.
        int indexC1= roh.getObsIndex(L1CCodeID);
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
        try{
            indexP1 = roh.getObsIndex(L1PCodeID);
            cout << "L1 PRange from " << L1PCodeID << " was used" << endl;
        }
        catch (...){
            cerr << "The observation file doesn't have P1 pseudoranges." << endl;
            exit(1);
        }

        int indexP2;
        try{
            indexP2 = roh.getObsIndex(L2CodeID);
            cout << "iono-free with " << L2CodeID << " was used" << endl;
        }
        catch (...)
        {
            indexP2 = -1;
        }
        // raimSolver.Debug = true;

         // Let's process all lines of observation data, one by one
        while (rin >> rod)
        {
            Matrix<double> SVP, Cov;
            int N = 0;
            vector<bool> UseSat;
            vector<int> GoodIndexes;
            // prepare for iteration loop
            Vector<double>Resid, Slope, Sol = 0.0; // initial guess: center of earth
            int n_iterate = 10;
            double converge = 1e-3;
            double Conv = DBL_MAX;
            vector<SatID> prnVec;
            vector<double> rangeP2Vec;
			vector<double> rangeVec;
            vector<double> icorrVec;
            vector<uchar> CNoVec;

#pragma region Select Observable
            // Apply editing criteria
            if (rod.epochFlag == 0 || rod.epochFlag == 1)  // Begin usable data
            {
                Rinex3ObsData::DataMap::const_iterator it;

                for (it = rod.obs.begin(); it != rod.obs.end(); it++)
                {
                    double P1(0.0), C1(0.0);
                    char S1(0);
                    try
                    {
                        C1 = rod.getObs((*it).first, indexC1).data;
                        P1 = rod.getObs((*it).first, indexP1).data;
                        S1 = rod.getObs((*it).first, indexCNoL1).data;
                    }
                    catch (...)
                    {
                        // Ignore this satellite if P1 is not found
                        continue;
                    }

                    double ionocorr(0.0);
                    if (indexP2 >= 0)
                    {
                        double P2(0.0);
                        try
                        {
                            P2 = rod.getObs((*it).first, indexP2).data;
                        }
                        catch (...)
                        {
                            continue;
                        }
						if(abs(P2-C1)>1e2) continue;
						rangeP2Vec.push_back(P2);
                        ionocorr = 1.0 / (1.0 - gamma) * (C1 - P2);

                    }

                    prnVec.push_back((*it).first);
                    CNoVec.push_back(S1);

                    rangeVec.push_back(P1 /*- ionocorr /*- rod.clockOffset*C_MPS*/);
                    icorrVec.push_back(ionocorr);
                }
#pragma endregion

                //number of sats
                int N = prnVec.size();
               // cout << setprecision(12) << static_cast<YDSTime> (rod.time) ;
                raimSolver.PrepareAutonomousSolution(rod.time, prnVec, rangeVec, SP3EphList, SVP);
               // cout << " rejected SV ";
                int rejSV = 0;
                for (int j = 0; j<prnVec.size(); j++)
                {
                    if (prnVec[j].id > 0)
                    {
                        
                        if (CNoVec[j] > 26)
                        {
                            N++;
                            UseSat.push_back(true);
                            GoodIndexes.push_back(j);
                        }
                        
                        else
                        {
                            rejSV++;
                            UseSat.push_back(false);
                            cout <<" "<< prnVec[j].id << " SNR = " << (int)CNoVec[j];
                        }
                    }
                    else{
                        
                        UseSat.push_back(false);
                    }
                }
                if (rejSV > 0)
                    cout << endl;

                raimSolver.AutonomousPRSolution(rod.time, UseSat, SVP, tropModelPtr, false, n_iterate, converge, Sol, Cov, Resid, Slope);
                raimSolver.RMSLimit = 3e6;

  
                raimSolver.RAIMCompute(rod.time,
                    prnVec,
                    rangeVec,
                    SP3EphList,
                    tropModelPtr);

                os << setprecision(12) << static_cast<YDSTime> (rod.time) << " "<< Sol[0] << " " << Sol[1] << " " << Sol[2] << " " << n_iterate << " " << GoodIndexes.size()<<" "<< rejSV;
				
				double ssq(0.0);
				for (auto var : Resid){
					ssq+= var*var;
                }
					os << sqrt(ssq / (Resid.size() - 1));

                os << endl;

                //if (raimSolver.isValid()){

                //    os << setprecision(12) << raimSolver.Solution[0] << " ";
                //    os << raimSolver.Solution[1] << " ";
                //    os << raimSolver.Solution[2];
                //    os << raimSolver.Solution[3];
                //    os << endl;

                //}  
                //else{
                //      os << "0 0 0" << endl;
                //}
            } // End of 'if( rod.epochFlag == 0 || rod.epochFlag == 1 )'

        }  // End of 'while( roffs >> rod )'

    }
    catch (Exception& e){
        cerr << e << endl;
    }
    catch (...){
        cerr << "Caught an unexpected exception." << endl;
    }

    os.close();
    exit(0);

} 

void Autonomus::process2()
{
	solverLEO.maskEl =  confReader.fetchListValueAsDouble("Elmask");
	solverLEO.maskSNR = confReader.fetchListValueAsInt("SNRmask");
	IonoModelStore iono;
	IonoModel ioModel;

	try
	{

	}
	catch (...)
	{
		cerr << "cant read IonoModel" << endl;
	}
	ofstream os;
	os.open("output.txt");

	try {
		// In order to throw exceptions, it is necessary to set the failbit
		rin.exceptions(ios::failbit);

		Rinex3ObsHeader roh;
		Rinex3ObsData rod;

		// Let's read the header
		rin >> roh;

		// The following lines fetch the corresponding indexes for some
		// observation types we are interested in. Given that old-style
		// observation types are used, GPS is assumed.
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
		try {
			indexP1 = roh.getObsIndex(L1PCodeID);
			cout << "L1 PRange from " << L1PCodeID << " was used" << endl;
		}
		catch (...) {
			cerr << "The observation file doesn't have P1 pseudoranges." << endl;
			exit(1);
		}

		int indexP2;
		try {
			indexP2 = roh.getObsIndex(L2CodeID);
			cout << "iono-free with " << L2CodeID << " was used" << endl;
		}
		catch (...)
		{
			indexP2 = -1;
		}
		// raimSolver.Debug = true;

		// Let's process all lines of observation data, one by one
		while (rin >> rod)
		{
			Matrix<double> SVP, Cov;
			int N = 0;
		
			vector<int> GoodIndexes;
			// prepare for iteration loop
			Vector<double>Resid, Slope, Sol = 0.0; // initial guess: center of earth
			int n_iterate = 10;
			double converge = 1e-3;
			double Conv = DBL_MAX;
			vector<SatID> prnVec;
			vector<double> rangeVec;
			vector<uchar> SNRs;
			vector<bool> UseSat;
			
			int j = 0;
			solverLEO.selectObservable(rod, indexC1, indexP2, indexCNoL1, prnVec, rangeVec, SNRs);
			do
			{
				for (size_t i = 0; i < prnVec.size(); i++)
				{
					if (SNRs[i] >= solverLEO.maskSNR)
						UseSat.push_back(true);
					else
						UseSat.push_back(false);
				}
				solverLEO.prepare(rod.time, prnVec,rangeVec,SP3EphList,

			)
				j++;

			}
			while (j<n_iterate)


		}  // End of 'while( roffs >> rod )'

	}
	catch (Exception& e) {
		cerr << e << endl;
	}
	catch (...) {
		cerr << "Caught an unexpected exception." << endl;
	}

	os.close();

}

#pragma region 
// End of 'if( rod.epochFlag == 0 || rod.epochFlag == 1 )'
#pragma endregion

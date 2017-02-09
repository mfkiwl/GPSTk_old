#include"autonomus.h"
#include<list>
#include <direct.h>
#include<windows.h>
#include<regex>

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

    subdir = confReader.fetchListValue("RinesObsDir");
    auxiliary::getAllFiles(subdir, rinexObsFiles);
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

#pragma region try get the date

			CommonTime refTime = CommonTime::BEGINNING_OF_TIME;
			if (rNavHeader.fileAgency == "AIUB")
			{

				for (auto it : rNavHeader.commentList)
				{
                    int doy = -1, yr = -1;
					std::tr1::cmatch res;
					std::tr1::regex rxDoY("DAY [0-9]{3}"), rxY(" [0-9]{4}");
					bool b = std::tr1::regex_search(it.c_str(), res, rxDoY);
					if (b)
					{
						string sDay = res[0];
						sDay = sDay.substr(sDay.size() - 4, 4);
						doy = stoi(sDay);
					}
					if (std::tr1::regex_search(it.c_str(), res, rxY))
					{
						string sDay = res[0];
						sDay = sDay.substr(sDay.size() - 5, 5);
						yr = stoi(sDay);
					}
					if (doy > 0 && yr > 0)
					{
						refTime = YDSTime(yr, doy,0,TimeSystem::GPS);
						break;
					}
				}
			}
			else
			{
				long week = rNavHeader.mapTimeCorr["GPUT"].refWeek;
				if (week > 0)
				{
					GPSWeekSecond gpsws = GPSWeekSecond(week, 0);
					refTime = gpsws.convertToCommonTime();
				}
			}
#pragma endregion

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
//
void  Autonomus:: initProcess()
{
    isSpace = confReader.fetchListValueAsBoolean("IsSpaceborneRcv");
    if (isSpace)
        swichToSpaceborn();
    else
    {
        double xapp(confReader.fetchListValueAsDouble("nominalPosition"));
        double yapp(confReader.fetchListValueAsDouble("nominalPosition"));
        double zapp(confReader.fetchListValueAsDouble("nominalPosition"));
        nominalPos = Position(xapp, yapp, zapp);
        DoY = confReader.fetchListValueAsInt("dayOfYear");
    }

    maskEl = confReader.fetchListValueAsDouble("ElMask");
    maskSNR = confReader.fetchListValueAsInt("SNRmask");

    cout << "mask El " << maskEl << endl;
    cout << "mask SNR " << (int)maskSNR << endl;
}

void Autonomus::checkObservable(const string & path)
{
	ofstream os("ObsStat.out");

	for (auto obsFile : rinexObsFiles)
	{

		//Input observation file stream
		Rinex3ObsStream rin;
		// Open Rinex observations file in read-only mode
		rin.open(obsFile, std::ios::in);

		rin.exceptions(ios::failbit);
		Rinex3ObsHeader roh;
		Rinex3ObsData rod;

		//read the header
		rin >> roh;
		int indexC1 = roh.getObsIndex(L1CCodeID);
		int indexCNoL1 = roh.getObsIndex(L1CNo);
		int indexP1 = roh.getObsIndex(L1PCodeID);
		int indexP2 = roh.getObsIndex(L2CodeID);
		while (rin>>rod)
		{
			if (rod.epochFlag == 0 || rod.epochFlag == 1)  // Begin usable data
			{
				int NumC1(0), NumP1(0), NumP2(0), NumBadCNo1(0);
				os << setprecision(12) << rod.time << " " << rod.numSVs<<" ";

				for (auto it : rod.obs)
				{
					double C1 = rod.getObs(it.first, indexC1).data;

					int CNoL1 = rod.getObs(it.first, indexCNoL1).data;
					if (CNoL1 > 30) NumBadCNo1++;

					double P1 = rod.getObs(it.first, indexP1).data;
					if (P1 > 0.0)  NumP1++;
					
					double P2 = rod.getObs(it.first, indexP2).data;
					if (P2 > 0.0)  NumP2++;
				}

				os << NumBadCNo1 << " " << NumP1 << " " << NumP2 << endl;
			}
		}

	}
}
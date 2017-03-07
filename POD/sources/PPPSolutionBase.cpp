
#include"stdafx.h"
#include"PPPSolutionBase.h"
#include<list>
#include <direct.h>
#include<windows.h>
#include<regex>

namespace POD
{
    PPPSolutionBase* PPPSolutionBase::Factory(bool isSpaceborne, ConfDataReader & reader)
    {
        if (isSpaceborne)
            return new PODSolution(reader);
        else
            return new PPPSolution(reader);
    }
    PPPSolutionBase::PPPSolutionBase(ConfDataReader & cReader) : confReader(&cReader)
    {
    } 

    void swichToSpaceborn()
    {

    }
    bool  PPPSolutionBase::LoadData()
    {
        cout << "Ephemeris Loading... ";
        cout << loadEphemeris() << endl;
        cout << "Clocks Loading... ";
        cout << loadClocks() << endl;
        cout << "IonoModel Loading... ";
        cout << loadIono() << endl;

        //get application dir
        char current_work_dir[_MAX_FNAME];
        _getcwd(current_work_dir, sizeof(current_work_dir));
        string s_dir(current_work_dir);
        //set generic files direcory 
        string subdir = confReader->fetchListValue("GenericFilesDir");
        genFilesDir = s_dir + "\\" + subdir + "\\";

        subdir = confReader->fetchListValue("RinesObsDir");
        auxiliary::getAllFiles(subdir, rinexObsFiles);

        return true;
    }
    
    //
    bool PPPSolutionBase::loadEphemeris()
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
            try
            {

                SP3EphList.loadFile(file);
            }
            catch (FileMissingException& e)
            {
                // If file doesn't exist, issue a warning
                cerr << "SP3 file '" << file << "' doesn't exist or you don't "
                    << "have permission to read it. Skipping it." << endl;

                return false;
            }
        }
        return true;
    }
    //reading clock
    bool PPPSolutionBase::loadClocks()
    {
        list<string> files;
        string subdir = confReader.fetchListValue("RinexClockDir");
        auxiliary::getAllFiles(subdir, files);

        for (auto file : files)
        {
            // Try to load each ephemeris file
            try
            {
                SP3EphList.loadRinexClockFile(file);
            }
            catch (FileMissingException& e)
            {
                // If file doesn't exist, issue a warning
                cerr << "Rinex clock file '" << file << "' doesn't exist or you don't "
                    << "have permission to read it. Skipping it." << endl;
                return false;
            }
        }//   while ((ClkFile = confReader.fetchListValue("rinexClockFiles", station)) != "")
        return true;
    }

    bool  PPPSolutionBase::loadIono()
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
                            refTime = YDSTime(yr, doy, 0, TimeSystem::GPS);
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
    
    void PPPSolutionBase::checkObservable(const string & path)
    {
        ofstream os("ObsStatisic.out");

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
            while (rin >> rod)
            {
                if (rod.epochFlag == 0 || rod.epochFlag == 1)  // Begin usable data
                {
                    int NumC1(0), NumP1(0), NumP2(0), NumBadCNo1(0);
                    os << setprecision(12) << (CivilTime)rod.time << " " << rod.numSVs << " ";

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

    void PPPSolutionBase::process()
    {
        initProcess();

#ifndef DBG

        cout << "Approximate Positions loading... ";
        cout << loadApprPos("nomPos.in") << endl;
#else
        PRprocess();
#endif
        cout << "1" << endl;
        if (isSpace)
            PPPprocessLEO();
        else
            PPPprocessGB();
    }

}
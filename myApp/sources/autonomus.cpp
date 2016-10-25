#include"stdafx.h"
#include"actions.h"

int Actions:: StanaloneSPP(int argc, char *argv[])
{
	Position Pos;
	bool useFormerPos = false;
	// Declaration of objects for storing ephemerides and handling RAIM
	GPSEphemerisStore bcestore;
	PRSolution2 raimSolver;
	ModeledPR modelPR;            // Declare a ModeledReferencePR object
	MOPSWeight mopsWeights;
	SolverWMS solver;
	ExtractData obsChen1;
	ExtractData obsChen2;

	//IOnospheric model

	IonoModelStore ionoStore;
	IonoModel ioModel;

	// Object for void-type tropospheric model (in case no meteorological
	// RINEX is available)
	ZeroTropModel noTropModel;

	// Object for GG-type tropospheric model (Goad and Goodman, 1974)
	// Default constructor => default values for model
	MOPSTropModel mopsTropModel;
	//

	// Pointer to one of the two available tropospheric models. It points
	// to the void model by default
	TropModel *tropModelPtr = &mopsTropModel;

	//tropModelPtr->setWeather(20, 1010, 50);
	// This verifies the ammount of command-line parameters given and
	// prints a help message, if necessary
	if ((argc < 3) || (argc > 4))
	{
		cerr << "Usage:" << endl;
		cerr << "   " << argv[0]
			<< " <RINEX Obs file>  <RINEX Nav file>  [<RINEX Met file>]"
			<< endl;

		exit(-1);
	}

	// Let's compute an useful constant (also found in "GNSSconstants.hpp")
	const double gamma = (L1_FREQ_GPS / L2_FREQ_GPS)*(L1_FREQ_GPS / L2_FREQ_GPS);

	try
	{
#pragma region Read Ephemerides

		// Read nav file and store unique list of ephemerides
		Rinex3NavStream rnffs(argv[2]);    // Open ephemerides data file
		Rinex3NavData rne;
		Rinex3NavHeader hdr;

		// Let's read the header (may be skipped)
		rnffs >> hdr;

		if (hdr.valid & Rinex3NavHeader::validIonoCorrGPS)
		{
			// Extract the Alpha and Beta parameters from the header
			double* ionAlpha = hdr.mapIonoCorr["GPSA"].param;
			double* ionBeta = hdr.mapIonoCorr["GPSB"].param;

			// Feed the ionospheric model with the parameters

			ioModel.setModel(ionAlpha, ionBeta);
			ionoStore.addIonoModel(CommonTime::BEGINNING_OF_TIME, ioModel);
		}

		// Storing the ephemeris in "bcstore"
		while (rnffs >> rne) bcestore.addEphemeris(rne);

		// Setting the criteria for looking up ephemeris
		bcestore.SearchNear();

#pragma endregion

#pragma region Read Meteo
		// If provided, open and store met file into a linked list.
		list<RinexMetData> rml;

		if (argc == 4)
		{

			RinexMetStream rms(argv[3]);    // Open meteorological data file
			RinexMetHeader rmh;

			// Let's read the header (may be skipped)
			rms >> rmh;

			RinexMetData rmd;

			// All data is read into "rml", a meteorological data
			// linked list
			while (rms >> rmd) rml.push_back(rmd);

		}  // End of 'if( argc == 4 )'

#pragma endregion

		   // Open and read the observation file one epoch at a time.
		   // For each epoch, compute and print a position solution
		Rinex3ObsStream roffs(argv[1]);    // Open observations data file

										   // In order to throw exceptions, it is necessary to set the failbit
		roffs.exceptions(ios::failbit);

		Rinex3ObsHeader roh;
		Rinex3ObsData rod;

		// Let's read the header
		roffs >> roh;

#pragma region Set Observation Chennals IDs
		// The following lines fetch the corresponding indexes for some
		// observation types we are interested in. Given that old-style
		// observation types are used, GPS is assumed.
		int indexCan1;
		try
		{
			indexCan1 = roh.getObsIndex("C1W");
		}
		catch (...)
		{
			cerr << "The observation file doesn't have P1 pseudoranges." << endl;
			exit(1);
		}

		int indexCan2;
		try
		{
			indexCan2 = roh.getObsIndex("C2W");
		}
		catch (...)
		{
			indexCan2 = -1;
		}
#pragma endregion

		//
		//init position  via Rinex header coordinates
		Pos = Position(roh.antennaPosition, Position::Cartesian, (EllipsoidModel*)0, ReferenceFrame::WGS84);

		// Defining iterator "mi" for meteorological data linked list
		// "rml", and set it to the beginning
		list<RinexMetData>::iterator mi = rml.begin();

		// Let's process all lines of observation data, one by one
		while (roffs >> rod)
		{
#pragma region set  wether
			// Find a weather point. Only if a meteorological RINEX file
			// was provided, the meteorological data linked list "rml" is
			// neither empty or at its end, and the time of meteorological
			// records are below observation data epoch.
			while ((argc == 4) &&
				(!rml.empty()) &&
				(mi != rml.end()) &&
				((*mi).time < rod.time))
			{

				mi++;    // Read next item in list

						 // Feed GG tropospheric model object with meteorological
						 // parameters. Take into account, however, that setWeather
						 // is not accumulative, i.e., only the last fed set of
						 // data will be used for computation
						 //init 

				mopsTropModel = MOPSTropModel(Pos.getAltitude(), Pos.getGeodeticLatitude(), static_cast<YDSTime>(roh.firstObs).doy);
				mopsTropModel.setWeather((*mi).data[RinexMetHeader::TD], (*mi).data[RinexMetHeader::PR], (*mi).data[RinexMetHeader::HR]);

			}  // End of 'while( ( argc==4 ) && ...'
#pragma endregion

			   // Apply editing criteria
			if ((rod.epochFlag == 0 || rod.epochFlag == 1) && rod.numSVs > 3)  // Begin usable data
			{
				// Number of satellites with valid data in this epoch
				int validSats = 0;
				int prepareResult;
				double rxAltitude;  // Receiver altitude for tropospheric model
				double rxLatitude;  // Receiver latitude for tropospheric model

				if (obsChen1.getData(rod, indexCan1) < 4)
				{
					// The former position will not be valid next time
					useFormerPos = false;
					continue;
				}
				obsChen2.getData(rod, indexCan2);
				// If possible, use former position as a priori
				if (useFormerPos)
				{

					prepareResult = modelPR.Prepare(Pos);

					// We need to seed this kind of tropospheric model with
					// receiver altitude
					rxAltitude = Pos.getAltitude();
					rxLatitude = Pos.getGeodeticLatitude();

				}
				else
				{
					// Use Bancroft method is no a priori position is available
					cerr << "Bancroft method was used at epoch "
						<< static_cast<YDSTime>(rod.time).sod << endl;

					prepareResult = modelPR.Prepare(rod.time,
						obsChen1.availableSV,
						obsChen1.obsData,
						bcestore);

					// We need to seed this kind of tropospheric model with
					// receiver altitude
					rxAltitude = modelPR.rxPos.getAltitude();
					rxLatitude = modelPR.rxPos.getGeodeticLatitude();
				}

				// If there were problems with Prepare(), skip this epoch
				if (prepareResult)
				{
					// The former position will not be valid next time
					useFormerPos = false;
					continue;
				}

				// Now, let's compute the GPS model for our observable (C1)
				validSats = modelPR.Compute(rod.time,
					obsChen1.availableSV,
					obsChen1.obsData,
					bcestore,
					&mopsTropModel,
					&ionoStore);

				if (validSats >= 4)
				{

					// Now let's solve the navigation equations using the WMS method
					try
					{
						// First, compute the satellites' weights
						int goodSv = mopsWeights.getWeights(rod.time,
							modelPR.availableSV,
							bcestore,
							modelPR.ionoCorrections,
							modelPR.elevationSV,
							modelPR.azimuthSV,
							modelPR.rxPos);

						// Some minimum checking is in order
						if (goodSv != (int)modelPR.prefitResiduals.size()) continue;

						// Then, solve the system
						solver.Compute(modelPR.prefitResiduals,
							modelPR.geoMatrix,
							mopsWeights.weightsVector);

					}
					catch (InvalidSolver& e)
					{
						cerr << "Couldn't solve equations system at epoch "
							<< static_cast<YDSTime>(rod.time).sod << endl;
						cerr << e << endl;

						// The former position will not be valid next time
						useFormerPos = false;
						continue;
					}
					Position solPos((modelPR.rxPos.X() + solver.solution[0]),
						(modelPR.rxPos.Y() + solver.solution[1]),
						(modelPR.rxPos.Z() + solver.solution[2]));

					cout << CivilTime(rod.time) << " "<< rod.time.getSecondOfDay()<<" ";


					// Vector "Solution" holds the coordinates, expressed in
					// meters in an Earth Centered, Earth Fixed (ECEF) reference
					// frame. The order is x, y, z  (as all ECEF objects)
					double pos[3];
					for (size_t i = 0; i < 3; i++)
						pos[i] = raimSolver.Solution[i];

					cout << setprecision(12) << solPos.getGeodeticLatitude() << " ";
					cout << solPos.getLongitude() << " ";
					cout << solPos.getAltitude() << " ";

					cout << endl;
					Pos = solPos;

					// Next time, former position will be used as a priori
					useFormerPos = true;

					// End of 'if( validSats >= 4 )'
				}
				else
				{
					// The former position will not be valid next time
					useFormerPos = false;
				}

			}  // End of 'if( (rData.epochFlag == 0 || rData.epochFlag == 1) &&...'
			else
			{
				// The former position will not be valid next time
				useFormerPos = false;
			}

			// Apply editing criteria

		}// End of 'while( roffs >> rod )'
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
	return 0;
}
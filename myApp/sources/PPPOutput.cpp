#include "stdafx.h"

// Method to print solution values
void ex9::printSolution(ofstream& outfile,
	const SolverLMS& solver,
	const CommonTime& time0,
	const CommonTime& time,
	const ComputeDOP& cDOP,
	bool  useNEU,
	int   numSats,
	double dryTropo,
	vector<PowerSum> &stats,
	int   precision)
{

	// Prepare for printing
	outfile << fixed << setprecision(precision);

	// Print results
	outfile << static_cast<YDSTime>(time).year << "-";   // Year           - #1
	outfile << static_cast<YDSTime>(time).doy << "-";    // DayOfYear      - #2
	outfile << static_cast<YDSTime>(time).sod << "  ";   // SecondsOfDay   - #3
    outfile << setprecision(6)<< (static_cast<YDSTime>(time).doy + static_cast<YDSTime>(time).sod / 86400.0)<<"  "<< setprecision(precision);

	//calculate statistic
	double x(0), y(0), z(0), varX(0), varY(0), varZ(0);

	// We add 0.1 meters to 'wetMap' because 'NeillTropModel' sets a
	// nominal value of 0.1 m. Also to get the total we have to add the
	// dry tropospheric delay value
	// ztd - #7
	double wetMap = solver.getSolution(TypeID::wetMap) + 0.1 + dryTropo;
	double varWetMap = solver.getVariance(TypeID::wetMap);

	if (useNEU) {

		x = solver.getSolution(TypeID::dLat);      // dLat  - #4
		y = solver.getSolution(TypeID::dLon);      // dLon  - #5
		z = solver.getSolution(TypeID::dH);        // dH    - #6

		varX = solver.getVariance(TypeID::dLat);   // Cov dLat  - #8
		varY = solver.getVariance(TypeID::dLon);   // Cov dLon  - #9
		varZ = solver.getVariance(TypeID::dH);     // Cov dH    - #10
	}
	else {

		x = solver.getSolution(TypeID::dx);         // dx    - #4
		y = solver.getSolution(TypeID::dy);         // dy    - #5
		z = solver.getSolution(TypeID::dz);         // dz    - #6
        
		varX = solver.getVariance(TypeID::dx);     // Cov dx    - #8
		varY = solver.getVariance(TypeID::dy);     // Cov dy    - #9
		varZ = solver.getVariance(TypeID::dz);     // Cov dz    - #10
	}
	//
	outfile  << x << "  " << y << "  " << z << "  " << wetMap << "  ";
	outfile << sqrt(varX*varX+ varY*varY+ varZ*varZ)<< "  ";

	outfile << numSats << endl;    // Number of satellites - #12

    //time of convergence by default 7200 seconds;
	double tConv(5400.0);

    double dt = time - time0;
  
	if (dt > tConv)
	{
		stats[0].add(x);
		stats[1].add(y);
		stats[2].add(z);
		stats[3].add(wetMap);
	}

	return;


}  // End of method 'ex9::printSolution()'



void ex9::printStats(ofstream& outfile,
	const vector<PowerSum> &stats)
{
	Triple Aver, Var;
	double _3DRMS(0.0);

	for (size_t i = 0; i < 3; i++)
	{
		Aver[i] = stats[i].average();
		Var[i] = stats[i].variance();
		_3DRMS += Var[i];
		Var[i] = sqrt(Var[i]);
	}

	outfile << "Averege  " << Aver[0] << "  " << Aver[1] << "  " << Aver[2] << "  ";
	outfile << "St.Dev.  " << Var[0] << "  " << Var[1] << "  " << Var[2] << "  " << sqrt(_3DRMS) << endl;
}

// Method to print model values
void ex9::printModel(ofstream& modelfile,
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
		it++) {

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
			itObs++) {
			// Print type names and values
			modelfile << (*itObs).first << " ";
			modelfile << (*itObs).second << " ";

		}  // End of 'for( typeValueMap::const_iterator itObs = ...'

		modelfile << endl;

	}  // End for (it = gData.body.begin(); ... )

}  // End of method 'ex9::printModel()'


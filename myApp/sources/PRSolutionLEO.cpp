#include"PRSolutionLEO.h"

void  PRSolutionLEO::selectObservable(
	const Rinex3ObsData &rod,
	int iL1Code,
	int iL2Code,
	int iL1SNR,
	vector<SatID> & PRNs,
	vector<double> &L1PRs,
	vector<uchar> & SNRs
)
{

	// Let's compute an useful constant (also found in "GNSSconstants.hpp")
	const double gamma = (L1_FREQ_GPS / L2_FREQ_GPS)*(L1_FREQ_GPS / L2_FREQ_GPS);
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
				C1 = rod.getObs((*it).first, iL1Code).data;
			    S1 = rod.getObs((*it).first, iL1SNR).data;
			}
			catch (...)
			{
				// Ignore this satellite if P1 is not found
				continue;
			}

			double ionocorr(0.0);
			if (iL2Code >= 0)
			{
				double P2(0.0);
				try
				{
					P2 = rod.getObs((*it).first, iL2Code).data;
				}
				catch (...)
				{
					continue;
				}
				if (abs(P2 - C1) > 1e2) continue;
				ionocorr = 1.0 / (1.0 - gamma) * (C1 - P2);

			}

			PRNs.push_back((*it).first);
			SNRs.push_back(S1);
			L1PRs.push_back(C1 /*- ionocorr /*- rod.clockOffset*C_MPS*/);
		}





	}
}



void PRSolutionLEO::prepare(
	const CommonTime &t,
	const vector<SatID> &IDs,
	const vector<double> &PRs,
	const XvtStore<SatID>& Eph,
	const IonoModelStore &iono,
	const double &elm,
	const double &Conv,
	Position PosRX,
	vector<bool> &UsedSat,
	Matrix<double> &SVP)
{
	CommonTime tx = t;
	for (size_t i = 0; i < IDs.size(); i++)
	{
		// transmit time
		Xvt PVT;
		SatID &ID = IDs[i];
		// first estimate of transmit time
		tx -= PRs[i] / C_MPS;
		// get ephemeris range, etc
		try
		{
			PVT = Eph.getXvt(ID, tx);
		}
		catch (InvalidRequest& e)
		{
			UsedSat[i] = false;
			continue;
		}

		tx -= PVT.clkbias + PVT.relcorr;

		try
		{
			PVT = Eph.getXvt(ID, tx);
		}
		catch (InvalidRequest& e)
		{
			///Negate SatID because getXvt failed.
			ID.id = -::abs(ID.id);
			UsedSat[i] = false;
			continue;
		}
		// SVP = {SV position at transmit time}, raw range + clk + rel
		for (int l = 0; l < 3; l++)
		{
			SVP(i, l) = PVT.x[l];
		}
		SVP(i, 3) = PRs[i] + C_MPS * (PVT.clkbias + PVT.relcorr);


		if (Conv < 100)
		{
			Position SVpos(PVT);
			double elv = PosRX.elevationGeodetic(SVpos);
			if (elv < elm)
			{
				UsedSat[i] = false;
				continue;
			}

			double azm = PosRX.azimuth(SVpos);

			SVP(i, 3) -= iono.getCorrection(t, PosRX, elv, azm);
		}
	}
}

	int  PRSolutionLEO::ajustParameters(
		const Matrix<double> &SVP,
		Vector<bool> &Use,
		double &Conv,
		int &nIter,
		Vector<double>& Sol,
		Matrix<double>& Cov,
		Vector<double>& Resid	)
	{
		int iret, j, n, N;
		size_t i;
		// find the number of good satellites
		for (N = 0, i = 0; i<Use.size(); i++)
		{
			if (Use[i]) N++;
		}

		// define for computation

		Vector<double> CRange(N), dX(4);
		Matrix<double> P(N, 4), PT, G(4, N), PG(N, N);
		Xvt SV, RX;

		Sol.resize(4);
		Cov.resize(4, 4);
		Resid.resize(N);

		for (i = 0; i<3; i++) RX.x[i] = Sol(i);

		for (n = 0, i = 0; i<Use.size(); i++)
		{
			if (!Use[i]) continue;

			double rho = RSS(SVP(i, 0) - Sol(0), SVP(i, 1) - Sol(1), SVP(i, 2) - Sol(2));
			
			// corrected pseudorange (m)
			CRange(n) = SVP(i, 3);

			// partials matrix
			P(n, 0) = (Sol(0) - SVP(i, 0)) / rho; // x direction cosine
			P(n, 1) = (Sol(1) - SVP(i, 1)) / rho; // y direction cosine
			P(n, 2) = (Sol(2) - SVP(i, 2)) / rho; // z direction cosine
			P(n, 3) = 1.0;

			// data vector: corrected range residual
			Resid(n) = CRange(n) - rho - Sol(3);

			PT = transpose(P);

			Cov = PT * P;
			try
			{
				Cov = inverseSVD(Cov);
			}
			//try { Cov = inverseLUD(Cov); }
			catch (SingularMatrixException& sme)
			{
				return -2;
			}
			// generalized inverse
			G = Cov * PT;
			dX = G * Resid;
			Sol += dX;
			// test for convergence
			Conv = norm(dX);
			nIter++;
			return 0;
		}



	};


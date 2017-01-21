
#include"stdafx.h"
#include"PRSolutionLEO.h"

double PRSolutionLEO::eps = 1e-4;

void  PRSolutionLEO::selectObservable(
    const Rinex3ObsData &rod,
    int iL1Code,
    int iL2Code,
    int iL1SNR,
    vector<SatID> & PRNs,
    vector<double> &L1PRs,
    vector<uchar> & SNRs,
    bool isApplyRCO
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
            if (isApplyRCO) C1 -= rod.clockOffset*C_MPS;
            L1PRs.push_back(C1);
        }
    }
}

void PRSolutionLEO::prepare(
    const CommonTime &t,
    vector<SatID> &IDs,
    const vector<double> &PRs,
    const XvtStore<SatID>& Eph,
    const IonoModelStore &iono,
    vector<bool> &useSat,
    Matrix<double> &SVP)
{
    CommonTime tx;

    for (size_t i = 0; i < IDs.size(); i++)
    {
        if (!useSat[i]) continue;
        // transmit time
        Xvt PVT;
        // first estimate of transmit time
        tx = t;
        tx -= PRs[i] / C_MPS;
        // get ephemeris range, etc
        try
        {
            PVT = Eph.getXvt(IDs[i], tx);
        }
        catch (InvalidRequest& e)
        {
            useSat[i] = false;
            continue;
        }

        tx -= PVT.clkbias + PVT.relcorr;

        try
        {
            PVT = Eph.getXvt(IDs[i], tx);
        }
        catch (InvalidRequest& e)
        {
            ///Negate SatID because getXvt failed.
            IDs[i].id = -::abs(IDs[i].id);
            useSat[i] = false;
            continue;
        }
        // SVP = {SV position at transmit time}, raw range + clk + rel
        for (int l = 0; l < 3; l++)
        {
            SVP(i, l) = PVT.x[l];
        }
        SVP(i, 3) = PRs[i] + C_MPS * (PVT.clkbias + PVT.relcorr);
    }
}

int  PRSolutionLEO::ajustParameters(
    const CommonTime &t,
    const Matrix<double> &SVP,
    vector<bool> &useSat,
    Matrix<double>& Cov,
    Vector<double>& Resid,
    IonoModelStore &iono,
    bool isApplyIono
)
{
    int  j, n, N;
    size_t i;
    iter = 0;
    double conv = DBL_MAX;
    do
    {
        // find the number of good satellites
        for (N = 0, i = 0; i < useSat.size(); i++)
        {
            if (useSat[i]) N++;
        }

        // define for computation
        Vector<double> CRange(N), dX(4);
        Matrix<double> P(N, 4), PT, G(4, N), PG(N, N);

        //Sol.resize(4);
        //Cov.resize(4, 4);
        Resid.resize(N);

        Position PosRX(Sol(0), Sol(1), Sol(2));
        ps.clear();

        for (n = 0, i = 0; i < useSat.size(); i++)
        {
            double iodel(0.0);
            if (conv < 100 && useSat[i])
            {
                Position SVpos(SVP(i, 0), SVP(i, 1), SVP(i, 2));
                double elv = PosRX.elevationGeodetic(SVpos);
                if (elv < maskEl)
                {
                    useSat[i] = false;
                    continue;
                }
                if (isApplyIono)
                {
                    double azm = PosRX.azimuth(SVpos);
                    double iodel = iono.getCorrection(t, PosRX, elv, azm);
                }
            }
            if (!useSat[i]) continue;

            double rho = RSS(SVP(i, 0) - Sol(0), SVP(i, 1) - Sol(1), SVP(i, 2) - Sol(2));

            // corrected pseudorange (m)
            CRange(n) = SVP(i, 3);

            // partials matrix
            P(n, 0) = (Sol(0) - SVP(i, 0)) / rho; // x direction cosine
            P(n, 1) = (Sol(1) - SVP(i, 1)) / rho; // y direction cosine
            P(n, 2) = (Sol(2) - SVP(i, 2)) / rho; // z direction cosine
            P(n, 3) = 1.0;

            // data vector: corrected range residual
            Resid(n) = CRange(n) - rho - Sol(3) - iodel;
            ps.add(Resid(n));
            n++;
        }

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
        conv = norm(dX);
        iter++;

    } while ((maxIter<iter) || (eps<conv));

    calcStat(Resid, Cov);

    if (sigma > 100)
    {

        return recalc(0, t, SVP, useSat, iono, isApplyIono);

    }

    return 1;
};

void PRSolutionLEO::calcStat(Vector<double> resid, Matrix<double> Cov)
{
    RMS3D = 0, PDOP = 0;

    double variance = ps.variance();
    this->sigma = sqrt(variance);

    for (size_t i = 0; i < 3; i++)
    {
        RMS3D += variance*Cov(i, i);
        PDOP += Cov(i, i);
    }
    RMS3D = sqrt(RMS3D);
}

string PRSolutionLEO::printSolution(const vector<bool> &useSat)
{
    std::ostringstream strs;
    strs << setprecision(12) << " " << Sol(0) << " " << Sol(1) << " " << Sol(2) << " " << iter << " " << useSat.size() << " " << ps.size() << " " << sigma << " " << RMS3D;
    return strs.str();
}

int PRSolutionLEO::recalc(
    int i_ex,
    const CommonTime &t,
    const Matrix<double> &SVP,
    vector<bool> &useSat,
    IonoModelStore &iono,
    bool isApplyIono)
{
    for (int _j = 0; _j < useSat.size(); _j++)
    {
        if (!useSat[_j]) continue;

        useSat[_j] = false;

        double conv = DBL_MAX;
        int iter_ = 0;
        int  j;

        int N = 0;
        // find the number of good satellites
        for (int i = 0; i < useSat.size(); i++)
        {
            if (useSat[i]) N++;
        }

        // define for computation
        Vector<double> CRange(N), dX(4), Resid(N);
        Matrix<double> P(N, 4), PT(4, N), G(4, N), Cov(4, 4);

        //
        while (1)
        {
            Position PosRX(Sol(0), Sol(1), Sol(2));
            ps.clear();
            int n = 0;
            for (int i = 0; i < useSat.size(); i++)
            {
                double iodel(0.0);
                if (conv < 100 && isApplyIono && useSat[i])
                {
                    Position SVpos(SVP(i, 0), SVP(i, 1), SVP(i, 2));
                    double elv = PosRX.elevationGeodetic(SVpos);
                    double azm = PosRX.azimuth(SVpos);
                    double iodel = iono.getCorrection(t, PosRX, elv, azm);
                }
                if (!useSat[i]) continue;
                double rho = RSS(SVP(i, 0) - Sol(0), SVP(i, 1) - Sol(1), SVP(i, 2) - Sol(2));

                // corrected pseudorange (m)
                CRange(n) = SVP(i, 3);

                // partials matrix
                P(n, 0) = (Sol(0) - SVP(i, 0)) / rho; // x direction cosine
                P(n, 1) = (Sol(1) - SVP(i, 1)) / rho; // y direction cosine
                P(n, 2) = (Sol(2) - SVP(i, 2)) / rho; // z direction cosine
                P(n, 3) = 1.0;

                // data vector: corrected range residual
                Resid(n) = CRange(n) - rho - Sol(3) - iodel;
                ps.add(Resid(n));
                n++;
            }

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
            conv = norm(dX);
            iter_++;

            bool b1 = maxIter < iter_;
            bool b2 = eps < conv;
            if (b1 || b1) break;

        }

        calcStat(Resid, Cov);
        if (this->sigma < 100) return 1;
        else
        {
            useSat[_j] = true;

        }
    }
    return 0;
}


#include"PRSolutionLEO.h"

void PRSolutionLEO::Prepare(const CommonTime &t,
                            const Vector<SatID> &IDs,
                            const Vector<double> &PRs,
                            const Vector<char> SNR,
                            const XvtStore<SatID>& Eph,
                            double Conv,
                            Vector<bool> &UsedSat,
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
                continue;
            }
            // SVP = {SV position at transmit time}, raw range + clk + rel
            for (int l = 0; l<3; l++)
            {
                SVP(i, l) = PVT.x[l];
            }
            SVP(i, 3) = rangeVec[i] + C_MPS * (PVT.clkbias + PVT.relcorr);



            if (Conv < 100)
            {

            }
        }
        // find the number of good satellites
        for (N = 0, i = 0; i<UseSat.size(); i++)
        {
            if (Use[i])
                N++;
        }

        // define for computation
        Vector<double> CRange(N), dX(4);
        Matrix<double> P(N, 4), PT, G(4, N), PG(N, N);
        Xvt SV, RX;

        while (true)
        {

        }
    }

#pragma endregion


};
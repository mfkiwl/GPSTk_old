#include"stdafx.h"
#include "SP3Aux.h"

SP3Aux::SP3Aux()
{
}
int SP3Aux::interpSP3Eph(
	const SP3EphemerisStore & master,
	const double newSampl,
    const CommonTime& t0,
    const CommonTime& te,
	SP3EphemerisStore & slv )
{
    int order = master.getPositionInterpOrder();
	slv = SP3EphemerisStore(master);
	slv.clear();
    
    double dt0 = t0 - master.getInitialTime();
    double dte = master.getFinalTime() - te;
    
    const double minIndent = master.getPositionInterpOrder()*900.0/2;

    if (dt0 < minIndent || dte < minIndent) return 1;

    //satellites with clocks and positions
    auto satwClk = master.getClockSatList();
	auto satwPos = master.getPositionSatList();
    //
	bool hasClkDrift = master.hasClockDrift();
    CommonTime time = t0;
    
    //Clock bias multiplier depends on the source of clock data in internal storage
	double clkKoeff = 1.0;//(master.isUseSP3ClockData()) ? 1e6 : 1.0;

	while (true)
	{
		//
		for (auto it : satwClk)
		{
			try
			{
				Xvt sData = master.getXvt(it, time);

				slv.addPositionData(it, time, 0.001*sData.getPos(), Triple());
				slv.addClockBias(it, time, sData.clkbias*clkKoeff);
				if (hasClkDrift) slv.addClockDrift(it, time, sData.clkdrift);
			}
			catch (InvalidRequest& ir)
			{
				cerr << (ir).getText() << endl;
				continue;
			}
		}

        time.addSeconds(newSampl);
        if (time > te) break;
	}
	return 0;
}
//
int SP3Aux::interpSP3Eph(
    const SP3EphemerisStore & master,
    const double newSampl,
    SP3EphemerisStore & slv)
{
    if (master.size() < 11) return 1;
    //dafault sampling
    double sampl = 900.0;
    int order = master.getPositionInterpOrder();

    CommonTime t0 = master.getInitialTime();
    CommonTime te = master.getFinalTime();
    t0.addSeconds(sampl*(order/2));
    te.addSeconds(-sampl*(order / 2));
    return interpSP3Eph(master, newSampl, t0, te, slv);

}

SP3Aux::~SP3Aux()
{
}

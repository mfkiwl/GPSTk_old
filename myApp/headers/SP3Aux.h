#ifndef SP3AUX_HPP
#define SP3AUX_HPP

class SP3Aux
{
public:
    SP3Aux();
	~SP3Aux();


	static int interpSP3Eph(const SP3EphemerisStore & master, 
							const double sampl,
							SP3EphemerisStore & slv);

    static int interpSP3Eph(const SP3EphemerisStore & master,
                            const double sampl,
                            const CommonTime& t0,
                            const CommonTime& te,
                            SP3EphemerisStore & slv);
};


#endif // !SP3AUX_HPP
#include "sdr.h"

class CStatic_Frequency_Changer
{
private:
	double freq;
public:
	/* Default initialization frequency is kept at GPS L1. This is because, without any modification to the program,
	it should run L1 GPS atleast, without any modification (Safe measure) */
	CStatic_Frequency_Changer() : freq(1575420000.0)
	{}

	double GetFrequency() { return freq; }
	void SetFrequency(double v) { freq = v; }
};


CStatic_Frequency_Changer the_CStatic_Frequenncy_Changer;

EXTERNCPP void FrequencyChanger_SetFrequency(double freq)
{
	the_CStatic_Frequenncy_Changer.SetFrequency(freq);
}

EXTERNCPP double FrequencyChanger_GetFrequency(void)
{
	return the_CStatic_Frequenncy_Changer.GetFrequency();
}

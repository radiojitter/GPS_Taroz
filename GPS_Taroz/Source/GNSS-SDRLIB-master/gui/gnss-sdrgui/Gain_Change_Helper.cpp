#include "sdr.h"


/* gain changing Helper API */
class CStatic_GainChanger
{
private:
	GainChanger_UpdateGainCallback _UpdateGain;
	double gainValue;
	SRWLOCK locking;
public:
	CStatic_GainChanger() : _UpdateGain(NULL), gainValue(0.0)
	{
		locking.Ptr = NULL;
		InitializeSRWLock(&locking);
	}

	int GetGainPercent()
	{
		return (int) gainValue;
	}

	void SetGainPercent(int gainpercent)
	{
		AcquireSRWLockExclusive(&locking);
		Internal_SetGainPercent(gainpercent);
		ReleaseSRWLockExclusive(&locking);		
	}
private:
	void Internal_SetGainPercent(int gainpercent)
	{
		try
		{
			if(_UpdateGain == NULL)
			{
				gainValue = gainpercent;
			}
			else
			{
				gainValue = (_UpdateGain)((double) gainpercent);
			}
		}
		catch(...)
		{}
	}
public:
	void Reset()
	{
		_UpdateGain = NULL;
	}
	void Init(GainChanger_UpdateGainCallback _func)
	{
		if(_func != NULL)
		{
			_UpdateGain = _func;
			gainValue = (_UpdateGain)(gainValue);
		}
	}
	
};

CStatic_GainChanger the_CStatic_GainChanger;

EXTERNCPP void GainChanger_SetCallback(GainChanger_UpdateGainCallback _func)
{
	the_CStatic_GainChanger.Init(_func);

}

EXTERNCPP void GainChanger_SetGain(int percent)
{
	the_CStatic_GainChanger.SetGainPercent(percent);
}

EXTERNCPP int	GainChanger_GetGain(void)
{
	return the_CStatic_GainChanger.GetGainPercent();
}

EXTERNCPP void GainChanger_Reset(void)
{
	the_CStatic_GainChanger.Reset();
}
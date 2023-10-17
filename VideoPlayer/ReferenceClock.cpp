#include "ReferenceClock.h"
#include "ClockSource.h"


ReferenceClock& ReferenceClock::getInstance()
{
	static ReferenceClock instance;
	return instance;
}

double ReferenceClock::getValue() const
{
	return clockSource != nullptr ? clockSource->getClock() : 0.0;
}

#pragma once

class ClockSource
{
public:
	virtual ~ClockSource() = default;
	virtual double getClock() const = 0;
};

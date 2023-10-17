#pragma once

class ClockSource;

class ReferenceClock final
{
private:
	ReferenceClock() = default;

public:
	~ReferenceClock() = default;

	static ReferenceClock& getInstance();

	double getValue() const;
	void setSource(const ClockSource *clkSource)
	{
		clockSource = clkSource;
	}

private:
	const ClockSource *clockSource = nullptr;
};

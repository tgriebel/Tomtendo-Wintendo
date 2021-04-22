#pragma once

#include <chrono>

const uint64_t MasterClockHz		= 21477272;
const uint64_t CpuClockDivide		= 12;
const uint64_t ApuClockDivide		= 24;
const uint64_t ApuSequenceDivide	= 89490;
const uint64_t PpuClockDivide		= 4;
const uint64_t PpuCyclesPerCpuCycle = ( CpuClockDivide / PpuClockDivide );
const uint64_t PpuCyclesPerScanline	= 341;
const uint64_t FPS					= 60;
const uint64_t MinFPS				= 30;

static const uint64_t NanoToSeconds		= 1000000000;
static const uint64_t MicroToSeconds	= 1000000;
static const uint64_t MilliToSeconds	= 1000;

template< uint64_t NUM, uint64_t DENOM >
struct ratio_t
{
	static const uint64_t	n = NUM;
	static const uint64_t	d = DENOM;
	static constexpr double r = ( n / ( ( d == 0 ) ? 1.0 : static_cast< double >( d ) ) );

	using type = ratio_t< NUM, DENOM >;
};

template< class T, class PERIOD >
class duration_t
{
public:
	duration_t()
	{
		ticks = 0;
	}

	duration_t( uint64_t _ticks )
	{
		ticks = _ticks;
	}

	inline T count() const
	{
		return ticks;
	}

	double ToSeconds() const
	{
		return ( ticks * PERIOD::r );
	}

	duration_t< T, PERIOD >& operator++()
	{
		++ticks;
		return *this;
	}

	duration_t< T, PERIOD >& operator+=( const duration_t< T, PERIOD >& rhs )
	{
		ticks += rhs.count();
		return *this;
	}

private:
	using rep		= T;
	using period	= typename PERIOD::type;

	T ticks;
};

template< class T, class PERIOD >
inline bool operator<( const duration_t< T, PERIOD >& lhs, const duration_t< T, PERIOD >& rhs )
{
	return ( lhs.count() < rhs.count() );
}

template< class T, class PERIOD >
inline duration_t< T, PERIOD > operator+( const duration_t< T, PERIOD >& lhs, const duration_t< T, PERIOD >& rhs )
{
	return ( lhs.count() + rhs.count() );
}

template< class T, class PERIOD >
inline duration_t< T, PERIOD > operator-( const duration_t< T, PERIOD >& lhs, const duration_t< T, PERIOD >& rhs )
{
	return ( lhs.count() - rhs.count() );
}

using nanosec_t		= ratio_t< 1, NanoToSeconds >;
using microsec_t	= ratio_t< 1, MicroToSeconds >;
using millisec_t	= ratio_t< 1, MilliToSeconds >;

using nano_t = duration_t< uint64_t, nanosec_t >;
using micro_t = duration_t< uint64_t, microsec_t >;
using milli_t = duration_t< uint64_t, millisec_t >;

using masterCycle_t = duration_t< uint64_t, ratio_t< 1,					MasterClockHz > >;
using ppuCycle_t	= duration_t< uint64_t, ratio_t< PpuClockDivide,	MasterClockHz > >;
using cpuCycle_t	= duration_t< uint64_t, ratio_t< CpuClockDivide,	MasterClockHz > >;
using apuCycle_t	= duration_t< uint64_t, ratio_t< ApuClockDivide,	MasterClockHz > >;
using apuSeqCycle_t = duration_t< uint64_t, ratio_t< ApuSequenceDivide, MasterClockHz> >;
using scanCycle_t	= duration_t< uint64_t, ratio_t< PpuCyclesPerScanline * PpuClockDivide, MasterClockHz> >; // TODO: Verify
using frameRate_t	= duration_t< double, std::ratio<1, FPS> >;
using timePoint_t	= std::chrono::time_point< std::chrono::steady_clock >;

static constexpr float CPU_HZ = ( MasterClockHz / CpuClockDivide );
static constexpr float APU_HZ = ( MasterClockHz / ApuClockDivide );
static constexpr float PPU_HZ = ( MasterClockHz / PpuClockDivide );

static const std::chrono::nanoseconds FrameLatencyNs = std::chrono::nanoseconds( 16666667 );
static const std::chrono::nanoseconds MaxFrameLatencyNs = 4 * FrameLatencyNs;

static inline masterCycle_t NanoToCycle( const nano_t& nanoseconds )
{
	return masterCycle_t( static_cast<uint64_t>( nanoseconds.ToSeconds() * MasterClockHz ) );
}

static inline nano_t CycleToNano( const masterCycle_t& cycle )
{
	return nano_t( static_cast<uint64_t>( cycle.ToSeconds() * NanoToSeconds ) );
}

static inline cpuCycle_t MasterToCpuCycle( const masterCycle_t& cycle )
{
	return cpuCycle_t( cycle.count() / CpuClockDivide );
}

static inline ppuCycle_t MasterToPpuCycle( const masterCycle_t& cycle )
{
	return ppuCycle_t( cycle.count() / PpuClockDivide );
}

static inline apuCycle_t MasterToApuCycle( const masterCycle_t& cycle )
{
	return apuCycle_t( cycle.count() / ApuClockDivide );
}

static inline apuCycle_t CpuToApuCycle( const cpuCycle_t& cycle )
{
	return apuCycle_t( cycle.count() / 2 );
}

static inline masterCycle_t CpuToMasterCycle( const cpuCycle_t& cycle )
{
	return masterCycle_t( cycle.count() * CpuClockDivide );
}
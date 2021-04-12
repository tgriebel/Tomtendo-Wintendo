#pragma once

#include <chrono>

const uint64_t MasterClockHz		= 21477272;
const uint64_t CpuClockDivide		= 12;
const uint64_t ApuClockDivide		= 24;
const uint64_t ApuSequenceDivide	= 89490;
const uint64_t PpuClockDivide		= 4;
const uint64_t PpuCyclesPerScanline	= 341;
const uint64_t FPS					= 60;
const uint64_t MinFPS				= 30;

using masterCycle_t	= std::chrono::duration< uint64_t, std::ratio<1, MasterClockHz> >;
using ppuCycle_t	= std::chrono::duration< uint64_t, std::ratio<PpuClockDivide, MasterClockHz> >;
using cpuCycle_t	= std::chrono::duration< uint64_t, std::ratio<CpuClockDivide, MasterClockHz> >;
using apuCycle_t	= std::chrono::duration< uint64_t, std::ratio<ApuClockDivide, MasterClockHz> >;
using apuSeqCycle_t	= std::chrono::duration< uint64_t, std::ratio<ApuSequenceDivide, MasterClockHz> >;
using scanCycle_t	= std::chrono::duration< uint64_t, std::ratio<PpuCyclesPerScanline* PpuClockDivide, MasterClockHz> >; // TODO: Verify
using frameRate_t	= std::chrono::duration< double, std::ratio<1, FPS> >;
using timePoint_t	= std::chrono::time_point< std::chrono::steady_clock >;

static constexpr uint64_t CPU_HZ = chrono::duration_cast<cpuCycle_t>( chrono::seconds( 1 ) ).count();
static constexpr uint64_t APU_HZ = chrono::duration_cast<apuCycle_t>( chrono::seconds( 1 ) ).count();
static constexpr uint64_t PPU_HZ = chrono::duration_cast<ppuCycle_t>( chrono::seconds( 1 ) ).count();

static const std::chrono::nanoseconds FrameLatencyNs = std::chrono::duration_cast<chrono::nanoseconds>( frameRate_t( 1 ) );
static const std::chrono::nanoseconds MaxFrameLatencyNs = 4 * FrameLatencyNs;

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

private:
	using rep		= T;
	using period	= typename PERIOD::type;

	T ticks;
};

using nanosec_t		= ratio_t< 1, NanoToSeconds >;
using microsec_t	= ratio_t< 1, MicroToSeconds >;
using millisec_t	= ratio_t< 1, MilliToSeconds >;

using nano_t = duration_t< uint64_t, nanosec_t >;
using micro_t = duration_t< uint64_t, microsec_t >;
using milli_t = duration_t< uint64_t, millisec_t >;

static inline masterCycle_t NanoToCycle( const nano_t& nanoseconds )
{
	return masterCycle_t( static_cast<uint64_t>( nanoseconds.ToSeconds() * MasterClockHz ) );
}

static inline cpuCycle_t MasterToCpuCycle( const masterCycle_t& cycle )
{
	return std::chrono::duration_cast<cpuCycle_t>( cycle );
}

static inline ppuCycle_t MasterToPpuCycle( const masterCycle_t& cycle )
{
	return std::chrono::duration_cast<ppuCycle_t>( cycle );
}

static inline apuCycle_t MasterToApuCycle( const masterCycle_t& cycle )
{
	return std::chrono::duration_cast<apuCycle_t>( cycle );
}
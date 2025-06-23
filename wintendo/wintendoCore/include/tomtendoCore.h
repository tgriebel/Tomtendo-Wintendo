
/*=======================================================
* BEGIN - base.h
*=======================================================*/
/*
* MIT License
*
* Copyright( c ) 2023 Thomas Griebel
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this softwareand associated documentation files( the "Software" ), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright noticeand this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#include <cstdint>

#ifdef STATIC_BUILD
#undef TOMTENDO_LIBRARY_EXPORTS
#endif

#ifdef TOMTENDO_LIBRARY_EXPORTS
#define EXPORT_DLL extern "C" __declspec(dllexport)
#define EXPORT_CLASS_DLL __declspec(dllexport)
#elif !defined(STATIC_BUILD)
#define EXPORT_DLL __declspec(dllimport)
#define EXPORT_CLASS_DLL __declspec(dllimport)
#else
#define EXPORT_DLL
#define EXPORT_CLASS_DLL
#endif

#ifdef IMPORT_WIN
#include <windows.h> 
#include <stdio.h>

#define RuntimeImportDllFunction( library, runtimeInterface, name ) runtimeInterface->##name = (runtimeDllInterface_t::PFN_##name)GetProcAddress( library, #name );
#endif
/*=======================================================
* END - base.h
*=======================================================*/

/*=======================================================
* BEGIN - command.h
*=======================================================*/
#pragma once
//#include "base.h"

namespace Tomtendo
{
	enum class sysCmdType_t
	{
		LOAD_STATE,
		SAVE_STATE,
		RECORD,
		REPLAY,
		START_TRACE,
		STOP_TRACE,
	};

	struct sysCmd_t
	{
		static const uint32_t MaxParms = 10;

		union parm_t
		{
			int64_t		i;
			double		f;
			uint64_t	u;
		};

		sysCmdType_t	type;
		parm_t			parms[ MaxParms ];
	};
};
/*=======================================================
* END - command.h
*=======================================================*/

/*=======================================================
* BEGIN - image.h
*=======================================================*/
#pragma once
//#include "base.h"

namespace Tomtendo
{
	struct RGBA
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;
	};

	union Pixel
	{
		uint8_t vec[ 4 ];
		uint32_t rawABGR; // ABGR format
		RGBA rgba;

		inline uint32_t AsHexColor()
		{
			Pixel abgr;
			abgr.rgba = { rgba.alpha, rgba.red, rgba.green, rgba.blue };
			return abgr.rawABGR;
		}
	};

	struct wtPoint
	{
		int32_t x;
		int32_t y;
	};

	struct wtRect
	{
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
	};

	class wtRawImageInterface
	{
	public:
		virtual void					SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel ) = 0;
		virtual void					Set( const uint32_t index, const Pixel value ) = 0;
		virtual const Pixel&			Get( const uint32_t index ) = 0;
		virtual void					Clear( const uint32_t r8g8b8a8 = 0 ) = 0;

		virtual const uint32_t* const	GetRawBuffer() const = 0;
		virtual uint32_t				GetWidth() const = 0;
		virtual uint32_t				GetHeight() const = 0;
		virtual uint32_t				GetBufferLength() const = 0;
		virtual const char*				GetDebugName() const = 0;
		virtual void					SetDebugName( const char* debugName ) = 0;
	};

	template< uint32_t N, uint32_t M >
	class wtRawImage : public wtRawImageInterface
	{
	public:

		wtRawImage()
		{
			Clear();
			name = "";
		}

		wtRawImage( const char* name_ )
		{
			Clear();
			name = name_;
		}

		wtRawImage& operator=( const wtRawImage& _image )
		{
			if( this != &_image )
			{
				for ( uint32_t i = 0; i < _image.length; ++i ) {
					buffer[ i ].rawABGR = _image.buffer[ i ].rawABGR;
				}
				name = _image.name;
			}
			return *this;
		}

		void SetPixel( const uint32_t x, const uint32_t y, const Pixel& pixel )
		{
			const uint32_t index = ( x + y * width );
			assert( index < length );

			if ( index < length )
			{
				buffer[ index ] = pixel;
			}
		}

		inline void Set( const uint32_t index, const Pixel value )
		{
			assert( index < length );

			//if ( index < length )
			{
				buffer[ index ] = value;
			}
		}

		const Pixel& Get( const uint32_t index )
		{
			assert( index < length );
			if ( index < length )
			{
				return buffer[ index ];
			}
			return buffer[ 0 ];
		}

		inline void SetDebugName( const char* debugName )
		{
			name = debugName;
		}

		void Clear( const uint32_t r8g8b8a8 = 0 )
		{
			for ( uint32_t i = 0; i < length; ++i )
			{
				buffer[ i ].rawABGR = r8g8b8a8;
			}
		}

		inline const uint32_t* const GetRawBuffer() const
		{
			return &buffer[ 0 ].rawABGR;
		}

		inline uint32_t GetWidth() const
		{
			return width;
		}

		inline uint32_t GetHeight() const
		{
			return height;
		}

		inline uint32_t GetBufferLength() const
		{
			return length;
		}

		inline const char* GetDebugName() const
		{
			return name;
		}

		inline wtRect GetRect()
		{
			return { 0, 0, width, height };
		}

	private:
		static const uint32_t width = N;
		static const uint32_t height = M;
		static const uint32_t length = N * M;
		Pixel buffer[ length ];
		const char* name;
	};

	using wtDisplayImage		= wtRawImage<256, 240>;
	using wtNameTableImage		= wtRawImage<2 * 256, 2 * 240>;
	using wtPaletteImage		= wtRawImage<16, 2>;
	using wtPatternTableImage	= wtRawImage<128, 128>;
	using wt16x8ChrImage		= wtRawImage<8, 16>;
	using wt8x8ChrImage			= wtRawImage<8, 8>;
};
/*=======================================================
* END - image.h
*=======================================================*/

/*=======================================================
* BEGIN - input.h
*=======================================================*/
#pragma once
//#include "base.h"

#include "../../stdafx.h"
#include <stdint.h>
#include <map>

namespace Tomtendo
{
	enum class ControllerId : uint8_t
	{
		CONTROLLER_0 = 0X00,
		CONTROLLER_1 = 0X01,
		CONTROLLER_2 = 0X02,
		CONTROLLER_3 = 0X03,
		CONTROLLER_COUNT,
		CONTROLLER_INVALID,
	};

	enum class ButtonFlags : uint8_t
	{
		BUTTON_NONE = 0X00,

		BUTTON_RIGHT = 0X01,
		BUTTON_LEFT = 0X02,
		BUTTON_DOWN = 0X04,
		BUTTON_UP = 0X08,

		BUTTON_START = 0X10,
		BUTTON_SELECT = 0X20,
		BUTTON_B = 0X40,
		BUTTON_A = 0X80,
	};

	struct mouse_t
	{
		int32_t x;
		int32_t y;
	};

	inline ButtonFlags operator |( const ButtonFlags lhs, const ButtonFlags rhs )
	{
		return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) | static_cast<uint8_t>( rhs ) );
	}

	inline ButtonFlags operator &( const ButtonFlags lhs, const ButtonFlags rhs )
	{
		return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) & static_cast<uint8_t>( rhs ) );
	}

	inline ButtonFlags operator >>( const ButtonFlags lhs, const ButtonFlags rhs )
	{
		return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) >> static_cast<uint8_t>( rhs ) );
	}

	inline ButtonFlags operator <<( const ButtonFlags lhs, const ButtonFlags rhs )
	{
		return static_cast<ButtonFlags>( static_cast<uint8_t>( lhs ) << static_cast<uint8_t>( rhs ) );
	}

	struct EXPORT_CLASS_DLL keyBinding_t
	{
		ControllerId	controllerId	= ControllerId::CONTROLLER_INVALID;
		ButtonFlags		buttonFlags		= ButtonFlags::BUTTON_NONE;
	};

	struct EXPORT_CLASS_DLL Input
	{
		ButtonFlags		keyBuffer[ 2 ];
		mouse_t			mousePoint;
		keyBinding_t	keyMap[ 256 ];
	};

	EXPORT_DLL ButtonFlags	GetKeyBuffer( const Input* input, const ControllerId controllerId );
	EXPORT_DLL mouse_t		GetMouse( const Input* input );
	EXPORT_DLL void			BindKey( Input* input, const char key, const ControllerId controllerId, const ButtonFlags button );
	EXPORT_DLL void			StoreKey( Input* input, const uint32_t key );
	EXPORT_DLL void			ReleaseKey( Input* input, const uint32_t key );
	EXPORT_DLL void			StoreMouseClick( Input* input, const int32_t x, const int32_t y );
	EXPORT_DLL void			ClearMouseClick( Input* input );
};
/*=======================================================
* END - input.h
*=======================================================*/

/*=======================================================
* BEGIN - log.h
*=======================================================*/
#pragma once

#pragma once
//#include "base.h"

#include <vector>
#include <string>

namespace Tomtendo
{
	struct regDebugInfo_t
	{
		uint8_t			X;
		uint8_t			Y;
		uint8_t			A;
		uint8_t			SP;
		uint8_t			P;
		uint16_t		PC;
	};


	class OpDebugInfo
	{
	public:

		regDebugInfo_t	regInfo;
		const char*		mnemonic;
		int32_t			curScanline;
		uint64_t		cpuCycles;
		uint64_t		ppuCycles;
		uint64_t		instrCycles;

		uint32_t		loadCnt;
		uint32_t		storeCnt;

		uint16_t		instrBegin;
		uint16_t		address;
		uint16_t		offset;
		uint16_t		targetAddress;

		uint8_t			opType;
		uint8_t			addrMode;
		uint8_t			memValue;
		uint8_t			byteCode;

		uint8_t			operands;
		uint8_t			op0;
		uint8_t			op1;

		bool			isIllegal;
		bool			irq;
		bool			nmi;
		bool			oam;

		OpDebugInfo()
		{
			loadCnt = 0;
			storeCnt = 0;

			opType = 0;
			addrMode = 0;
			memValue = 0;
			address = 0;
			offset = 0;
			targetAddress = 0;
			instrBegin = 0;
			curScanline = 0;
			cpuCycles = 0;
			ppuCycles = 0;
			instrCycles = 0;

			op0 = 0;
			op1 = 0;

			isIllegal = false;
			irq = false;
			nmi = false;
			oam = false;

			mnemonic = "";
			operands = 0;
			byteCode = 0;

			regInfo = { 0, 0, 0, 0, 0, 0 };
		}

		void ToString( std::string& buffer, const bool registerDebug = true, const bool cycleDebug = true ) const;
	};

	using logFrame_t = std::vector<OpDebugInfo>;
	using logRecords_t = std::vector<logFrame_t>;

	class wtLog
	{
	private:
		logRecords_t		log;
		uint32_t			frameIx;
		uint32_t			totalCount;

	public:
		wtLog() : frameIx( 0 ), totalCount( 0 )
		{
			Reset( 1 );
		}

		void				Reset( const uint32_t targetCount );
		void				NewFrame();
		OpDebugInfo&		NewLine();
		const logFrame_t&	GetLogFrame( const uint32_t frameIx ) const;
		OpDebugInfo&		GetLogLine();
		uint32_t			GetRecordCount() const;
		bool				IsFull() const;
		bool				IsFinished() const;
		void				ToString( std::string& buffer, const uint32_t frameBegin, const uint32_t frameEnd, const bool registerDebug = true ) const;
	};
};
/*=======================================================
* END - log.h
*=======================================================*/

/*=======================================================
* BEGIN - playback.h
*=======================================================*/
#pragma once
//#include "base.h"
#include <stdint.h>

namespace Tomtendo
{
	enum class replayStateCode_t : uint8_t
	{
		LIVE,
		RECORD,
		REPLAY,
		FINISHED,
	};

	struct playbackState_t
	{
		replayStateCode_t	replayState;
		int64_t				startFrame;
		int64_t				currentFrame;
		int64_t				finalFrame;
		bool				pause;
	};
};
/*=======================================================
* END - playback.h
*=======================================================*/

/*=======================================================
* BEGIN - serializer.h
*=======================================================*/
#pragma once
//#include "base.h"
#include <stdio.h>
#include <string>
#include <sstream>

#define DBG_SERIALIZER 0
#define _SAVE_

namespace Tomtendo
{
	enum class serializeMode_t
	{
		STORE,
		LOAD,
	};

	union serializerTuple_t
	{
		struct b8_t
		{
			uint8_t		e[8];
		} b8;

		struct b16_t
		{
			uint16_t	e[4];
		} b16;

		struct b32_t
		{
			uint32_t	e[2];
		} b32;

		uint64_t b64;
	};


	struct serializerHeader_t
	{
		static const uint32_t MaxSections = 128;
		static const uint32_t MaxNameLength = 128;

		struct section_t
		{
			char		name[ MaxNameLength ];
			uint32_t	offset;
			uint32_t	size;
		};

		section_t	sections[ MaxSections ];
		uint32_t	sectionCount;
	};


	class Serializer
	{
	public:
	
		Serializer( const uint32_t _sizeInBytes, serializeMode_t _mode )
		{
			bytes = new uint8_t[ _sizeInBytes ];
			byteCount = _sizeInBytes;
			mode = _mode;
			Clear();
		}

		~Serializer()
		{
			if( bytes != nullptr ) {
				delete[] bytes;
			}
			byteCount = 0;
			mode = serializeMode_t::LOAD;
			SetPosition( 0 );
		}

		Serializer() = delete;
		Serializer( const Serializer& ) = delete;
		Serializer operator=( const Serializer& ) = delete;

		uint8_t*			GetPtr();
		void				SetPosition( const uint32_t index );
		void				Clear();
		uint32_t			CurrentSize() const;
		uint32_t			BufferSize() const;
		bool				CanStore( const uint32_t sizeInBytes ) const;
		void				SetMode( serializeMode_t mode );
		serializeMode_t		GetMode() const;

		uint32_t			NewLabel( const char name[ serializerHeader_t::MaxNameLength ] );
		void				EndLabel( const char name[ serializerHeader_t::MaxNameLength ] );
		bool				FindLabel( const char name[ serializerHeader_t::MaxNameLength ], serializerHeader_t::section_t** outSection );

		bool				NextBool( bool& v );
		bool				NextChar( int8_t& v );
		bool				NextUchar( uint8_t& v );
		bool				NextShort( int16_t& v );
		bool				NextUshort( uint16_t& v );
		bool				NextInt( int32_t& v );
		bool				NextUint( uint32_t& v );
		bool				NextLong( int64_t& v );
		bool				NextUlong( uint64_t& v );
		bool				NextFloat( float& v );
		bool				NextDouble( double& v );

		bool				Next8b( uint8_t& b8 );
		bool				Next16b( uint16_t& b16 );
		bool				Next32b( uint32_t& b32 );
		bool				Next64b( uint64_t& b64 );
		bool				NextArray( uint8_t* b8, uint32_t sizeInBytes );

	private:
		serializerHeader_t	header;
		uint8_t*			bytes;
		uint32_t			byteCount;
		uint32_t			index;
		serializeMode_t		mode;
	public: // FIXME: temp
		std::stringstream	dbgText;
	};
};
/*=======================================================
* END - serializer.h
*=======================================================*/

/*=======================================================
* BEGIN - time.h
*=======================================================*/
#pragma once
//#include "base.h"
#include <chrono>

namespace Tomtendo
{
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
};
/*=======================================================
* END - time.h
*=======================================================*/

/*=======================================================
* BEGIN - timer.h
*=======================================================*/
#pragma once
//#include "base.h"
#include <chrono>

namespace Tomtendo
{
	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Reset()
		{
			startTimeNs = std::chrono::nanoseconds( 0 );
			endTimeNs = std::chrono::nanoseconds( 0 );
			totalTimeNs = std::chrono::nanoseconds( 0 );
		}

		void Start()
		{
			startTimeNs = std::chrono::system_clock::now().time_since_epoch();
			endTimeNs = startTimeNs;
		}

		void Stop()
		{
			endTimeNs = std::chrono::system_clock::now().time_since_epoch();
			totalTimeNs += ( endTimeNs - startTimeNs );
			startTimeNs = endTimeNs;
		}

		double GetElapsedNs()
		{
			endTimeNs = std::chrono::system_clock::now().time_since_epoch();
			return static_cast<double>( totalTimeNs.count() + ( endTimeNs - startTimeNs ).count() );
		}

		double GetElapsedUs()
		{
			return ( GetElapsedNs() / 1000.0f );
		}

		double GetElapsedMs()
		{
			return ( GetElapsedUs() / 1000.0f );
		}

	private:
		std::chrono::nanoseconds startTimeNs;
		std::chrono::nanoseconds endTimeNs;
		std::chrono::nanoseconds totalTimeNs;
	};
};
/*=======================================================
* END - timer.h
*=======================================================*/

/*=======================================================
* BEGIN - util.h
*=======================================================*/
#pragma once
//#include "base.h"
#include <stdint.h>
#include "assert.h"

namespace Tomtendo
{
	template < uint16_t B >
	class BitCounter
	{
	public:
		BitCounter() {
			Reload();
		}

		void Inc() {
			bits++;
		}

		void Dec() {
			bits--;
		}

		void Reload( uint16_t value = 0 )
		{
			bits = value;
			unused = 0;
		}

		uint16_t Value() {
			return bits;
		}

		bool IsZero() {
			return ( Value() == 0 );
		}
	private:
		uint16_t bits : B;
		uint16_t unused : ( 16 - B );
	};


	template< uint16_t B >
	class wtShiftReg
	{
	public:

		wtShiftReg()
		{
			Clear();
		}

		void Shift( const bool bitValue )
		{
			reg >>= 1;
			reg &= LMask;
			reg |= ( static_cast<uint32_t>( bitValue ) << ( Bits - 1 ) ) & ~LMask;

			++shifts;
		}

		uint32_t GetShiftCnt() const
		{
			return shifts;
		}

		bool IsFull() const
		{
			return ( shifts >= Bits );
		}

		bool GetBitValue( const uint16_t bit ) const
		{
			return ( ( reg >> bit ) & 0x01 );
		}

		uint32_t GetValue() const
		{
			return reg & Mask;
		}

		void Set( const uint32_t value )
		{
			reg = value;
			shifts = 0;
		}

		void Clear()
		{
			reg = 0;
			shifts = 0;
		}

	private:
		uint32_t reg;
		uint32_t shifts;
		static const uint16_t Bits = B;

		static constexpr uint32_t CalcMask()
		{
			uint32_t mask = 0x02;
			for ( uint32_t i = 1; i < B; ++i )
			{
				mask <<= 1;
			}
			return ( mask - 1 );
		}

		static const uint32_t Mask = CalcMask();
		static const uint32_t LMask = ( Mask >> 1 );
		static const uint32_t RMask = ~0x01ul;
	};


	template< typename T, uint32_t SIZE >
	class wtBuffer
	{
	public:
		void Clear()
		{
			Reset();
			for ( uint32_t i = 0; i < SIZE; ++i )
			{
				samples[ i ] = 0;
			}
		}

		void Reset()
		{
			currentIndex = 0;
			memset( samples, 0xFF, sizeof( samples[ 0 ] ) * SIZE );
		}

		void Write( const T& sample )
		{
			samples[ currentIndex ] = sample;
			currentIndex++;
			assert( currentIndex <= SIZE );
		}

		float Read( const uint32_t index ) const
		{
			assert( index <= SIZE );
			return samples[ index ];
		}

		const T* GetRawBuffer()
		{
			return &samples[ 0 ];
		}

		uint32_t GetSampleCnt() const
		{
			return currentIndex;
		}

		bool IsFull() const
		{
			return ( currentIndex == SIZE );
		}

	private:
		T			samples[ SIZE ];
		uint32_t	currentIndex;
	};


	template< typename T, uint32_t SIZE >
	class wtQueue
	{
	public:
		wtQueue()
		{
			Reset();
		}

		float Peek( const uint32_t sampleIx ) const
		{
			if( sampleIx >= GetSampleCnt() ) {
				return 0.0f;
			}

			const int32_t index = ( begin + sampleIx ) % SIZE;
			return samples[ index ];
		}

		void EnqueFIFO( const T& sample )
		{
			if ( IsFull() ) {
				Deque();
			}
			Enque( sample );
		}

		void Enque( const T& sample )
		{
			if ( IsFull() )
				return;

			if ( begin == -1 )
			{
				begin = 0;
			}

			end = ( end + 1 ) % SIZE;
			samples[ end ] = sample;
		}

		float Deque()
		{
			if ( IsEmpty() )
				return 0.0f;

			const float retValue = samples[ begin ];
			samples[ begin ] = 0;
			if ( begin == end )
			{
				begin = -1;
				end = -1;
			}
			else
			{
				begin = ( begin + 1 ) % SIZE;
			}

			return retValue;
		}

		bool IsEmpty() const
		{
			return ( GetSampleCnt() == 0 );
		}

		bool IsFull()
		{
			return ( GetSampleCnt() == ( SIZE - 1 ) );
		}

		uint32_t GetSampleCnt() const
		{
			if ( ( begin == -1 ) || ( end == -1 ) )
			{
				return 0;
			}
			else if ( end >= begin )
			{
				return ( ( end - begin ) + 1 );
			}
			else
			{
				return ( SIZE - ( begin - end ) + 1 );
			}
		}

		void Reset()
		{
			begin = -1;
			end = -1;
		}

	private:
		T		samples[ SIZE ];
		int32_t	begin;
		int32_t	end;
	};
};
/*=======================================================
* END - util.h
*=======================================================*/

/*=======================================================
* BEGIN - interface.h
*=======================================================*/
#pragma once

//#include "base.h"
//#include "input.h"
//#include "command.h"
//#include "playback.h"
//#include "time.h"
//#include "timer.h"
//#include "util.h"
//#include "image.h"
//#include "serializer.h"
//#include "log.h"

class wtSystem;

namespace Tomtendo
{
	class wtLog;
	struct config_t;
	struct wtFrameResult;

	struct Emulator
	{
		wtSystem*	system = nullptr;
		Input		input;
	};

	EXPORT_DLL bool	Boot( Emulator* emu, const wchar_t* filePath, const uint32_t resetVectorManual = 0x10000 );
	EXPORT_DLL void	Shutdown( Emulator* emu );
	EXPORT_DLL int	RunEpoch( Emulator* emu, const std::chrono::nanoseconds& runCycles );
	EXPORT_DLL void	GetFrameResult( Emulator* emu, wtFrameResult& outFrameResult );
	EXPORT_DLL void	SetConfig( Emulator* emu, config_t& cfg );

	EXPORT_DLL void	SubmitCommand( Emulator* emu, const sysCmd_t& cmd );

	EXPORT_DLL void	UpdateDebugImages( Emulator* emu );
	EXPORT_DLL void	GenerateRomDissambly( Emulator* emu, std::string prgRomAsm[ 128 ] );
	EXPORT_DLL void	GenerateChrRomTables( Emulator* emu, wtPatternTableImage chrRom[ 32 ] );

	static const char* STATE_MEMORY_LABEL = "Memory";
	static const char* STATE_VRAM_LABEL	= "VRAM";

	EXPORT_DLL void CreateEmulatorInstance( Emulator** emulatorInstance );

	EXPORT_DLL void DestroyEmulatorInstance( Emulator** emulatorInstance );

	EXPORT_DLL uint32_t	ScreenWidth();

	EXPORT_DLL uint32_t	ScreenHeight();

	EXPORT_DLL uint32_t	SpriteLimit();

#ifdef IMPORT_WIN
	struct runtimeDllInterface_t
	{
		typedef void( __cdecl* PFN_CreateEmulatorInstance )( Emulator** emulatorInstance );
		PFN_CreateEmulatorInstance CreateEmulatorInstance = nullptr;

		typedef void( __cdecl* PFN_DestroyEmulatorInstance )( Emulator** emulatorInstance );
		PFN_DestroyEmulatorInstance DestroyEmulatorInstance = nullptr;

		typedef bool( __cdecl* PFN_Boot )( Emulator* emu, const wchar_t* filePath, const uint32_t resetVectorManual );
		PFN_Boot Boot = nullptr;

		typedef void( __cdecl* PFN_Shutdown )( Emulator* emu );
		PFN_Shutdown Shutdown;

		typedef int( __cdecl* PFN_RunEpoch )( Emulator* emu, const std::chrono::nanoseconds& runCycles );
		PFN_RunEpoch RunEpoch = nullptr;

		typedef void( __cdecl* PFN_GetFrameResult )( Emulator* emu, wtFrameResult& outFrameResult );
		PFN_GetFrameResult GetFrameResult;

		typedef uint32_t( __cdecl* PFN_ScreenWidth )( void );
		PFN_ScreenWidth ScreenWidth = nullptr;

		typedef uint32_t( __cdecl* PFN_ScreenHeight )( void );
		PFN_ScreenHeight ScreenHeight = nullptr;

		typedef uint32_t( __cdecl* PFN_SpriteLimit )( void );
		PFN_SpriteLimit SpriteLimit = nullptr;

		typedef Tomtendo::config_t( __cdecl* PFN_DefaultConfig )( );
		PFN_DefaultConfig DefaultConfig = nullptr;

		typedef void( __cdecl* PFN_SetConfig )( Emulator* emu, config_t& cfg );
		PFN_SetConfig SetConfig = nullptr;

		typedef void( __cdecl* PFN_SubmitCommand )( Emulator* emu, const sysCmd_t& cmd );
		PFN_SubmitCommand SubmitCommand = nullptr;

		typedef void( __cdecl* PFN_UpdateDebugImages )( Emulator* emu );
		PFN_UpdateDebugImages UpdateDebugImages = nullptr;

		typedef void( __cdecl* PFN_GenerateRomDissambly )( Emulator* emu, std::string prgRomAsm[ 128 ] );
		PFN_GenerateRomDissambly GenerateRomDissambly = nullptr;

		typedef void( __cdecl* PFN_GenerateChrRomTables )( Emulator* emu, wtPatternTableImage chrRom[ 32 ] );
		PFN_GenerateChrRomTables GenerateChrRomTables = nullptr;

		typedef void( __cdecl* PFN_GetKeyBuffer )( const Input* input, const ControllerId controllerId );
		PFN_GetKeyBuffer GetKeyBuffer = nullptr;

		typedef void( __cdecl* PFN_GetMouse )( const Input* input );
		PFN_GetMouse GetMouse = nullptr;

		typedef void( __cdecl* PFN_BindKey )( Input* input, const char key, const ControllerId controllerId, const ButtonFlags button );
		PFN_BindKey BindKey = nullptr;

		typedef void( __cdecl* PFN_StoreKey )( Input* input, const uint32_t key );
		PFN_StoreKey StoreKey = nullptr;

		typedef void( __cdecl* PFN_ReleaseKey )( Input* input, const uint32_t key );
		PFN_ReleaseKey ReleaseKey = nullptr;

		typedef void( __cdecl* PFN_StoreMouseClick )( Input* input, const int32_t x, const int32_t y );
		PFN_StoreMouseClick StoreMouseClick = nullptr;

		typedef void( __cdecl* PFN_ClearMouseClick )( Input* input );
		PFN_ClearMouseClick ClearMouseClick = nullptr;
	};

	void LoadDllInterface( runtimeDllInterface_t* dllInterface, HINSTANCE libInstance )
	{
		assert( dllInterface != nullptr );
		assert( libInstance != nullptr );

		if ( ( dllInterface == nullptr ) || ( libInstance == nullptr ) ) {
			return;
		}

		// Global
		RuntimeImportDllFunction( libInstance, dllInterface, DefaultConfig );
		RuntimeImportDllFunction( libInstance, dllInterface, ScreenWidth );
		RuntimeImportDllFunction( libInstance, dllInterface, ScreenHeight );
		RuntimeImportDllFunction( libInstance, dllInterface, SpriteLimit );

		// Init
		RuntimeImportDllFunction( libInstance, dllInterface, CreateEmulatorInstance );
		RuntimeImportDllFunction( libInstance, dllInterface, DestroyEmulatorInstance );

		// Interface 
		RuntimeImportDllFunction( libInstance, dllInterface, Boot );
		RuntimeImportDllFunction( libInstance, dllInterface, Shutdown );
		RuntimeImportDllFunction( libInstance, dllInterface, RunEpoch );
		RuntimeImportDllFunction( libInstance, dllInterface, GetFrameResult );	
		RuntimeImportDllFunction( libInstance, dllInterface, SetConfig );
		RuntimeImportDllFunction( libInstance, dllInterface, SubmitCommand );
		RuntimeImportDllFunction( libInstance, dllInterface, UpdateDebugImages );
		RuntimeImportDllFunction( libInstance, dllInterface, GenerateRomDissambly );
		RuntimeImportDllFunction( libInstance, dllInterface, GenerateChrRomTables );

		// Input
		RuntimeImportDllFunction( libInstance, dllInterface, GetKeyBuffer );
		RuntimeImportDllFunction( libInstance, dllInterface, GetMouse );
		RuntimeImportDllFunction( libInstance, dllInterface, BindKey );
		RuntimeImportDllFunction( libInstance, dllInterface, StoreKey );
		RuntimeImportDllFunction( libInstance, dllInterface, ReleaseKey );
		RuntimeImportDllFunction( libInstance, dllInterface, StoreMouseClick );
		RuntimeImportDllFunction( libInstance, dllInterface, ClearMouseClick );
	}
#endif

	enum analogMode_t
	{
		NTSC,
		PAL,
		ANALOG_MODE_COUNT,
	};

	enum class emulationFlags_t : uint32_t
	{
		NONE			= 1 << 0,
		CLAMP_FPS		= 1 << 1,
		LIMIT_STALL		= 1 << 2,
		HEADLESS		= 1 << 3,
		ALL				= 0xFFFFFFFF,
	};

	inline uint32_t operator&( emulationFlags_t lhs, emulationFlags_t rhs )
	{
		return ( static_cast<uint32_t>( lhs ) & static_cast<uint32_t>( rhs ) );
	}

	struct EXPORT_CLASS_DLL config_t
	{
		struct System
		{
			emulationFlags_t	flags;
		} sys;

		//struct CPU
		//{
		//} cpu;

		struct APU
		{
			float				volume;
			float				frequencyScale;
			int32_t				waveShift;
			bool				disableSweep;
			bool				disableEnvelope;
			bool				mutePulse1;
			bool				mutePulse2;
			bool				muteTri;
			bool				muteNoise;
			bool				muteDMC;
			uint8_t				dbgChannelBits;
		} apu;

		struct PPU
		{
			int32_t				chrPalette;
			int32_t				spriteLimit;
			bool				showBG;
			bool				showSprite;
		} ppu;
	};

	EXPORT_DLL config_t DefaultConfig();

	struct debugTiming_t
	{
		uint32_t		frameTimeUs;
		uint32_t		totalTimeUs;
		uint32_t		simulationTimeUs;
		uint32_t		realTimeUs;
		uint64_t		frameNumber;
		uint64_t		framePerRun;
		uint64_t		runInvocations;
		masterCycle_t	cycleBegin;
		masterCycle_t	cycleEnd;
		masterCycle_t	stateCycle;
	};

	struct cpuDebug_t
	{
		uint8_t			X;
		uint8_t			Y;
		uint8_t			A;
		uint8_t			SP;
		uint8_t			P;
		bool			carry;
		bool			zero;
		bool			interrupt;
		bool			decimal;
		bool			unused;
		bool			brk;
		bool			overflow;
		bool			negative;
		uint16_t		PC;
		uint16_t		resetVector;
		uint16_t		nmiVector;
		uint16_t		irqVector;
	};

	struct apuPulseDebug_t
	{
		int		duty;
		bool	constant;
		int		volume;
		int		counterHalt;
		int		timer;
		int		counter;
		int		period;
		int		sweepDelta;
		bool	sweepEnabled;
		int		sweepPeriod;
		int		sweepShift;
		int		sweepNegate;

	};

	struct apuTriangleDebug_t
	{
		int lengthCounter;
		int linearCounter;
		int timer;
		int reg4008_halt;
		int reg4008_load;
		int reg400A_timer;
		int reg400B_counter;
	};

	struct apuNoiseDebug_t
	{
		int shifter;
		int timer;
		int reg400C_halt;
		int reg400C_constant;
		int reg400C_volume;
		int reg400E_mode;
		int reg400E_period;
		int reg400E_length;
	};

	struct apuDmcDebug_t
	{
		int outputLevel;
		int sampleBuffer;
		int bitCount;
		int bytesRemaining;
		int period;
		int periodCounter;
		float frequency;

		int reg4010_Loop;
		int reg4010_Freq;
		int reg4010_Irq;
		int reg4011_load;
		int reg4012_addr;
		int reg4013_length;
	};

	struct apuDebug_t
	{
		apuPulseDebug_t		pulse1;
		apuPulseDebug_t		pulse2;
		apuNoiseDebug_t		noise;
		apuTriangleDebug_t	triangle;
		apuDmcDebug_t		dmc;

		bool				pulse1Enabled;
		bool				pulse2Enabled;
		bool				triangleEnabled;
		bool				noiseEnabled;
		bool				dmcEnabled;
		uint32_t			halfClkTicks;
		uint32_t			quarterClkTicks;
		uint32_t			irqClkEvents;
		cpuCycle_t			frameCounterTicks;
		cpuCycle_t			cycle;
		apuCycle_t			apuCycle;
	};

	struct ppuDebug_t
	{
		struct pickedSprite_t
		{
			uint8_t	x;
			uint8_t	y;
			uint8_t	tileId;
			uint8_t	palette;
			uint8_t	oamIndex;
			uint8_t	secondaryOamIndex;	// debugging
			uint8_t	tableId;			// debugging
			uint8_t	flippedHorizontal;
			uint8_t	flippedVertical;
			uint8_t	priority;
			bool	sprite0;
			bool	is8x16;
		} picked;
	};

	static constexpr uint32_t	ApuSamplesPerSec = static_cast<uint32_t>( CPU_HZ + 1 );
	static constexpr uint32_t	ApuBufferMs = static_cast<uint32_t>( 1000.0f / MinFPS );
	static constexpr uint32_t	ApuBufferSize = static_cast<uint32_t>( ApuSamplesPerSec * ( ApuBufferMs / 1000.0f ) );
	using wtSampleQueue = wtQueue< float, ApuBufferSize >;
	using wtSoundBuffer = wtBuffer< float, ApuBufferSize >;

	struct apuOutput_t
	{
		wtSampleQueue	dbgMixed;
		wtSampleQueue	dbgPulse1;
		wtSampleQueue	dbgPulse2;
		wtSampleQueue	dbgTri;
		wtSampleQueue	dbgNoise;
		wtSampleQueue	dbgDmc;
		wtSampleQueue	mixed;
	};

	struct stateHeader_t
	{
		uint8_t* memory;
		uint8_t* vram;
		uint32_t memorySize;
		uint32_t vramSize;
	};

	class wtStateBlob
	{
	public:

		wtStateBlob()
		{
			bytes = nullptr;
			byteCount = 0;
			cycle = masterCycle_t( 0 );
		}

		~wtStateBlob()
		{
			Reset();
		}

		bool		IsValid() const;
		uint32_t	GetBufferSize() const;

		uint8_t*	GetPtr();
		void		Set( Serializer& s, const masterCycle_t sysCycle );
		void		WriteTo( Serializer& s ) const;
		void		Reset();

		stateHeader_t	header;
	private:
		uint8_t*		bytes;
		uint32_t		byteCount;
		masterCycle_t	cycle;
	};

	enum wtMirrorMode : uint8_t
	{
		MIRROR_MODE_SINGLE,
		MIRROR_MODE_HORIZONTAL,
		MIRROR_MODE_VERTICAL,
		MIRROR_MODE_FOURSCREEN,
		MIRROR_MODE_SINGLE_LO,
		MIRROR_MODE_SINGLE_HI,
		MIRROR_MODE_COUNT
	};

	// TODO: bother with endianness?
	struct wtRomHeader
	{
		uint8_t type[ 3 ];
		uint8_t magic;
		uint8_t prgRomBanks;
		uint8_t chrRomBanks;
		struct ControlsBits0
		{
			uint8_t mirror : 1;
			uint8_t usesBattery : 1;
			uint8_t usesTrainer : 1;
			uint8_t fourScreenMirror : 1;
			uint8_t mapperNumberLower : 4;
		} controlBits0;
		struct ControlsBits1
		{
			uint8_t reserved0 : 4;
			uint8_t mappedNumberUpper : 4;
		} controlBits1;
		uint8_t reserved[ 8 ];
	};

	struct wtFrameResult
	{
		uint64_t					currentFrame;
		uint64_t					stateCount;
		playbackState_t				playbackState;
		wtDisplayImage*				frameBuffer;
		apuOutput_t*				soundOutput;
		wtStateBlob*				frameState;

		// Debug
		debugTiming_t				dbgInfo;
		wtRomHeader					romHeader;
		wtMirrorMode				mirrorMode;
		uint32_t					mapperId;
		uint64_t					dbgFrameBufferIx;
		uint64_t					frameToggleCount;
		wtNameTableImage*			nameTableSheet;
		wtPaletteImage*				paletteDebug;
		wtPatternTableImage*		patternTable0;
		wtPatternTableImage*		patternTable1;
		wt16x8ChrImage*				pickedObj8x16;
		cpuDebug_t					cpuDebug;
		apuDebug_t					apuDebug;
		ppuDebug_t					ppuDebug;
		wtLog*						dbgLog;
	};
};
/*=======================================================
* END - interface.h
*=======================================================*/

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

#include <cstdint>
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
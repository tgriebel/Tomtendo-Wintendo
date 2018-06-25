#pragma once


struct PPU
{
	static const half VirtualMemorySize = 0xFFFF;
	static const half PhysicalMemorySize = 0x4000; // Wrap at this point

	bool nmiOccurred;

	ppuCycle_t cycle;

	inline void Step( const ppuCycle_t targetCycles )
	{
		/*
		Start of vertical blanking : Set NMI_occurred in PPU to true.
		End of vertical blanking, sometime in pre - render scanline : Set NMI_occurred to false.
		Read PPUSTATUS : Return old status of NMI_occurred in bit 7, then set NMI_occurred to false.
		Write to PPUCTRL : Set NMI_output to bit 7.
		The PPU pulls / NMI low if and only if both NMI_occurred and NMI_output are true.By toggling NMI_output( PPUCTRL.7 ) during vertical blank without reading PPUSTATUS, a program can cause / NMI to be pulled low multiple times, causing multiple NMIs to be generated.
		*/
	}
/*
	PPU()
	{
		cycle = ppuCycle_t( 0 );
	}*/
};
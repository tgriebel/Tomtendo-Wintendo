#include "renderer_d3d12.h"
#include "audioEngine.h"

using frameSampleBuffer = wtQueue< float, 500 >;
static frameSampleBuffer frameTimePlot;

static float ImGuiGetWtQueueSample( void* data, int32_t idx )
{
	frameSampleBuffer* queue = reinterpret_cast<frameSampleBuffer*>( data );
	return queue->Peek( idx );
}

void wtRenderer::BuildImguiCommandList()
{
#ifdef IMGUI_ENABLE
	wtSystem&		nesSystem		= *app->system;
	config_t&		systemConfig	= app->systemConfig;
	wtAppDebug_t&	debugData		= app->debugData;
	wtAudioEngine&	audio			= *app->audio;

	ThrowIfFailed( cmd.imguiCommandAllocator[ currentFrame ]->Reset() );
	ThrowIfFailed( cmd.imguiCommandList[ currentFrame ]->Reset( cmd.imguiCommandAllocator[ currentFrame ].Get(), pipeline.pso.Get() ) );

	cmd.imguiCommandList[ currentFrame ]->SetGraphicsRootSignature( pipeline.rootSig.Get() );

	ID3D12DescriptorHeap* ppHeaps[] = { pipeline.cbvSrvUavHeap.Get() };
	cmd.imguiCommandList[ currentFrame ]->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
	cmd.imguiCommandList[ currentFrame ]->SetGraphicsRootDescriptorTable( 0, pipeline.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart() );

	cmd.imguiCommandList[ currentFrame ]->RSSetViewports( 1, &view.viewport );
	cmd.imguiCommandList[ currentFrame ]->RSSetScissorRects( 1, &view.scissorRect );

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrame ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
	cmd.imguiCommandList[ currentFrame ]->ResourceBarrier( 1, &barrier );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame, swapChain.rtvDescriptorSize );
	cmd.imguiCommandList[ currentFrame ]->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	static int32_t playbackFrame = 0;
	static bool autoPlayback = false;

	if ( ImGui::BeginTabBar( "Debug Info" ) )
	{
		bool systemTabOpen = true;
		if ( ImGui::BeginTabItem( "System", &systemTabOpen ) )
		{
			if ( ImGui::CollapsingHeader( "Registers", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Columns( 2 );
				ImGui::Text( "A: %i",			fr->cpuDebug.A );
				ImGui::Text( "X: %i",			fr->cpuDebug.X );
				ImGui::Text( "Y: %i",			fr->cpuDebug.Y );
				ImGui::Text( "PC: %i",			fr->cpuDebug.PC );
				ImGui::Text( "SP: %i",			fr->cpuDebug.SP );
				ImGui::NextColumn();
				ImGui::Text( "Status Flags" );
				ImGui::Text( "Carry: %i",		fr->cpuDebug.P.bit.c );
				ImGui::Text( "Zero: %i",		fr->cpuDebug.P.bit.z );
				ImGui::Text( "Interrupt: %i",	fr->cpuDebug.P.bit.i );
				ImGui::Text( "Decimal: %i",		fr->cpuDebug.P.bit.d );
				ImGui::Text( "Unused: %i",		fr->cpuDebug.P.bit.u );
				ImGui::Text( "Break: %i",		fr->cpuDebug.P.bit.b );
				ImGui::Text( "Overflow: %i",	fr->cpuDebug.P.bit.v );
				ImGui::Text( "Negative: %i",	fr->cpuDebug.P.bit.n );
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "ROM Info", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Columns( 2 );
				ImGui::Text( "PRG ROM Banks: %i",	fr->romHeader.prgRomBanks );
				ImGui::Text( "CHR ROM Banks: %i",	fr->romHeader.chrRomBanks );
				ImGui::Text( "Mirror Mode: %i",		fr->mirrorMode );
				ImGui::Text( "Mapper ID: %i",		fr->mapperId );
				ImGui::Text( "Battery: %i",			fr->romHeader.controlBits0.usesBattery );
				ImGui::NextColumn();
				ImGui::Text( "Trainer: %i",			fr->romHeader.controlBits0.usesTrainer );
				ImGui::Text( "Four Screen: %i",		fr->romHeader.controlBits0.fourScreenMirror );
				ImGui::Text( "Reset Vector: %X",	fr->cpuDebug.resetVector );
				ImGui::Text( "NMI Vector: %X",		fr->cpuDebug.nmiVector );
				ImGui::Text( "IRQ Vector: %X",		fr->cpuDebug.irqVector );
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "ASM", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				for ( int i = 0; i < fr->romHeader.prgRomBanks; ++i )
				{
					if ( debugData.prgRomAsm[ i ].empty() )
						continue;

					char treeNodeName[ wtAppDebug_t::MaxPrgRomBanks ];
					sprintf_s( treeNodeName, wtAppDebug_t::MaxPrgRomBanks, "prgBankText_%i", i );
					if ( ImGui::TreeNode( treeNodeName, "PRG Bank %i", i ) )
					{
						if ( ImGui::Button( "Copy" ) ) {
							ToClipboard( debugData.prgRomAsm[ i ] );
						}
						ImGui::TextWrapped( debugData.prgRomAsm[ i ].c_str() );
						ImGui::TreePop();
					}
				}
			}

			if ( ImGui::CollapsingHeader( "Trace Log", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static int32_t frameCount = 0;
				if ( ImGui::Button( "Start" ) ) {
					sysCmd_t traceCmd;
					traceCmd.type			= sysCmdType_t::START_TRACE;
					traceCmd.parms[ 0 ].u	= max( 0, frameCount );
					nesSystem.SubmitCommand( traceCmd );
					app->traceLog.clear();
					app->logUnpacked = false;
				}
				ImGui::SameLine();
				ImGui::InputInt( "Frame Count:", &frameCount );
				if ( app->traceLog.size() > 0 )
				{
					ImGui::Columns( 2 );
					const bool isOpen = ImGui::TreeNode( "Log" );
					ImGui::NextColumn();
					if ( ImGui::Button( "Copy" ) ) {
						ToClipboard( app->traceLog );
					}
					if ( isOpen ) {
						ImGui::Columns( 1 );
						ImGui::TextWrapped( app->traceLog.c_str() );
						ImGui::TreePop();
						ImGui::Columns( 2 );
					}
				}
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "Timing", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Text( "Frame Cycle Start: %u",	fr->dbgInfo.cycleBegin );
				ImGui::Text( "Frame Cycle End: %u",		fr->dbgInfo.cycleEnd );
			}

			if ( ImGui::CollapsingHeader( "Controls", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				if ( ImGui::Button( "Load State" ) )
				{
					sysCmd_t traceCmd;
					traceCmd.type = sysCmdType_t::LOAD_STATE;
					nesSystem.SubmitCommand( traceCmd );
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Save State" ) )
				{
					sysCmd_t traceCmd;
					traceCmd.type = sysCmdType_t::SAVE_STATE;
					nesSystem.SubmitCommand( traceCmd );
				}

				if ( ImGui::Button( "Record" ) )
				{
					sysCmd_t traceCmd;
					traceCmd.type			= sysCmdType_t::RECORD;
					traceCmd.parms[ 0 ].i	= -1;
					nesSystem.SubmitCommand( traceCmd );
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Rewind" ) )
				{
					sysCmd_t traceCmd;
					traceCmd.type			= sysCmdType_t::REPLAY;
					traceCmd.parms[ 0 ].i	= 0;
					traceCmd.parms[ 1 ].u	= true;
					nesSystem.SubmitCommand( traceCmd );

					autoPlayback = false;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Play" ) )
				{
					playbackFrame = 0;
					autoPlayback = true;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Stop" ) )
				{
					sysCmd_t traceCmd;
					traceCmd.type			= sysCmdType_t::REPLAY;
					traceCmd.parms[ 0 ].i	= playbackFrame;
					traceCmd.parms[ 1 ].u	= true;
					nesSystem.SubmitCommand( traceCmd );
					autoPlayback = false;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Next" ) ) {
					++playbackFrame;
					sysCmd_t traceCmd;
					traceCmd.type			= sysCmdType_t::REPLAY;
					traceCmd.parms[ 0 ].i	= playbackFrame;
					traceCmd.parms[ 1 ].u	= true;
					nesSystem.SubmitCommand( traceCmd );
				}

				if ( fr->stateCount > 0 )
				{
					int playPercent = (int)( 100.0f * playbackFrame / (float)fr->stateCount );
					ImGui::SliderInt( "", &playPercent, 0, 100 );
					ImGui::SameLine();
					ImGui::Text( "%i", fr->stateCount );
				}
			}

			if ( ImGui::CollapsingHeader( "Physical Memory", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static MemoryEditor cpuMemEdit;
				cpuMemEdit.DrawContents( fr->memDebug.cpuMemory, memDebug_t::CpuMemorySize, 0 );
			}

			ImGui::EndTabItem();
		}

		bool ppuTabOpen = true;
		if ( ImGui::BeginTabItem( "PPU", &ppuTabOpen ) )
		{
			if ( ImGui::CollapsingHeader( "Frame Buffer", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				const uint32_t imageId = 0;
				const wtRawImageInterface* srcImage = &fr->frameBuffer;
				ImGui::Image( (ImTextureID)textureResources[ currentFrame ][ imageId ].gpuHandle.ptr, ImVec2( (float)srcImage->GetWidth(), (float)srcImage->GetHeight() ) );
				ImGui::Text( "%.3f ms/frame (%.1f FPS)",		fr->dbgInfo.frameTimeUs / 1000.0f, 1000000.0f / fr->dbgInfo.frameTimeUs );
				ImGui::Text( "Display Frame #%i",				frameNumber );
				ImGui::Text( "Emulator Frame #%i",				fr->dbgInfo.frameNumber );
				ImGui::Text( "Emulator Run Invocations #%i",	fr->dbgInfo.runInvocations );
				ImGui::Text( "Avg Frames Emulated %f",			fr->dbgInfo.framePerRun / (float)fr->dbgInfo.runInvocations );
				ImGui::Text( "Copy time: %4.2f ms",				app->t.copyTime );
				ImGui::Text( "Time since last copy: %4.2f ms",	app->t.elapsedCopyTime );
				ImGui::Text( "Copy size: %i MB",				sizeof( *fr ) / 1024 / 1024 );

				frameTimePlot.EnqueFIFO( fr->dbgInfo.frameTimeUs / 1000.0f );

				ImGui::PlotLines( "", &ImGuiGetWtQueueSample,
					reinterpret_cast<void*>( &frameTimePlot ),
					frameTimePlot.GetSampleCnt(),
					0,
					NULL,
					0.0f,
					17.0f,
					ImVec2( 0, 0 ) );
			}

			if ( ImGui::CollapsingHeader( "Name Table", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static float ntScale = 1.0f;
				ImGui::SliderFloat( "Scale", &ntScale, 0.1f, 10.0f );
				const uint32_t imageId = 1;
				const wtRawImageInterface* srcImage = &fr->nameTableSheet;
				ImGui::Image( (ImTextureID)textureResources[ currentFrame ][ imageId ].gpuHandle.ptr, ImVec2( ntScale * srcImage->GetWidth(), ntScale * srcImage->GetHeight() ) );
			}

			if ( ImGui::CollapsingHeader( "Pattern Table", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static float ptScale = 2.0f;
				static int patternTableSelect = 0;
				ImGui::SliderFloat( "Scale", &ptScale, 0.1f, 10.0f );
				ImGui::Columns( 2 );
				const wtAppTextureD3D12& ptrn0 = textureResources[ currentFrame ][ SHADER_RESOURES_PATTERN0 ];
				ImGui::Image( (ImTextureID)ptrn0.gpuHandle.ptr, ImVec2( ptScale * ptrn0.width, ptScale * ptrn0.height ) );
				ImGui::NextColumn();
				const wtAppTextureD3D12& ptrn1 = textureResources[ currentFrame ][ SHADER_RESOURES_PATTERN1 ];
				ImGui::Image( (ImTextureID)ptrn1.gpuHandle.ptr, ImVec2( ptScale * ptrn1.width, ptScale * ptrn1.height ) );
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "CHR ROM", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				if ( ImGui::InputInt( "ChrPalette", &systemConfig.ppu.chrPalette, 1, 1, ImGuiInputTextFlags_None ) )
				{
					systemConfig.ppu.chrPalette = max( -1, min( 7, systemConfig.ppu.chrPalette ) );
					app->refreshChrRomTables = true;
				}

				ImGui::Columns( 4 );
				for ( int i = 0; i < fr->romHeader.chrRomBanks; ++i )
				{
					const uint32_t imageId = SHADER_RESOURES_CHRBANK0 + i;
					const wtRawImageInterface* srcImage = &debugData.chrRom[ i ];
					ImGui::Image( (ImTextureID)textureResources[ currentFrame ][ imageId ].gpuHandle.ptr, ImVec2( (float)srcImage->GetWidth(), (float)srcImage->GetHeight() ) );
					ImGui::NextColumn();
				}
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "Palette", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				const uint32_t imageId = 2;
				const wtRawImageInterface* srcImage = &fr->paletteDebug;
				ImGui::Image( (ImTextureID)textureResources[ currentFrame ][ imageId ].gpuHandle.ptr, ImVec2( 10.0f * srcImage->GetWidth(), 10.0f * srcImage->GetHeight() ) );
			}

			if ( ImGui::CollapsingHeader( "Controls", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Show BG",			&systemConfig.ppu.showBG );
				ImGui::Checkbox( "Show Sprite",		&systemConfig.ppu.showSprite );
				ImGui::SliderInt( "Line Sprites",	&systemConfig.ppu.spriteLimit, 1, PPU::TotalSprites );
			}

			if ( ImGui::CollapsingHeader( "Picked Object", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				const uint32_t imageId = 5;
				const wtRawImageInterface* srcImage = &fr->pickedObj8x16;

				ImGui::Columns( 3 );
				ImGui::Text( "X: %i",				fr->ppuDebug.spritePicked.x );
				ImGui::Text( "Y: %i",				fr->ppuDebug.spritePicked.y );
				ImGui::Text( "Palette: %i",			fr->ppuDebug.spritePicked.palette );
				ImGui::Text( "Priority: %i",		fr->ppuDebug.spritePicked.priority >> 2 );
				ImGui::NextColumn();
				ImGui::Text( "Flipped X: %i",		fr->ppuDebug.spritePicked.flippedHorizontal );
				ImGui::Text( "Flipped Y: %i",		fr->ppuDebug.spritePicked.flippedVertical );
				ImGui::Text( "Tile ID: %i",			fr->ppuDebug.spritePicked.tileId );
				ImGui::Text( "OAM Index: %i",		fr->ppuDebug.spritePicked.oamIndex );
				ImGui::Text( "2nd OAM Index: %i",	fr->ppuDebug.spritePicked.secondaryOamIndex );
				ImGui::NextColumn();
				ImGui::Image( (ImTextureID)textureResources[ currentFrame ][ imageId ].gpuHandle.ptr, ImVec2( 4.0f * srcImage->GetWidth(), 4.0f * srcImage->GetHeight() ) );
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "NT Memory", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static MemoryEditor ppuMemEdit;
				ppuMemEdit.DrawContents( fr->memDebug.ppuMemory, memDebug_t::PpuMemorySize );
			}
			ImGui::EndTabItem();
		}

		bool apuTabOpen = true;
		if ( ImGui::BeginTabItem( "APU", &apuTabOpen ) )
		{
			apuDebug_t& apuDebug = fr->apuDebug;
			ImGui::Checkbox( "Play Sound", &app->audio->enableSound );
			const float maxVolume = 2 * systemConfig.apu.volume;
			if ( ImGui::CollapsingHeader( "Pulse 1", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute",					&systemConfig.apu.mutePulse1 );
				ImGui::SameLine();
				ImGui::Text( "| $4015 - Enabled: %i",		apuDebug.status.sem.p1 );
				ImGui::Columns( 2 );
				ImGui::Text( "$4000 - Duty: %i",			apuDebug.pulse1.regCtrl.sem.duty );
				ImGui::Text( "$4000 - Constant: %i",		apuDebug.pulse1.regCtrl.sem.isConstant );
				ImGui::Text( "$4000 - Volume: %i",			apuDebug.pulse1.regCtrl.sem.volume );
				ImGui::Text( "$4000 - Halt Flag: %i",		apuDebug.pulse1.regCtrl.sem.counterHalt );
				ImGui::Text( "$4002 - Timer: %i",			apuDebug.pulse1.regTune.sem0.timer );
				ImGui::Text( "$4003 - Counter: %i",			apuDebug.pulse1.regTune.sem0.counter );
				ImGui::NextColumn();
				ImGui::Text( "Pulse Period: %i",			apuDebug.pulse1.period.Value() );
				ImGui::Text( "Sweep Delta: %i",				abs( apuDebug.pulse1.period.Value() - apuDebug.pulse1.regTune.sem0.timer ) );
				ImGui::Text( "$4001 - Sweep Enabled: %i",	apuDebug.pulse1.regRamp.sem.enabled );
				ImGui::Text( "$4001 - Sweep Period: %i",	apuDebug.pulse1.regRamp.sem.period );
				ImGui::Text( "$4001 - Sweep Shift: %i",		apuDebug.pulse1.regRamp.sem.shift );
				ImGui::Text( "$4001 - Sweep Negate: %i",	apuDebug.pulse1.regRamp.sem.negate );
				ImGui::Columns( 1 );
				ImGui::PlotHistogram( "Wave", GetQueueSample, &fr->soundOutput.dbgPulse1, fr->soundOutput.dbgPulse1.GetSampleCnt(), 0, "", -15, 15, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Pulse 2", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute",					&systemConfig.apu.mutePulse2 );
				ImGui::SameLine();
				ImGui::Text( "| $4015 - Enabled: %i",		apuDebug.status.sem.p2 );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i",					fr->soundOutput.dbgPulse2.GetSampleCnt() );
				ImGui::Text( "$4004 - Duty: %i",			apuDebug.pulse2.regCtrl.sem.duty );
				ImGui::Text( "$4004 - Constant: %i",		apuDebug.pulse2.regCtrl.sem.isConstant );
				ImGui::Text( "$4004 - Reg Volume: %i",		apuDebug.pulse2.regCtrl.sem.volume );
				ImGui::Text( "$4004 - Halt Flag: %i",		apuDebug.pulse2.regCtrl.sem.counterHalt );
				ImGui::Text( "$4006 - Timer: %i",			apuDebug.pulse2.regTune.sem0.timer );
				ImGui::Text( "$4007 - Counter: %i",			apuDebug.pulse2.regTune.sem0.counter );
				ImGui::NextColumn();
				ImGui::Text( "Pulse Period: %i",			apuDebug.pulse2.period.Value() );
				ImGui::Text( "Sweep Delta: %i",				abs( apuDebug.pulse2.period.Value() - apuDebug.pulse2.regTune.sem0.timer ) );
				ImGui::Text( "$4005 - Sweep Enabled: %i",	apuDebug.pulse2.regRamp.sem.enabled );
				ImGui::Text( "$4005 - Sweep Period: %i",	apuDebug.pulse2.regRamp.sem.period );
				ImGui::Text( "$4005 - Sweep Shift: %i",		apuDebug.pulse2.regRamp.sem.shift );
				ImGui::Text( "$4005 - Sweep Negate: %i",	apuDebug.pulse2.regRamp.sem.negate );
				ImGui::Columns( 1 );
				ImGui::PlotHistogram( "Wave", GetQueueSample, &fr->soundOutput.dbgPulse2, fr->soundOutput.dbgPulse2.GetSampleCnt(), 0, "", -15, 15, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Triangle", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute",					&systemConfig.apu.muteTri );
				ImGui::SameLine();
				ImGui::Text( "| $4015 - Enabled: %i",		apuDebug.status.sem.t );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i",					fr->soundOutput.dbgTri.GetSampleCnt() );
				ImGui::Text( "Length Counter: %i",			apuDebug.triangle.lengthCounter );
				ImGui::Text( "Linear Counter: %i",			apuDebug.triangle.linearCounter.Value() );
				ImGui::Text( "Timer: %i",					apuDebug.triangle.timer.Value() );
				ImGui::NextColumn();
				ImGui::Text( "$4008 - Halt Flag: %i",		apuDebug.triangle.regLinear.sem.counterHalt );
				ImGui::Text( "$4008 - Counter Load: %i",	apuDebug.triangle.regLinear.sem.counterLoad );
				ImGui::Text( "$400A - Timer: %i",			apuDebug.triangle.regTimer.sem0.timer );
				ImGui::Text( "$400B - Counter: %i",			apuDebug.triangle.regTimer.sem0.counter );
				ImGui::Columns( 1 );
				ImGui::PlotHistogram( "Wave", GetQueueSample, &fr->soundOutput.dbgTri, fr->soundOutput.dbgTri.GetSampleCnt(), 0, "", -30, 30, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Noise", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute",					&systemConfig.apu.muteNoise );
				ImGui::SameLine();
				ImGui::Text( "| $4015 - Enabled: %i",		apuDebug.status.sem.n );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i",					fr->soundOutput.dbgNoise.GetSampleCnt() );
				ImGui::Text( "Shifter: %i",					apuDebug.noise.shift.Value() );
				ImGui::Text( "Timer: %i",					apuDebug.noise.timer );
				ImGui::NextColumn();
				ImGui::Text( "$400C - Halt Flag: %i",		apuDebug.noise.regCtrl.sem.counterHalt );
				ImGui::Text( "$400C - Constant Volume: %i",	apuDebug.noise.regCtrl.sem.isConstant );
				ImGui::Text( "$400C - Volume: %i",			apuDebug.noise.regCtrl.sem.volume );
				ImGui::Text( "$400E - Mode: %i",			apuDebug.noise.regFreq1.sem.mode );
				ImGui::Text( "$400E - Period: %i",			apuDebug.noise.regFreq1.sem.period );
				ImGui::Text( "$400F - Length Counter: %i",	apuDebug.noise.regFreq2.sem.length );
				ImGui::Columns( 1 );
				ImGui::PlotHistogram( "Wave", GetQueueSample, &fr->soundOutput.dbgNoise, fr->soundOutput.dbgNoise.GetSampleCnt(), 0, "", -30, 30, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "DMC", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute",					&systemConfig.apu.muteDMC );
				ImGui::SameLine();
				ImGui::Text( "| $4015 - Enabled: %i",		apuDebug.status.sem.d );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i",					fr->soundOutput.dbgDmc.GetSampleCnt() );
				ImGui::Text( "Volume: %i",					apuDebug.dmc.outputLevel.Value() );
				ImGui::Text( "Sample Buffer: %i",			apuDebug.dmc.sampleBuffer );
				ImGui::Text( "Bit Counter: %i",				apuDebug.dmc.bitCnt );
				ImGui::Text( "Bytes Remaining: %i",			apuDebug.dmc.bytesRemaining );
				ImGui::Text( "Period: %i",					apuDebug.dmc.period );
				ImGui::Text( "Period Counter: %i",			apuDebug.dmc.periodCounter );
				ImGui::Text( "Frequency: %i",				CPU_HZ / apuDebug.dmc.period );
				ImGui::NextColumn();
				ImGui::Text( "$4010 - Loop: %i",			apuDebug.dmc.regCtrl.sem.loop );
				ImGui::Text( "$4010 - Freq: %i",			apuDebug.dmc.regCtrl.sem.freq );
				ImGui::Text( "$4010 - IRQ: %i",				apuDebug.dmc.regCtrl.sem.irqEnable );
				ImGui::Text( "$4011 - Load Counter: %i",	apuDebug.dmc.regLoad );
				ImGui::Text( "$4012 - Addr: %X",			apuDebug.dmc.regAddr );
				ImGui::Text( "$4013 - Length: %i",			apuDebug.dmc.regLength );
				ImGui::Columns( 1 );
				ImGui::PlotHistogram( "Wave", GetQueueSample, &fr->soundOutput.dbgDmc, fr->soundOutput.dbgDmc.GetSampleCnt(), 0, "", -30, 30, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Frame Counter", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Text( "Half Clock Ticks: %i",		fr->apuDebug.halfClkTicks );
				ImGui::Text( "Quarter Clock Ticks: %i",		fr->apuDebug.quarterClkTicks );
				ImGui::Text( "IRQ Events: %i",				fr->apuDebug.irqClkEvents );
				ImGui::Text( "Cycle: %i",					fr->apuDebug.cycle.count() );
				ImGui::Text( "Apu Cycle: %i",				fr->apuDebug.apuCycle.count() );
				ImGui::Text( "Frame Counter Cycle: %i",		fr->apuDebug.frameCounterTicks.count() );
			}
			if ( ImGui::CollapsingHeader( "Controls", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::InputFloat( "Volume",				&systemConfig.apu.volume, 1.0f, 10.0f, 2, ImGuiInputTextFlags_None );
				ImGui::InputFloat( "Frequency",				&systemConfig.apu.frequencyScale, 1.0f, 10.0f, 2, ImGuiInputTextFlags_None );
				ImGui::InputInt( "Pulse Wave Shift",		&systemConfig.apu.waveShift );
				ImGui::Checkbox( "Disable Sweep",			&systemConfig.apu.disableSweep );
				ImGui::Checkbox( "Disable Envelope",		&systemConfig.apu.disableEnvelope );
			}
			if ( ImGui::CollapsingHeader( "Filters", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::InputFloat( "HighPass 1 Q:",		&app->audio->Q1 );
				ImGui::InputFloat( "HighPass 2 Q:",		&app->audio->Q2 );
				ImGui::InputFloat( "LowPass Q:",		&app->audio->Q3 );
				ImGui::InputFloat( "HighPass 1 Freq:",	&app->audio->F1 );
				ImGui::InputFloat( "HighPass 2 Freq:",	&app->audio->F2 );
				ImGui::InputFloat( "LowPass Freq:",		&app->audio->F3 );
			}

			ImGui::PlotLines( "Audio Wave",				app->audio->dbgSoundFrameData.GetRawBuffer(), ApuBufferSize, 0, "", -32768.0f, 32768.0f, ImVec2( 1000.0f, 100.0f ) );
			ImGui::Text( "Sample Length: %i",			app->audio->dbgLastSoundSampleLength );
			ImGui::Text( "Target MS: %4.2f",			1000.0f * app->audio->dbgLastSoundSampleLength / (float)wtAudioEngine::SourceFreqHz );
			ImGui::Text( "Average MS: %4.2f",			voiceCallback.totalDuration / voiceCallback.processedQueues );
			ImGui::Text( "Time since last submit: %i",	(int)app->t.audioSubmitTime );
			ImGui::Text( "Queues: %i",					app->audio->audioState.BuffersQueued );
			ImGui::Text( "Queues Started: %i",			voiceCallback.totalQueues );
			ImGui::EndTabItem();
		}

		bool displayTabOpen = true;
		if ( ImGui::BeginTabItem( "Display Settings", &displayTabOpen ) )
		{
			ImGui::Checkbox( "Enable",			&pipeline.shaderData.enable );
			ImGui::SliderFloat( "Hard Pix",		&pipeline.shaderData.hardPix, -4.0f, -2.0f );
			ImGui::SliderFloat( "Hard Scan",	&pipeline.shaderData.hardScan, -16.0f, -8.0f );
			ImGui::SliderFloat( "Warp X",		&pipeline.shaderData.warp.x, 0.0f, 1.0f / 8.0f );
			ImGui::SliderFloat( "Warp Y",		&pipeline.shaderData.warp.y, 0.0f, 1.0f / 8.0f );
			ImGui::SliderFloat( "Mask Light",	&pipeline.shaderData.maskLight, 0.0f, 2.0f );
			ImGui::SliderFloat( "Mask Dark",	&pipeline.shaderData.maskDark, 0.0f, 2.0f );
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	if ( autoPlayback )
	{
		if ( ( (int32_t)app->t.runTime.GetElapsedMs() % 8 ) == 0 )
		{
			sysCmd_t traceCmd;
			traceCmd.type = sysCmdType_t::REPLAY;
			traceCmd.parms[ 0 ].i = playbackFrame;
			nesSystem.SubmitCommand( traceCmd );

			++playbackFrame;
		}
	}
	playbackFrame = min( playbackFrame, static_cast<int32_t>( fr->stateCount ) );

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), cmd.imguiCommandList[ currentFrame ].Get() );

	barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrame ].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
	cmd.imguiCommandList[ currentFrame ]->ResourceBarrier( 1, &barrier );

	ThrowIfFailed( cmd.imguiCommandList[ currentFrame ]->Close() );
#endif
}
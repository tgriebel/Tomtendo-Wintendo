#pragma once

#include "stdafx.h"
#include "common.h"

union PulseCtrl
{
	struct PulseCtrlSemantic
	{
		uint8_t b0 : 1;
		uint8_t b1 : 1;
		uint8_t b2 : 1;
		uint8_t b3 : 1;

		uint8_t b4 : 1;
		uint8_t b5 : 1;
		uint8_t b6 : 1;
		uint8_t b7 : 1;
	} sem;

	uint8_t raw;
};

union PulseRamp
{
	struct PulseRampSemantic
	{
		uint8_t b0 : 1;
		uint8_t b1 : 1;
		uint8_t b2 : 1;
		uint8_t b3 : 1;

		uint8_t b4 : 1;
		uint8_t b5 : 1;
		uint8_t b6 : 1;
		uint8_t b7 : 1;
	} sem;

	uint8_t raw;
};

union TuneCtrl
{
	struct TuneCtrlSemantic
	{
		uint8_t b0 : 8;
		uint8_t b1 : 5;
		uint8_t b2 : 3;
	} sem;

	uint8_t lower;
	uint8_t upper;
	uint16_t raw;
};

union TriangleCtrl
{
	struct TriangleCtrlSemantic
	{
		uint8_t b0 : 1;
		uint8_t b1 : 1;
		uint8_t b2 : 1;
		uint8_t b3 : 1;

		uint8_t b4 : 1;
		uint8_t b5 : 1;
		uint8_t b6 : 1;
		uint8_t b7 : 1;
	} sem;

	uint8_t raw;
};

union NoiseCtrl
{
	struct NoiseCtrlSemantic
	{
		uint8_t b0 : 1;
		uint8_t b1 : 1;
		uint8_t b2 : 1;
		uint8_t b3 : 1;

		uint8_t b4 : 1;
		uint8_t b5 : 1;
		uint8_t b6 : 1;
		uint8_t b7 : 1;
	} sem;

	uint8_t raw;
};
/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#pragma once
#include "fftw3.h"

class FFTComputer
{
public:
	enum Size
	{
		FFT_64		= 64,
		FFT_128		= 128,
		FFT_256		= 256,
		FFT_512		= 512,
		FFT_1024	= 1024,
		FFT_2048	= 2048,
		FFT_4096	= 4096,
		FFT_8192	= 8192
	};
	enum Type
	{
		FFT_Forward		= 0,
		FFT_Backward	= 1
	};

	FFTComputer(const Size aSize, const Type aType);
	~FFTComputer();

	void computeFFT(const float* aInput, float* aOutput);
	const float* fftDatas() const;
	int size() const;

	void setSize(const int aSize);
    void setType(const FFTComputer::Type type);

private:
	void initialize();

private:
	fftwf_plan		mPlan;
	//float*			mInOut;
	float*			mInput;
	float*			mOutput;

	int				mSize;
	Type			mType;
};

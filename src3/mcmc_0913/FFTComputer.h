/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
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

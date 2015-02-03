#include "FFTComputer.h"

#include <cmath>
#include <cstdlib>
#include <cstring>


FFTComputer::FFTComputer(const FFTComputer::Size aSize, const FFTComputer::Type aType):
mSize((int)aSize),
mType(aType)
{
	initialize();
}

FFTComputer::~FFTComputer()
{
	fftwf_destroy_plan(mPlan);
	//fftwf_free(mInOut);
	fftwf_free(mInput);
	fftwf_free(mOutput);
}

void FFTComputer::initialize()
{
	// The data is an array of type fftw_complex, which is by default a float[2] 
	// composed of the real (in[i][0]) and imaginary (in[i][1]) parts of a complex number. 
	// It is recommended to use fftw_malloc, which behaves like malloc except that it properly aligns the array
	// when SIMD instructions (such as SSE and Altivec) are available (see SIMD alignment and fftw_malloc). 
	
	//mInOut = (float*) fftwf_malloc(2 * (mSize/2 + 1) * sizeof(float));
	mInput = (float*) fftwf_malloc(mSize * sizeof(float));
	mOutput = (float*) fftwf_malloc(2 * (mSize/2 + 1) * sizeof(float));
	
	// Create a plan, which is an object that contains 
	// all the data that FFTW needs to compute the FFT.
	// Using FFTW_MEASURE overwites the input datas so make sure to initialize them later!
	
	switch(mType)
	{
		case FFT_Forward:	{mPlan = fftwf_plan_dft_r2c_1d(mSize, mInput, (fftwf_complex*)mOutput, FFTW_ESTIMATE); break;}	// FFTW_FORWARD
		case FFT_Backward:	{mPlan = fftwf_plan_dft_c2r_1d(mSize, (fftwf_complex*)mOutput, mInput, FFTW_ESTIMATE); break;}	// FFTW_BACKWARD
		default:			{break;}
	}
}
void FFTComputer::setSize(const int aSize)
{
	if(mSize != aSize)
	{
		fftwf_destroy_plan(mPlan);
		//fftwf_free(mInOut);
		fftwf_free(mInput);
		fftwf_free(mOutput);
        
		mSize = aSize;
		initialize();
	}
}

void FFTComputer::setType(const FFTComputer::Type type)
{
	if(mType != type)
	{
		fftwf_destroy_plan(mPlan);
		//fftwf_free(mInOut);
		fftwf_free(mInput);
		fftwf_free(mOutput);
        
		mType = type;
		initialize();
	}
}

const float* FFTComputer::fftDatas() const {return mOutput;}
int FFTComputer::size() const {return mSize;}

void FFTComputer::computeFFT(const float* aInput, float* aOutput)
{
	for(int i=0; i<mSize; ++i)
	{
		mInput[i] = aInput[i] * sinf(3.141592f * i / (float)mSize);
	}
	
	// Execute by using the plan as many times as you like for transforms on the specified in/out arrays, 
	// computing the actual transforms via the following function.
	// If you want to transform a different array of the same size, 
	// you can create a new plan with fftw_plan_dft_1d and FFTW automatically reuses the information from the previous plan, if possible.

	fftwf_execute(mPlan);
	
	for(int i=0; i<(mSize/2 + 1); ++i)
	{
		const float lModule = (mOutput[2*i] * mOutput[2*i] + mOutput[2*i+1] * mOutput[2*i+1]) / (mSize * mSize);
		aOutput[i] = 10.f * log10f(lModule);
	}
	memcpy(mOutput, aOutput, (mSize/2 + 1) * sizeof(float));
}


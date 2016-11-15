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


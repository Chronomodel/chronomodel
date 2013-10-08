#pragma once

enum Control_DisplayMode
{
	eControl_Linear			= 0,
	eControl_Logarithmic	= 1
};
enum Control_Unit
{
	eControl_Generic		= 0,
	eControl_Percent		= 1,
	eControl_Hertz			= 2,
	eControl_Decibels		= 3,
	eControl_Seconds		= 4
};
inline const char* unitText(const Control_Unit aUnit)
{
	switch(aUnit)
	{
		case eControl_Generic:	{return "";}
		case eControl_Percent:	{return "%";}
		case eControl_Hertz:	{return "Hz";}
		case eControl_Decibels:	{return "dB";}
		case eControl_Seconds:	{return "s";}
		default:				{return "";}
	}
}

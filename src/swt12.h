//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//---------------------------------------------------------------------------
//
// Swt12.H - Include file for Switch Library
// Version  2.00
//
#include "ownet.h"
// Typedefs
typedef struct tagSRAM
{
	uchar Supply;
	uchar Chan_B;
	uchar Chan_A;
	uchar Chan_Sel;
	uchar Sour_Sel;
	uchar Polarity;
} SwitchProps;

// Local subroutines
int ReadSwitch12(int device,int ClearActivity);
int SetSwitch12(int device, SwitchProps State);
int WriteSwitch12(int device, ushort st) ;
uchar SwitchSetLatchState12(int chan, int latchState, int doSmart, uchar state) ;
int SwitchStateToString12(int infobyte,char *outstr);

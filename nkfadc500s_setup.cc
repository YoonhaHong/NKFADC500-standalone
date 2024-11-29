#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>

#include "TROOT.h"
#include "NoticeNKFADC500ROOT.h"
using namespace std;

#define SID 1

int main(int argc, char** argv){

    const int nCh = 4;
    usb3comroot *usb = new usb3comroot;
    usb->USB3Init(0);


    // define KFADC500 class
    NKNKFADC500 *nkfadc = new NKNKFADC500;

    // open KFADC500
    nkfadc->NKFADC500open(SID, 0);


    FILE* fp = fopen("setup.txt" ,"rt");
	if ( fp == NULL ){
		cout<<"Can not open setup file!"<<endl;
        return 0;
    }
	// NKFADC500 setting: every timing setting unit is ns
	// --------------------------------------------------
	unsigned long fRL = 4;
    nkfadc->NKFADC500write_RL(SID, fRL);
	// Recording length 
	// ns scale: 1=128, 2=256, and 4=512
	// us scale: 8=1, 16=2, 32=4, 64=8, 128=16, and 256=32

	unsigned long fTLT;
    fscanf(fp, "%x", &fTLT);
	// Trigger lookup table value, any: FFFE, 1&3&2&4: 8000, 1&2|3&4: F888, 1&3|2&4: ECA0
	// 1 must be fired: AAAA
	// 2 must be fired: CCCC
	// 1|2: EEEE
	// 1&2: 8888
	// 3&4: F000

	unsigned long fTHR[nCh];
	fscanf(fp, "%ld %ld %ld %ld", fTHR[0]+0, fTHR[0]+1, fTHR[0]+2, fTHR[0]+3);
	// Discrimination threshold
	    // 1 ~ 4095 for pulse height threshold: 4,095 vs. 2 V (dynamic range) -> 100 ~ 50 mV 
        // mV = 0.6 x ADC

	unsigned long fDLY[nCh];
	fscanf(fp, "%ld %ld %ld %ld", fDLY[0]+0, fDLY[0]+1, fDLY[0]+2, fDLY[0]+3);
	// ADC waveform delay from trigger point: 0 ~ 31992
		// By default +80 will be added as intrinsic delay of device (e.g. 64 = 64 + 80)
		// Give the number can be devided by 2 (e.g., 48 for ~50, 72 for ~75)

	unsigned long fPOL[nCh];	  
	fscanf(fp, "%ld %ld %ld %ld", fPOL[0]+0, fPOL[0]+1, fPOL[0]+2, fPOL[0]+3);
    // Input pulse polarity: 0 = negative, 1 = positive

	unsigned long fDACOFF[nCh]; 
	fscanf(fp, "%ld %ld %ld %ld", fDACOFF[0]+0, fDACOFF[0]+1, fDACOFF[0]+2, fDACOFF[0]+3);
    // ADC offset value: 0 ~ 4095

	unsigned long fTM_PC   = 1; // When 1 enables pulse count trigger 
	unsigned long fTM_PW   = 0; // When 1 enables pulse width trigger
	unsigned long fTM_PS   = 0; // When 1 enables peak sum trigger
	unsigned long fTM_PSOR = 0; // When 1 enables peak sum OR trigger
	unsigned long fTM = (fTM_PSOR<<3) | (fTM_PS<<2) | (fTM_PW<<1) | fTM_PC;
    for(int Ch=0; Ch<4; Ch++) nkfadc->NKFADC500write_TM(SID, Ch + 1, tmode);
	// Set trigger mode

    unsigned long fPTRIG = 0; 
    nkfadc->NKFADC500write_PTRIG(SID, fPTRIG);
    // pedestal trigger interval in ms, 0 for disable

    unsigned long fSCALE = 1;         // prescale = 1 ~ 65535
    //nkfadc->NKFADC500write_PSCALE(SID, fSCALE);

    unsigned long fSR = 1;        // sampling rate /1/2/4/8 (S.R. = 500 / sr MHz)
    nkfadc->NKFADC500write_DSR(SID, fSR);

    usb3comroot *usb = new usb3comroot;
    usb->USB3Init(0);

    // define KFADC500 class
    NKNKFADC500 *nkfadc = new NKNKFADC500;

    // open KFADC500
    nkfadc->NKFADC500open(SID, 0);

    // set NKFADC500
    nkfadc->NKFADC500resetTIMER(SID);
    nkfadc->NKFADC500reset(SID);
    nkfadc->NKFADC500_ADCALIGN_500(SID);
    nkfadc->NKFADC500_ADCALIGN_DRAM(SID);
    nkfadc->NKFADC500write_DRAMON(SID, 1);
    //nkfadc->NKFADC500write_TRIGENABLE(SID, trig_enable);
    for (int Ch = 0; Ch < 4; Ch++){
        nkfadc->NKFADC500write_DLY(SID, Ch + 1, 32 + fDLY[Ch]);
        nkfadc->NKFADC500write_THR(SID, Ch + 1, fTHR[Ch]);
        nkfadc->NKFADC500write_POL(SID, Ch + 1, fPOL[Ch]]);
        nkfadc->NKFADC500write_CW(SID, Ch + 1,  32);// Coincidence width: 8 ~ 32760
        nkfadc->NKFADC500write_PSW(SID, Ch + 1, 2);// Peak sum width: 2 ~ 16382 ns
        nkfadc->NKFADC500write_AMODE(SID, Ch + 1, 1);// ADC mode: 0 = raw, 1 = filtered
        nkfadc->NKFADC500write_PCT(SID, Ch + 1, 1);// Pulse count threshold: 1 ~ 15
        nkfadc->NKFADC500write_PCI(SID, Ch + 1, 32);// Pulse count interval: 32 ~ 8160 ns
        nkfadc->NKFADC500write_PWT(SID, Ch + 1, 2);// Pulse width threshold: 2 ~ 1022 ns
        nkfadc->NKFADC500write_DT(SID, Ch + 1, 20);// Trigger deadtime: 0 ~ 8355840 ns
    }

    nkfadc->NKFADC500measure_PED(SID, 1);
    nkfadc->NKFADC500measure_PED(SID, 2);
    nkfadc->NKFADC500measure_PED(SID, 3);
    nkfadc->NKFADC500measure_PED(SID, 4);


    //FADC common
    //printf("- %-25s: %ld\n", "Prescale on trig input", nkfadc->NKFADC500read_PSCALE(SID));
    printf("- %-25s: %ld\n", "Ped. trig interval (ms)", nkfadc->NKFADC500read_PTRIG(SID));
    printf("- %-25s: %ld\n", "DRAM_ON", nkfadc->NKFADC500read_DRAMON(SID));
    printf("- %-25s: %ld\n", "Recording length", nkfadc->NKFADC500read_RL(SID));
    printf("- %-25s: %lX\n", "Trigger LUT value", nkfadc->NKFADC500read_TLT(SID));
    printf("- %-25s: %ld\n", "Local trigger on", nkfadc->NKFADC500read_TRIGENABLE(SID));

    //FADC ch by ch
    printf("- %-25s:", "ADC mode");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_AMODE(SID, j+1));
    printf("\n- %-25s:", "ADC offset");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_DACOFF(SID, j+1));
    printf("\n- %-25s:", "Pedestal");
    for (int j=0; j<4; j++) printf("%5ld", nkfadc->NKFADC500read_PED(SID, j+1));
    printf("\n- %-25s:", "Coincidence width");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_CW(SID, j+1));
    printf("\n- %-25s:", "Wavefrom delay");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_DLY(SID, j+1));
    printf("\n- %-25s:", "Input pulse polarity");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_POL(SID, j+1));
    printf("%5ld (TDC)", nkfadc->NKFADC500read_POL(SID, 5)); //TDC
    printf("\n- %-25s:", "Pulse count interval (ns)");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_PCI(SID, j+1));
    printf("\n- %-25s:", "Pulse count threshold");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_PCT(SID, j+1));
    printf("\n- %-25s:", "Pulse width threshold");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_PWT(SID, j+1));
    printf("\n- %-25s:", "Peak sum width");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_PSW(SID, j+1));
    printf("\n- %-25s:", "Trigger deadtime (ns)");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_DT(SID, j+1));
    printf("\n- %-25s:", "Trigger mode (FADC)");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_TM(SID, j+1));
    printf("\n- %-25s:", "Discrimination threshold");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_THR(SID, j+1));
    printf("\n- %-25s:", "Zero suppression");
    for (int j=0; j<4; j++)	printf("%5ld", nkfadc->NKFADC500read_ZEROSUP(SID, j+1));
    printf("\n");

	return 0;
}//Main

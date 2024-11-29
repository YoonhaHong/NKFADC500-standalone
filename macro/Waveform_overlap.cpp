#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TString.h"
#include "TStyle.h"
#include "TTree.h"

#include <iostream>
#include <fstream>
#include <numeric>
#include <vector>
using namespace std;

int GetDataLength(const char* inFile)
{
	//Reading 1st event, 1st header (32 bytes fixed), 1st channel's very first four bits will be enough
	ifstream in;
	in.open(inFile, std::ios::binary);
	if (!in.is_open()) { cout <<"GetDataLength - cannot open the file! Stop.\n"; return 1; }

	char data[32];
	in.read(data, 32);
	unsigned long dataLength = 0;
	for (int i=0; i<4; i++) dataLength += ((ULong_t)(data[4*i] & 0xFF) << 8*i);

	in.close();
	return (int)dataLength;
}//GetDataLength

void Waveform_overlap(const int RunNo=20001, const char* inPath = "../data")
{
	gStyle->SetOptStat(0);

    int ADCMAX = 500;
    int ADCMIN = 300;

    const int nCh = 4;

	TFile* F  = new TFile(Form("%s/FADCData_%i.root", inPath, RunNo));
	TTree* T = (TTree*)F->Get("T");

 	const int nEvents = T->GetEntries();
	cout << "Number of events: " << nEvents << endl;
	
	const int DLen = GetDataLength(Form("%s/FADCData_%i.dat", inPath,RunNo));
	const int nADC = (DLen-32)/2;
	const int nTDC = nADC/4;

	int fADC[nCh][nADC]; //ADC
	T->SetBranchAddress("adc",      &fADC);
	

	TH2F* H2_Waveform[nCh]; //Sampling index vs. trigger #, bin content is ADC

    for(int Ch=0; Ch<nCh; Ch++){
		H2_Waveform[Ch] = new TH2F( Form("H2_Waveform_ch%i",Ch+1), Form("Waveform_ch%i",Ch+1), nADC, 0, nADC, 4095, 0, 4095);
		H2_Waveform[Ch] -> Sumw2();
	}
	for (int iEv=0; iEv<nEvents; iEv++)
	{
		T->GetEntry(iEv);
		for (int iCh=0; iCh<nCh; iCh++)
		{
			for (int c=0; c<nADC; c++)
			{
                int& tADC = fADC[iCh][c];
				H2_Waveform[iCh]->Fill(c, tADC );
                if(tADC > ADCMAX) ADCMAX = tADC;
                else if(tADC < ADCMIN) ADCMIN = tADC;
			}//c, # of sampling per event
		}//Ch


	}//iEv

	TCanvas* c1 = new TCanvas("c1","c1",400*nCh,400);
	c1 -> Divide(nCh, 1, 0.02, 0.02);
    for(int Ch=0; Ch<nCh; Ch++){
        c1 -> cd(Ch+1);
        gPad -> SetLogz();
        gPad -> SetRightMargin(0.13);
        H2_Waveform[Ch] -> Draw("COLZ");
        H2_Waveform[Ch] -> GetYaxis() -> SetRange(ADCMIN, ADCMAX);
        H2_Waveform[Ch] -> GetYaxis() -> SetTitle( "ADC" );
        H2_Waveform[Ch] -> GetXaxis() -> SetTitle( "iSample [2 ns]" );
    }
    c1->Draw();
    c1->Update();
    c1->SaveAs( Form("../fig/Waveform_%d.png", RunNo) );
}

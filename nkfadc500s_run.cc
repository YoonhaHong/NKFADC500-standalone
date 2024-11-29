#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <utility>

#include "TROOT.h"
#include "TString.h"
#include "NoticeNKFADC500ROOT.h"
#include "usb3comroot.h"
using namespace std;

#define SID 1
#define DRAM_SIZE       (10)                  // available PC DRAM size in Mbyte
#define CHUNK_SIZE      (DRAM_SIZE*1024)      // array size in kilobyte
#define DATA_ARRAY_SIZE (DRAM_SIZE*1024*1024) // array size in byte

bool bSTOP = false;
void sigint_handler(int sig)
{
	cout<< "SIGINT: Close\n";
	bSTOP = true;
	return;
}


int main(int argc, char** argv)
{
	std::signal (SIGINT, sigint_handler);

	//Local variables
	const int runNo  = std::atoi( argv[1] ); //Run ID
	const int nEvent = std::atoi( argv[2] ); //# of events to take

	const int nCh  = 4; //# of channels per FADC module

	//-------------------------------------------
	//Output file
	string fName;
	ofstream ofStr;
	fName = Form("./data/FADCData_%i.dat", runNo);
	ofStr.open(fName.c_str(), std::ios::binary);

	//Raw data array
	char* dArray = (char*)malloc(nCh * DATA_ARRAY_SIZE);

	//Define VME class
	usb3comroot *usb = new usb3comroot;
	usb->USB3Init(0);

	//Define KFADC500 class
	NKNKFADC500* nkfadc = new NKNKFADC500;

	nkfadc->NKFADC500open(SID, 0);
	nkfadc->NKFADC500start(SID);


	chrono::steady_clock::time_point tStart, tStop;
	tStart = chrono::steady_clock::now();

	int tEvent = 1;
	bool bCountNonzero = false;
	unsigned long bCount = 0;

	int nERR = 0;
	while (bSTOP==false && nEvent!=tEvent) //Take data continually if nEvent = 0
	{
		bCountNonzero = false;
		bCount = nkfadc->NKFADC500read_BCOUNT(SID);
		if (bCount == 0)  continue; //usleep(10);
		if (bCount != 16) nERR++; //for checking FADC error
		if (bCount > 0){
				if (bCount > CHUNK_SIZE)
				{
					cout <<Form("WARNING! BCOUNT of mID_%i > CHUNK: truncate it...\n", a);
					bCount = CHUNK_SIZE;
				}
				nkfadc->NKFADC500read_DATA(SID, bCount, dArray);
				ofStr.write(dArray, bCount * 1024);
			}

		if (tEvent>0 && tEvent%10==0){
			cout << Form("Writing... %3li", bCount); << endl
				 << "nBuff: " << nEvent << " tBuff: " << tEvent << endl;
		}
		tEvent++;
	}//While

	tStop = chrono::steady_clock::now();
	int tElapse = chrono::duration_cast<chrono::milliseconds>(tStop - tStart).count();

	float fBufferFreq = (float)nEvent/(float)tElapse * 8e3f;

	cout << endl << endl 
		 << "Number of buffers = " << tEvent << endl
		 << "Number of ERROR = " << nERR << endl
		 << "Elapsed time(ms) = " << tElapse << endl
		 << "Frequency(Hz) = " << fBufferFreq  << endl << endl; //if Recording Length is 512ns, 1 Buffer ~ 8 Events

	ofstream fout("Rate.txt", ios_base::out | ios_base::app);
	fout << runNo << " " << nEvent << ' ' << tElapse << " ms " << fBufferFreq << " Hz " << endl;
	fout.close();

	free(dArray); //Free data array
	nkfadc->NKFADC500stop(SID);
	nkfadc->NKFADC500close(SID); //Close KFADC500
	ofStr.close(); //Close data file
	usb->USB3Exit(0);
	return 0;
}//Main

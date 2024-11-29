#include <unistd.h>
#include <stdio.h>
#include <chrono>

#define PC_DRAM_SIZE (10)                            // available PC DRAM size in Mbyte
#define DATA_ARRAY_SIZE (PC_DRAM_SIZE * 1024 * 1024) // array size in byte
#define CHUNK_SIZE (PC_DRAM_SIZE * 1024)             // array size in kilobyte


int run_nkfadc500(int Nevent = 100000)
{
    // setting variables
    // common setting variables for NKFADC500
    int sid = 1;          // NKFADC500 SID
    unsigned long rl = 2; // recording length: 1=128ns, 2=256ns, 4=512ns,
                          // 8=1us, 16=2us, 32=4us, 64=8us, 128=16us, 256=32us


    unsigned long tlt = 0xEEEE;
    // Trigger lookup table value, any: FFFE, 1&3&2&4: 8000, 1&2|3&4: F888, 1&3|2&4: ECA0
    // 1 must be fired: AAAA
    // 2 must be fired: CCCC
    // 1|2: EEEE
    // 1&2: 8888
    // 3&4: F000

    unsigned long ptrig_interval = 0; // pedestal trigger interval in ms, 0 for disable
    unsigned long pscale = 1;         // prescale = 1 ~ 65535
    // individual channel setting variables
    // in this example, we'll put the same setting for every channel
    // every timing setting unit is ns
    unsigned long sr = 1;        // sampling rate /1/2/4/8 (S.R. = 500 / sr MHz)
    unsigned long cw = 1000;     // coincidence width (8 ~ 32760)
    unsigned long offset = 3500; // ADC offset value (0~4095)
    unsigned long dly = 100;     // ADC waveform delay from trigger point (0 ~ 31992)
    unsigned long thr = 300;     // discrimination threshold
                                 // 1 ~ 4095 for pulse height threshold
                                 // 1 ~ 67108863 for peak sum threshold
    unsigned long pol = 0;       // input pulse polarity, 0 = negative, 1 = positive
    unsigned long psw = 2;       // peak sum width in ns (2 ~ 16382 ns)
    unsigned long amode = 1;     // ADC mode, 0 = raw, 1 = filtered
    unsigned long pct = 1;       // pulse count threshold (1 ~ 15)
    unsigned long pci = 1000;    // pulse count interval (32 ~ 8160 ns)
    unsigned long pwt = 100;     // pulse width threshold (2 ~ 1022 ns)
    unsigned long dt = 0;        // trigger deadtime (0 ~ 8355840 ns)
    unsigned long ten = 1;       // when 1 enables pulse count trigger
    unsigned long tew = 0;       // when 1 enables pulse width trigger
    unsigned long tep = 0;       // when 1 enables peak sum trigger
    unsigned long tep2 = 0;      // when 1 enables peak sum OR trigger
    unsigned long tmode = (tep2 << 3) | (tep << 2) | (tew << 1) | ten;
    //unsigned long en_strig = 1; 		 // when 1 enables self trigger
    //unsigned long en_ptrig = 1; 		 // when 1 enables pedestal trigger
    //unsigned long en_rtrig = 1; 	         // when 1 enables software trigger
    //unsigned long en_etrig = 1;            // when 1 enables external trigger
    //unsigned long trig_enable = (en_etrig<<3) | (en_rtrig<<2) | (en_ptrig<<1) | en_strig;

    // local variables
    char *data;              // raw data array
    int adc;                 // ADC data
    int data_length;         // data length
    int run_number;          // run number
    int trigger_type;        // trigger type
    int trigger_destination; // trigger destination
    int trigger_number;      // trigger number from TCB
    unsigned long ttime;     // trigger time from TCB in ns
    int mid;                 // module id in data packet header
    int channel;             // channel number of data header
    int local_tnum;          // local trigger # from NKFADC500
    int trigger_pattern;     // trigger pattern from NKFADC500
    unsigned long ltime;     // starting time of waveform
    unsigned int evtn;       // event number counter
    unsigned long bcount;    // buffer counter

    // temporary variable
    int hist_point;   // number of samples to show
    float hist_range; // histogram range
    // int daq;			// DAQ flag
    FILE *fp;           // data file
    char filename[256]; // data filename
    int bufcnt;         // size of buffer data to be read
    int flag;
    int chunk, slice;
    int i, j, k;
    unsigned long ltmp;
    unsigned int itmp;

    unsigned long trig_timel;
    unsigned long trig_timeh;
    unsigned long start_timel;
    unsigned long start_timeh;

    chrono::steady_clock::time_point tStart, tStop;

    data = (char *)malloc(sizeof(char) * 4 * DATA_ARRAY_SIZE);


    // Loading usb & nkfadc500 lib.
    R__LOAD_LIBRARY(libusb3comroot.so);         // load usb3 library
    R__LOAD_LIBRARY(libNoticeNKFADC500ROOT.so); // load nkfadc500 library

    // define VME class
    usb3comroot *usb = new usb3comroot;
    usb->USB3Init(0);

    // define KFADC500 class
    NKNKFADC500 *nkfadc = new NKNKFADC500;

    // open KFADC500
    nkfadc->NKFADC500open(sid, 0);

    // set NKFADC500
    nkfadc->NKFADC500write_DSR(sid, sr);
    nkfadc->NKFADC500resetTIMER(sid);
    nkfadc->NKFADC500reset(sid);
    nkfadc->NKFADC500_ADCALIGN_500(sid);
    nkfadc->NKFADC500_ADCALIGN_DRAM(sid);
    nkfadc->NKFADC500write_PTRIG(sid, ptrig_interval);
    nkfadc->NKFADC500write_RL(sid, rl);
    nkfadc->NKFADC500write_DRAMON(sid, 1);
    nkfadc->NKFADC500write_TLT(sid, tlt);
    //nkfadc->NKFADC500write_PSCALE(sid, pscale);
    //nkfadc->NKFADC500write_TRIGENABLE(sid, trig_enable);
    for (int Ch = 0; Ch < 4; Ch++){
        nkfadc->NKFADC500write_CW(sid, Ch + 1, cw);
        nkfadc->NKFADC500write_DACOFF(sid, Ch + 1, offset);
        nkfadc->NKFADC500write_DLY(sid, Ch + 1, cw + dly);
        nkfadc->NKFADC500write_THR(sid, Ch + 1, thr);
        nkfadc->NKFADC500write_POL(sid, Ch + 1, pol);
        nkfadc->NKFADC500write_PSW(sid, Ch + 1, psw);
        nkfadc->NKFADC500write_AMODE(sid, Ch + 1, amode);
        nkfadc->NKFADC500write_PCT(sid, Ch + 1, pct);
        nkfadc->NKFADC500write_PCI(sid, Ch + 1, pci);
        nkfadc->NKFADC500write_PWT(sid, Ch + 1, pwt);
        nkfadc->NKFADC500write_DT(sid, Ch + 1, dt);
        nkfadc->NKFADC500write_TM(sid, Ch + 1, tmode);
    }

    nkfadc->NKFADC500measure_PED(sid, 1);
    nkfadc->NKFADC500measure_PED(sid, 2);
    nkfadc->NKFADC500measure_PED(sid, 3);
    nkfadc->NKFADC500measure_PED(sid, 4);
    //  nkfadc->NKFADC500write_ZEROSUP(sid, 1, zerosup);
    printf("trigger enable = %ld\n", nkfadc->NKFADC500read_TRIGENABLE(sid));
    printf("rl = %ld\n", nkfadc->NKFADC500read_RL(sid));
    printf("ch1 thr = %ld\n", nkfadc->NKFADC500read_THR(sid, 1));
    printf("ch2 thr = %ld\n", nkfadc->NKFADC500read_THR(sid, 2));
    printf("ch3 thr = %ld\n", nkfadc->NKFADC500read_THR(sid, 3));
    printf("ch4 thr = %ld\n", nkfadc->NKFADC500read_THR(sid, 4));
    printf("tlt = %lX\n", nkfadc->NKFADC500read_TLT(sid));
    printf("ch1 offset = %ld\n", nkfadc->NKFADC500read_DACOFF(sid, 1));
    printf("ch2 offset = %ld\n", nkfadc->NKFADC500read_DACOFF(sid, 2));
    printf("ch3 offset = %ld\n", nkfadc->NKFADC500read_DACOFF(sid, 3));
    printf("ch4 offset = %ld\n", nkfadc->NKFADC500read_DACOFF(sid, 4));
    printf("ch1 pedestal = %ld\n", nkfadc->NKFADC500read_PED(sid, 1));
    printf("ch2 pedestal = %ld\n", nkfadc->NKFADC500read_PED(sid, 2));
    printf("ch3 pedestal = %ld\n", nkfadc->NKFADC500read_PED(sid, 3));
    printf("ch4 pedestal = %ld\n", nkfadc->NKFADC500read_PED(sid, 4));

    nkfadc->NKFADC500reset(sid);
    nkfadc->NKFADC500start(sid);

    // write run log by YH Hong

    tStart = chrono::steady_clock::now();

    TString logname; logname.Form( "./data/log_%d.txt", RunNo);
    FILE *logfile = fopen( logname, "w+");
    fprintf( logfile, "RUNNUMBER #%d \n\n", RunNo );
    fprintf( logfile, "Trigger type : %lx \n", nkfadc->NKFADC500read_TLT(sid));
    fprintf( logfile, "Recording Length : %.f ns \n", TMath::Power(2, nkfadc->NKFADC500read_RL(sid)+5.f) );

    fprintf( logfile, "ADC Threshold : " );
    for( int ch=0; ch<4; ch++){ fprintf( logfile, "%5ld", nkfadc->NKFADC500read_THR(sid, ch) ); } 

    fprintf( logfile, "\nADC Offset : " );
    for( int ch=0; ch<4; ch++){ fprintf( logfile, "%5ld", nkfadc->NKFADC500read_DACOFF(sid, ch) ); } 

    fprintf( logfile, "\nADC Pedestal : " );
    for( int ch=0; ch<4; ch++){ fprintf( logfile, "%5ld", nkfadc->NKFADC500read_PED(sid, ch) ); } 

    fprintf( logfile, "\nPolarity : " );
    for( int ch=0; ch<4; ch++){ fprintf( logfile, "%5ld", nkfadc->NKFADC500read_POL(sid, ch) ); } 
    fprintf( logfile, "\n" );


    // now take data
    // set data filename
    sprintf(filename, "./data/FADCData_%d.dat", RunNo);

    // open data file
    fp = fopen(filename, "wb");

    flag = 1;
    evtn = 0;

    while (flag){

        // check buffer if there are something to read (in kilobyte)
        bcount = nkfadc->NKFADC500read_BCOUNT(sid);

        //    printf("bcount = %ld\n", bcount);

        // if there is data, read them
        if (bcount){ //bCount = 16

            // if data size is too big, slice them into pieces
            chunk = bcount / CHUNK_SIZE;
            slice = bcount % CHUNK_SIZE;
            slice = slice / 128;
            slice = slice * 128;

            for (i = 0; i < chunk; i++){
                // read data from ADC's DRAM
                nkfadc->NKFADC500read_DATA(sid, CHUNK_SIZE, data);

                // write to file
                fwrite(data, 1, CHUNK_SIZE * 1024, fp);

                k = 0;
                while (k < CHUNK_SIZE * 1024){
                    // get values from raw data

                    // get data length
                    data_length = data[k] & 0xFF;
                    itmp = data[k + 1 * 4] & 0xFF;
                    data_length = data_length + (unsigned int)(itmp << 8);
                    itmp = data[k + 2 * 4] & 0xFF;
                    data_length = data_length + (unsigned int)(itmp << 16);
                    itmp = data[k + 3 * 4] & 0xFF;
                    data_length = data_length + (unsigned int)(itmp << 24);

                    k = k + data_length * 4;
                    evtn++;
                    if( evtn % 64 == 0) printf( "EV # %d\n", evtn);
                    if (evtn >= Nevent){
                        i = chunk;
                        slice = 0;
                        flag = 0;
                        break;
                    }
                }
            }
            

            if(slice){
                // read data from ADC's DRAM
	            nkfadc->NKFADC500read_DATA(sid, slice, data);

	            // write to file
	            fwrite(data, 1, slice * 1024, fp);

	            k = 0;
                while( k < slice * 1024){
            	    // get values from raw data

                    // get data length
                    data_length =  data[k] & 0xFF;
                    itmp = data[k+1*4] & 0xFF;
                    data_length = data_length + (unsigned int)(itmp << 8);
                    itmp = data[k+2*4] & 0xFF;
                    data_length = data_length + (unsigned int)(itmp << 16);
                    itmp = data[k+3*4] & 0xFF;
                    data_length = data_length + (unsigned int)(itmp << 24);

                    k = k + data_length * 4;
                    evtn++;
                    if( evtn % 64 == 0) printf( "EV # %d\n", evtn);
                    if ( evtn >= Nevent) { 
                        flag = 0;
                        break;
                    }

                }
            }
        }
    }

    tStop = chrono::steady_clock::now();
    int tElapse = chrono::duration_cast<chrono::milliseconds>(tStop-tStart).count();

    fprintf( logfile, "\nElapsed time = %d ms\n", tElapse );
    fprintf( logfile, "Total Events : %d\n", evtn );

    printf( "\nRUNNUMBER %d\n", RunNo );
    printf( "Elapsed time = %d ms\n", tElapse );
    printf( "Total Events : %d\n", evtn );

    fclose( logfile );

    // close data file
    fclose(fp);

    // stop NKFADC500
    nkfadc->NKFADC500start(sid);

    // close KFADC500
    nkfadc->NKFADC500close(sid);
    usb->USB3Exit(0);

    free(data);

    return 0;
}

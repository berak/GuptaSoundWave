#include "RtAudio.h"
#include "fft.h"


#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>


// tone frequency (slider)
int freq=100;  

int scale=5000;
int steps = 20;

int lthresh=23;  
int rthresh=39;  


// fft buffers
const int NUM_FREQ = 2*1024; 

//
// callback data
//
struct Data 
{
    FFT fft;
    float freqs[ NUM_FREQ ];    
    unsigned ticks;


    int value( int id, int scale=3000 ) const 
    {
        return int(sqrt(freqs[id])*scale);
        //return int(log(freqs[id])*scale);
    }
};


//
// audio call back
//
int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void * data)
{
    Data * d = (Data*)data;

    // Write sine data.
    float f = float(1+freq)/100.0f;  
    float *buffer = (float *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) 
    {
        float v = float(sin(double(f*d->ticks)));
        *buffer ++ = v;
        d->ticks ++;
    }

    // process mic input
	d->fft.time_to_frequency_domain( (float*)inputBuffer, d->freqs );
    return 0;
}


int main(int argc, char ** argv)
{
    for ( int i = 1; i<argc-1; i++ )
    {
        if ( !strcmp(argv[i],"f") ) { freq=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"l") ) { lthresh=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"r") ) { rthresh=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"s") ) { scale=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"t") ) { step=atoi(argv[++i]); continue; }
    }
    unsigned int bufferFrames = NUM_FREQ*2;

    Data data;
	data.fft.Init(bufferFrames, NUM_FREQ, 1, 2.5f);

    RtAudio adac;
    if ( adac.getDeviceCount() < 1 ) 
    {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }

    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId  = 2; // <----------- put them on 
    iParams.nChannels = 1; //              different devices
    oParams.deviceId  = 0; // <----------- for duplex mode
    oParams.nChannels = 1; // 

    try 
    {
        adac.openStream( &oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &inout, &data );
    }
    catch ( RtError& e ) 
    {
        e.printMessage();
        exit( 0 );
    }
    try 
    {
        adac.startStream();

        int k = 0;
        int kcen = 658;
        int lmean=0,rmean=0;
        while(true)
        {
            // find the global max:
            float m = 0;
            int mi=-1;
            for ( int i=64; i<NUM_FREQ; i++ )
            {
                if ( data.freqs[i] > m ) 
                {
                    m = data.freqs[i];
                    mi = i;
                }
            }
            kcen += mi;
            kcen /= 2;
            int lsum=0,rsum=0;
            for( int i=-steps; i<-2; i++ )
            {
                lsum += data.value(kcen+i,scale);
            }
            for( int i=2; i<steps; i++ )
            {
                rsum += data.value(kcen+i,scale);
            }
            rsum /= (steps-2);
            lsum /= (steps-2);
            int rd = rsum-rmean;
            int ld = lsum-lmean;
            int lc=' ',rc=' ';
            if ( rd>rthresh )
                rc='r';
            if ( ld>lthresh )
                lc='l';

            if ( ld>lthresh || ld>lthresh)
                std::cerr << char(lc) << " " << char(rc) << std::endl;
            Sleep(100);
        }
        // Stop the stream.
        adac.stopStream();
    }
    catch ( RtError& e ) 
    {
        e.printMessage();
        goto cleanup;
    }

cleanup:
   if ( adac.isStreamOpen() ) adac.closeStream();

  return 0;
}
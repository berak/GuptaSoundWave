//#include "rt_defs.h"
#include "RtAudio.h"
#include "fft.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>


// tone frequency (slider)
int freq=83;  

// fft buffers
const int NUM_FREQ = 2*1024; 

//
// callback data
//
struct Data 
{
    float freqs[ NUM_FREQ ];    
    FFT fft;
    unsigned ticks;
};


int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void * data)
{
    Data * d = (Data*)data;

    // Write sine data.
    float f = float(1+freq)/100.0;  
    float *buffer = (float *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) 
    {
        float v = float(sin(double(f*d->ticks)));
        *buffer ++ = v;
        d->ticks ++;
    }

    // process mic input
	d->fft.time_to_frequency_domain( (float*)inputBuffer, d->freqs );
    float m = 0;
    int mi=-1;
    for ( int i=64; i<NUM_FREQ; i++ )
    {
        if ( d->freqs[i] > m ) 
        {
            m = d->freqs[i];
            mi = i;
        }
    }
    std::cerr << mi << " " << m << std::endl;
    return 0;
}

int main()
{
    Data data;
	data.fft.Init(NUM_FREQ, NUM_FREQ, 1, 8.0f);

    RtAudio adac;
    if ( adac.getDeviceCount() < 1 ) 
    {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }

    unsigned int bufferFrames = NUM_FREQ*2;
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId  = 2; // 
    iParams.nChannels = 1; // mono mic
    oParams.deviceId  = 0; // first available device
    oParams.nChannels = 1; // mono to speed up

    try 
    {
        adac.openStream( &oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &inout, &data );
    }
    catch ( RtError& e ) 
    {
        e.printMessage();
        exit( 0 );
    }

    namedWindow("fft",0);
    createTrackbar("fft", "fft",&freq,100);
    try 
    {
        adac.startStream();

        Mat img(400,400,CV_8UC3);
        int k = 0;
        while(k!=27)
        {
            img = 0;
            int offset=500;
            for( int i=0,zend=400; i<zend; i++ )
            {
                line(img,Point(i,int(data.freqs[offset+i]*300000)),Point(i+1,int(data.freqs[offset+i+1]*300000)),Scalar(0,0,200));
            }
            imshow("fft",img);
            k = waitKey(40);
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
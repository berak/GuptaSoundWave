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
int freq=100;  

int lthresh=10;  
int rthresh=40;  

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

int main()
{
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

    namedWindow("fft",0);
    createTrackbar("tone", "fft",&freq,200);
    createTrackbar("left", "fft",&lthresh,100);
    createTrackbar("right", "fft",&rthresh,100);
    try 
    {
        adac.startStream();

        Mat img(400,400,CV_8UC3);
        int k = 0;
        int kcen = 658;
        int lmean=0,rmean=0;
        while(k!=27)
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
            int steps = 20;
            int lsum=0,rsum=0;
            for( int i=-steps; i<-2; i++ )
            {
                int y = int(data.freqs[kcen+i]*300000);
                lsum += y;
            }
            for( int i=2; i<steps; i++ )
            {
                int y = int(data.freqs[kcen+i]*300000);
                rsum += y;
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
            std::cerr << mi << " " << char(lc) << " " << char(rc) << " " << m << std::endl;

            img = 0;
            int offset = mi - 200;
            for( int i=0; i<400; i++ )
            {
                line(img,Point(i,int(data.freqs[offset+i]*300000)),Point(i+1,int(data.freqs[offset+i+1]*300000)),Scalar(0,0,200));
            }
            for( int i=-steps; i<steps; i++ )
            {
                int y = int(data.freqs[kcen+i]*300000);
                rectangle(img,Rect(200+i*10,0,10,y),Scalar(200,0,0));
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
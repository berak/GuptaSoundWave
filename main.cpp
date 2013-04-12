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


const int NUM_FREQ = 2*1024;
float _freqs[ NUM_FREQ ];

FFT fft;
int freq=37;
//struct CriticalSection : public CRITICAL_SECTION 
//{
//	CriticalSection()	{ InitializeCriticalSection(this); 	}	
//	~CriticalSection() 	{ DeleteCriticalSection (this);    	}
//
//	void lock()			{ EnterCriticalSection( this );		}
//	void unlock()		{ LeaveCriticalSection( this );		}
//} mute;
//

unsigned t = 0;
int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void * )
{
    if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

    // Write interleaved audio data.
    float f = float(1+freq)/100.0;  
    float *buffer = (float *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) 
    {
        float v = float(sin(double(f*t)));
        *buffer ++ = v;
        t ++;
    }


//	if ( nBufferFrames < NUM_FREQ*2 ) return 0;;
//    mute.lock();

	fft.time_to_frequency_domain( (float*)inputBuffer, _freqs );
    float m = 0;
    int mi=-1;
    for ( int i=512; i<NUM_FREQ; i++ )
    {
        if ( _freqs[i] > m ) 
        {
            m = _freqs[i];
            mi = i;
        }
    }
    std::cerr << mi << " " << m << std::endl;
  //  mute.unlock();
    return 0;
}

int main()
{
	fft.Init(NUM_FREQ, NUM_FREQ, 1, 1.0f);

    RtAudio adac;
    if ( adac.getDeviceCount() < 1 ) 
    {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }

    // Set the same number of channels for both input and output.
    unsigned int bufferFrames = NUM_FREQ*2;
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId  = 2; // 
    iParams.nChannels = 1; // mono mic
    oParams.deviceId  = 0; // first available device
    oParams.nChannels = 1; // mono to speed up

    try 
    {
        adac.openStream( &oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &inout, 0 );
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
            //mute.lock();
            int offset=500;
            for( int i=0,zend=400; i<zend; i++ )
            {
                line(img,Point(i,int(_freqs[offset+i]*300000)),Point(i+1,int(_freqs[offset+i+1]*300000)),Scalar(0,0,200));
            }
            imshow("fft",img);
            k = waitKey(40);
           // Sleep(100);
            //mute.unlock();
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
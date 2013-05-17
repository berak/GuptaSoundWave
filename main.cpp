#include "RtAudio.h"
#include "fft.h"

#define WITH_OPENCV

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>


// tone frequency (slider)
int freq = 145;  

int scale = 5000;
int steps = 20;

// trigger values for approach and retreat
int lthresh = 27;  
int rthresh = 39;  

// device ids
int inp_audio = 2;  
int out_audio = 0;  


// fft buffersize
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
    }
};


// 
extern void main_init();
extern void main_idle(const Data & data, int kcen);


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

inline int ipol(int a,int b,int t)
{
    return ( a*(t-1) + b ) / t;
}

int main(int argc, char ** argv)
{
    if ( argc==1 )
    {
        std::cerr << "f " << freq << "\t frequency" << std::endl; 
        std::cerr << "s " << scale << "\t scale" << std::endl; 
        std::cerr << "t " << steps << "\t steps" << std::endl; 
        std::cerr << "r " << rthresh << "\t right thresh" << std::endl; 
        std::cerr << "f " << lthresh << "\t left  thresh" << std::endl; 
        std::cerr << "i " << inp_audio << "\t inp_audio device id" << std::endl; 
        std::cerr << "o " << out_audio << "\t out_audio device id" << std::endl; 
    }
    for ( int i = 1; i<argc-1; i++ )
    {
        if ( !strcmp(argv[i],"f") ) { freq=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"l") ) { lthresh=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"r") ) { rthresh=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"s") ) { scale=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"t") ) { steps=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"i") ) { inp_audio=atoi(argv[++i]); continue; }
        if ( !strcmp(argv[i],"o") ) { out_audio=atoi(argv[++i]); continue; }
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
    iParams.deviceId  = inp_audio; // <----------- put them on 
    iParams.nChannels = 1; //              different devices
    oParams.deviceId  = out_audio; // <----------- for duplex mode
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
        main_init();

        int k = 0;
        int kcen = 658;
        int lmean=0,rmean=0;
        while(true)
        {
            // find the global max:
            float m = 0;
            int mi=-1;
            for ( int i=64; i<NUM_FREQ; i++ ) // skip low freq
            {
                if ( data.freqs[i] > m ) 
                {
                    m = data.freqs[i];
                    mi = i;
                }
            }
            kcen = ipol(kcen,mi,4);
            // get the mean of the lower and the higher neighbours
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
            lmean=ipol(lmean,lsum,256);
            rmean=ipol(rmean,rsum,256);
            int lc=' ',rc=' ';
            if ( rd>rthresh )
                rc='r';
            if ( ld>lthresh )
                lc='l';

            //if ( ld>lthresh || ld>lthresh )
                std::cerr << char(lc) << " " << char(rc) << std::endl;

            main_idle(data,kcen);
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


#ifndef WITH_OPENCV

void main_init()
{
}

void main_idle(const Data & data, int kcen)
{
    Sleep(100);
}

#else // WITH_OPENCV

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

void main_init()
{
    namedWindow("fft",0);
    createTrackbar("tone", "fft",&freq,200);
    createTrackbar("left", "fft",&lthresh,64);
    createTrackbar("right", "fft",&rthresh,64);
}

void main_idle(const Data & data, int kcen)
{
    Mat img=Mat::zeros(400,400,CV_8UC3);
    int offset = kcen - 200;
    for( int i=0; i<400; i++ )
    {
        int y1 = data.value(kcen+i,scale);
        int y2 = data.value(kcen+i+1,scale);
        line(img,Point(i,y1),Point(i+1,y2),Scalar(0,0,200));
    }
    for( int i=-steps; i<steps; i++ )
    {
        int y  = data.value(kcen+i,scale);
        rectangle(img,Rect(200+i*10,0,10,y),Scalar(200,0,0));
    }
    imshow("fft",img);
    int k = waitKey(40);
    if ( k==27 ) exit(0);
}

#endif // WITH_OPENCV

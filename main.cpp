//#include "rt_defs.h"
#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

unsigned t = 0;
int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void * )
{
    if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

    // Write interleaved audio data.
    int f = 100;  
    double *buffer = (double *) outputBuffer;
    for ( unsigned int i=0; i<nBufferFrames; i++ ) 
    {
        double v = sin(double(f*t));
        *buffer ++ = v;
        *buffer ++ = v;
        t ++;
    }
    return 0;
}

int main()
{
    RtAudio adac;
    if ( adac.getDeviceCount() < 1 ) 
    {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }

    // Set the same number of channels for both input and output.
    unsigned int bufferBytes, bufferFrames = 512*44;
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = 2; // first available device
    iParams.nChannels = 1; // mono mic
    oParams.deviceId = 0; // first available device
    oParams.nChannels = 2;

    try 
    {
        adac.openStream( &oParams, &iParams, RTAUDIO_FLOAT64, 44100, &bufferFrames, &inout, 0 );
    }
    catch ( RtError& e ) 
    {
        e.printMessage();
        exit( 0 );
    }

    bufferBytes = bufferFrames * 2 * 128;

    try 
    {
        adac.startStream();

        char input;
        std::cout << "\nRunning ... press <enter> to quit.\n";
        std::cin.get(input);

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
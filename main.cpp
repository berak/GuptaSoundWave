//#include "rt_defs.h"
#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>


// Two-channel sawtooth wave generator.
int saw( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
    unsigned int i, j;
    double *buffer = (double *) outputBuffer;
    double *lastValues = (double *) userData;

    if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

    // Write interleaved audio data.
    for ( i=0; i<nBufferFrames; i++ ) 
    {
        for ( j=0; j<2; j++ ) 
        {
            *buffer++ = lastValues[j];

            lastValues[j] += 0.005 * (j+1+(j*0.1));
            if ( lastValues[j] >= 1.0 ) lastValues[j] -= 2.0;
        }
    }

  return 0;
}

// Pass-through function.
int inout( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *data )
{
    saw( outputBuffer, inputBuffer, nBufferFrames, streamTime, status, data );
    // Since the number of input and output channels is equal, we can do
    // a simple buffer copy operation here.
    if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

//    unsigned long *bytes = (unsigned long *) data;
//    memcpy( outputBuffer, inputBuffer, *bytes );
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
    unsigned int bufferBytes, bufferFrames = 512;
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = 2; // first available device
    iParams.nChannels = 1; // mono mic
    oParams.deviceId = 0; // first available device
    oParams.nChannels = 2;

    try 
    {
        adac.openStream( &oParams, &iParams, RTAUDIO_SINT32, 44100, &bufferFrames, &inout, (void *)&bufferBytes );
    }
    catch ( RtError& e ) 
    {
        e.printMessage();
        exit( 0 );
    }

    bufferBytes = bufferFrames * 2 * 4;

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
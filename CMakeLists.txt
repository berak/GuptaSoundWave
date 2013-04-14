cmake_minimum_required (VERSION 2.6) 

PROJECT(soundwave)


SET(SRC
    main.cpp
    RtAudio/RtAudio.cpp
    fft/fft.cpp
)

ADD_EXECUTABLE( ${PROJECT_NAME} ${SRC} )

if (WITH_OPENCV)
    # Requires OpenCV v2.4.9 or later, because of the changed headers
    FIND_PACKAGE( OpenCV REQUIRED )
    IF (${OpenCV_VERSION} VERSION_LESS 2.4.0)
        MESSAGE(FATAL_ERROR "OpenCV version is not compatible : ${OpenCV_VERSION}. soundwave requires atleast OpenCV v2.4.0")
    ENDIF()
    TARGET_LINK_LIBRARIES( ${PROJECT_NAME}  ${OpenCV_LIBS} )
else()
endif()

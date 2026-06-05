## Intro:
This is a fork of EnclaveShrew's brilliant ScrollZoom. The idea of this fork is to implement reading scope parameters directly from the scope through use of keywords. 

Currently, the keywords must have one of the following prefixes - referring to initial zoom, max zoom, min zoom, and zoom step, respectively:

    const std::string ZOOM_KW_INIT = "dn_ScrollZoom_Init";
    const std::string ZOOM_KW_MAX = "dn_ScrollZoom_Max";
    const std::string ZOOM_KW_MIN = "dn_ScrollZoom_Min";
    const std::string ZOOM_KW_STEP = "dn_ScrollZoom_Step";
    const std::string ZOOM_KW_FIXED = "dn_ScrollZoom_Fixed";

Followed by a floating point number indicating it's value in the following fashion:

    _Number_Number // resolves to Number.Number

OR

    _Number // resolves to Number.0

Examples:

 - dn_ScrollZoom_Init_1_0 -> this means the initial zoom for this scope
   is 1.0x.
 - dn_ScrollZoom_Max_6 -> this means the max zoom for this scope is
   6.0x.
- dn_ScrollZoom_Min_1_5 -> this means the min zoom for this scope is 1.5x.
- dn_ScrollZoom_Step_2_0 -> this means the "step" in zoom for this scope is 2.0x.
- dn_ScrollZoom_Fixed_2_5 -> indicates a fixed scope; this means the min, max, and init zoom are 2.5x and the step is 0.

Make sure the step, max, and min values line up well to avoid inconsistencies when zooming in and out. 

## Requirements:
https://github.com/EnclaveShrew/commonlibf4-unified/tree/main

Follow the installation and build instructions.



https://github.com/brofield/simpleini

Clone to the ScrollZoom/lib/simpleini.



https://cmake.org/download/

At least version 3.20.



https://github.com/ninja-build/ninja/releases

I used version 1.13.2. Don't forget to add the ninja.exe to your PATH (personally I just put it in my cmake/bins folder since that was already on PATH).



Build Instructions:
Edit `CMakeLists.txt`, specifically on lines 77-83:

    set(
    	CommonLibF4Unified_DIR
    	"C:/Path/To/CommonLibF4-Unified/cmake"
    	CACHE PATH
    	"Path to the CommonLibF4 Unified CMake package"
    	FORCE
    )

to indicate where your installation of CommonLibF4-United is. 

In a terminal that has access to a 64-bit C++ compiler (I used MSVC 14.51), run the following CMake commands:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Make sure your cmake and xmake are using the same compiler otherwise you'll have linking issues while compiling ScrollZoom.

Don't forget to copy the ScrollZoom.dll (and .ini in the root folder if you don't have it yet) from /build to your F4SE plugin installation folder (Data/F4SE/Plugins). 

### Current Known Issues:
For some reason setting the fovMult to 1.0 doesn't update the fov ingame, so as a workaround the minimum fov is 1.01. Aside from that, it works as intended!
I disabled the minimum fovMult check for now, but I might reenable it later, at least as a backup.
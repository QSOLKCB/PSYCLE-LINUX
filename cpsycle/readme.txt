C-Psycle is a new Psycle version in "C" language (instead of C++, except
of some plugin and driver apis written in C++) with the goal of being
compatible with mfc-psycle and sharing as much features as possible.

Building/Installation

Linux

Config

To build psycle on a linux platform install the base compiler system and needed dev libaries of x11, xft and asound2 and others.

debian:
sudo apt-get update
sudo apt-get install build-essential liblilv-dev liblua5.4-dev libfreetype-dev  libfontconfig-dev libgl-dev libx11-dev libxft-dev libxext-dev libxmu-dev libasound2-dev libjack-jackd2-dev libstk-dev libsdl2-dev libfluidsynth-dev

Make

Change to the cpsycle path

Build release
    make
    make all        Executes the makefiles of the host and plugins submodules	
    make host       Executes the makefiles of the host
    make plugins    Executes the makefiles of the plugins. The plugins are
                    build in cpsycle/plugins/build.              
Build debug
    make debug
    make host-debug
    make plugins-debug

Clean debug/release
    make clean          cleans all (host and plugins)
    make clean-host     cleans only the host 
    make clean-plugins  cleans only the plugins
    
Changing debug/release first clean the previous build and after building rescan the plugins with the newmachine dialog.

Other plugins (linux-vsts, ladspa)

Use the package manager and rescan the plugins with the newmachine
dialog.

Start psycle

In cpsycle start the host with
    ./psycle

Windows 

Visual Studio

You can use the current community version 2022 with the install option
"Windows desktop development with C++ in Visual Studio", but you have to
install the older Visual Studio MSVC 142 VS2019 Buildtools aswell.
To build the windows version, check out the psycle svn trunk, open in the
cpsycle directory  "cpsycle.msvc-2019.sln", choose
Build target Win32 or X64) and ensure that the startproject is host. Compile
the solution. If something went wrong press CTRL + SHIFT + B to try again.

In cpsycle/debug{release}/x86{x64} you find host.exe asio.dll mme.dll
mmemidi.dll directx.dll wasapi.dll you need to copy  into the bin 
directory (e.g: C:\Program Files (x86)\Psycle Modular Music Studio)
of an existing installation. Now start psycle with host.exe.

Documentation

See in cpsycle/doc for more information how the host is structured.

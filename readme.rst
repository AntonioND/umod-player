UMOD Player
===========

1. Introduction
---------------

This is a library used to play MOD files. Eventually, it will also support XM,
S3M and IT. The main goal of this library is to have a music player for GBA
games. The GBA doesn't support more than 2 hardware PCM channels, which isn't
enough for playing music. All the mixing needs to be done by software.

This library has been designed so that it is easy to create regression tests.
This is needed to help optimize the code for the GBA. Once the player is
working, it is easy to optimize it and be relatively sure that nothing has been
broken.

Note that, at the moment, the only possible output from this library is a buffer
of samples. There is a WAV file renderer that is used for the regression tests.

This is not ready to be used in real projects yet.

2. Dependencies
---------------

To generate GBA ROMs:

- `devkitPro`_ (devkitARM and libgba)

To generate PC executables:

- GCC, Clang, MSVC or another compiler supported by CMake.
- CMake 3.15 or later

You need to install devkitPro following the instructions in this link, then
follow the instructions in the sections below.

https://devkitpro.org/wiki/Getting_Started

3. Build PC library and examples on Linux
-----------------------------------------

If you're on Linux or any Linux-like environment (like MinGW or Cygwin), go to
the **umod** folder. The following will build the library and run all the tests
to verify it's working:

.. code:: bash

    mkdir build ; cd build ; cmake ..
    make -j`nproc`
    ctest

4. Build GBA example
--------------------

Follow the instructions to build the PC tools as explained above.  Make sure
that the name of the folder is **build**, and it is next to the folder
**gba_example**, in the root of the repository.

Go to folder **player** and run ``make``. Go to folder **gba_example** and run
``make`` again:

.. code:: bash

   cd player
   make
   cd ..
   cd gba_example
   make

Note that the code of this example is just a proof of concept.

5. Licenses
-----------

All licenses used in this repository have a copy in the **licenses** folder.

All code is licensed under the **MIT** license (as specified in the header of
each source code file). This is all you need to worry about if you want to use
the library in your own projects.

All files that aren't source code are licensed under the **Creative Commons
Attribution 4.0 International** (``SPDX-License-Identifier: CC-BY-4.0``). In
practice, this covers all the test files in the **test** directory.

The folder **tests/songs** is exempt from this license, as they are songs found
online. Each song has a corresponding information file in the **licenses/songs**
folder of the repository. It links to the website they were downloaded from, the
author, and the license.

6. Acknowledgements
-------------------

- Brett Paterson (FireLight) for FMODDOC.TXT
- Kurt Kennett (Thunder) and Erland Van Olmen (ByteRaver / TNT / NO_ID) for
  MODFIL10.TXT and corrections to it.
- The authors of OpenMPT, the tool I used to compose.
- Dave Murphy (WinterMute) and others for devkitPro, devkitARM and libgba.
- Martin Korth (Nocash) for no$gba and GBATEK.
- Vicki Pfau (endrift) for mGBA.
- The Mod Archive for an endless collection of songs.
- Nightbeat, for some of the songs I've used to test this player with.

.. _devkitPro: https://devkitpro.org/
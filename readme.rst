UMOD Player v0.1.0
==================

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
of samples. There is a WAV file renderer that is used for the regression tests,
and a GBA example that shows how to integrate the player in a GBA project.

This is not ready to be used in real projects yet.

2. Dependencies
---------------

To generate PC executables and libraries:

- GCC, Clang, MSVC or another compiler supported by CMake.
- CMake 3.15 or later

To build the GBA example ROM:

- `devkitPro`_ (devkitARM and libgba)

You need to install devkitPro following the instructions in this link, then
follow the instructions in the sections below.

https://devkitpro.org/wiki/Getting_Started

To build the library for GBA you need either `devkitPro`_ or any
``gcc-arm-none-eabi`` toolchain. For example, you can get it from your package
manager, or from `Arm's GNU toolchain downloads website`_. In Ubuntu you can
just do:

.. code:: bash

    sudo apt install gcc-arm-none-eabi

3. Build PC library and examples on Linux
-----------------------------------------

If you're on Linux or any Linux-like environment (like MinGW or Cygwin), go to
the **umod** folder. The following will build the library and run all the tests
to verify it's working:

.. code:: bash

    mkdir build ; cd build ; cmake ..
    make -j`nproc`
    ctest -DBUILD_GBA=OFF

4. Build GBA library
--------------------

To build it using **devkitPro**:

.. code:: bash

    mkdir build ; cd build ; cmake ..
    make -j`nproc`

To build it without using **devkitPro**:

.. code:: bash

    mkdir build ; cd build ; cmake .. -DUSE_DEVKITPRO=OFF
    make -j`nproc`

To override the autodetected location of the cross compiler, you can add
``-DARM_GCC_PATH=/path/to/folder/`` to the ``cmake`` command.

5. Build GBA example
--------------------

This example is meant to be built using **devkitARM** and **libgba**. Both of
them are distributed as part of **devkitPro**. If you want to see an example
that doesn't use anything from **devkitPro**, check the `ugba-template`_
repository.

If you have installed **devkitPro**, follow the instructions below.

Follow the instructions to build the PC tools as explained above. Make sure
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

6. Licenses
-----------

All licenses used in this repository have a copy in the **licenses** folder.

All code is licensed under the **MIT** license (as specified in the header of
each source code file). This is all you need to worry about if you want to use
the library in your own projects.

All files that aren't source code are licensed under the **Creative Commons
Attribution 4.0 International** (``SPDX-License-Identifier: CC-BY-4.0``). In
practice, this covers all the test files in the **test** directory.

There are exceptions. Any song found online has its corresponding license
information in folders **licenses/sfx** or **licenses/songs**. It links to the
website they were downloaded from, the author, and the license.

If there isn't a corresponding license file it's because the file has been
created as part of this project. In that case, it is licensed under the
**CC-BY-4.0** license.

7. Acknowledgements
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
- Open Game Art, for some of the sound effects used for testing this player.

.. _Arm's GNU toolchain downloads website: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
.. _devkitPro: https://devkitpro.org/
.. _ugba-template: https://github.com/AntonioND/ugba-template

# Qt 4 with Freetype support for Windows platforms

This repo hosts a fork of Qt 4 that provides Freetype rendering on Windows platforms.  It has been tested against most of Microsoft's tool chains from msvc2008 to msvc2017.

The main interest of this fork is that it allows Windows CE applications requiring complex text layout (eg. Arabic) to be shipped without the inclusion of Uniscribe in the OS.

To build qt with Freetype support simply add -qt-freetype when configuring the build.

## Building with build-qt.cmd

build-qt.cmd has been added to simply builds.  It produces out-of-tree builds based on .cmd configurations stored in the build-targets folders.

```batch
Usage: build-qt.cmd target [build_action] [tool_chain]

 where [] indicates an optional parameter, and:

  - target is one of {Desktop, Wince}.
  - [build_action] is one of {configure, build, distill, all}.
    Default is all, ie. perform configure, build, and distill in sequence
  - tool_chain is one of {msvc2008, msvc2010, msvc2012, msvc2013, msvc2015, msvc2017}.
    Default is msvc2008
```

eg.

```batch
build-qt.cmd Desktop build msvc2017
```

The build-targets are .cmd (batch) libaries stored in the build-targets folder.  Each build target must implement the :configure and :build functions.  See build-targets\Desktop.cmd for an example.

The intention of the build targets is to make experimenting with different qt configurations simpler.  qt already supports different "platforms", which are defined under the mkspecs folder.  I consider these places to fine tune builds for the underlying platform.

But since qt's configure utility has many options that can drastically affect the functionality and size of the final build, I decided to implement these build target helpers as a way to facilitate experimentation.

------------------------------------------

The build-process happens out of tree.  build-qt.cmd expects to have write-permissions to its parent folder.

It creates a folder for build artefacts formed from a mashup of:
  * the git branch name,
  * the git SHA of HEAD,
  * the build-target,
  * the tool-chain

eg.

qt4.8-ee3698-Desktop-msvc2017

## Notes:

build-qt.cmd uses [jom](https://github.com/qt-labs/jom) to speed up the build.  I used an executable I had floating around, but you can also build it from source.

Building Qt 4 using a modern compiler will produce a lot of warnings especially during the configure stage.  qmake tries to parse some of the compiler options and gets confused.  This does not affect the build, but produces a lot of noise.  They should be cleaned up in another round.

There is no platform for win32-msvc2017.  I cheated by using the win32-msvc2015 platform to configure the build but setting PATH etc. to the msvc2017 toolchain.  Should be easy enough to fix up, but I'll leave that for another day.

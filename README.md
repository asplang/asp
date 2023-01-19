Asp scripting platform
======================

Build instructions
------------------

1.  Asp is built using CMake. Issue the following commands to build the
    compiler, engine library, and standalone application for the host
    environment.

```
$ mkdir build
$ cd build
$ cmake ..
$ ccmake . # Optional. Review/update project settings.
           # If making changes, use c to configure, then g to generate.
$ make
```

2. The binaries may be installed using the following command after the build
   is complete.

```
$ sudo make install
```

    This installs the following files (PREFIX is e.g., /usr or /usr/local):

    - ${PREFIX}/bin/aspc - Asp compiler.
    - ${PREFIX}/bin/aspg - Asp application specification file generator.
    - ${PREFIX}/bin/asps - Standalone application.
    - ${PREFIX}/etc/asp/standalone.aspec - Standalone application specification.
    - ${PREFIX}/lib/libaspe.so - Asp engine library (shared build).
    - ${PREFIX}/lib/libaspe.a - Asp engine library (static build).
    - ${PREFIX}/include/asp.h (+ others) - Headers for application development.
    - ${PREFIX}/include/asps/*.asps - Application specification include files.

Using the standalone application
--------------------------------

Before running a script in the standalone application, set the ASP_SPEC_FILE
environment variable to the standalone application specification file.

```
    $ export ASP_SPEC_FILE=/usr/etc/asp/standalone.aspec
```

Alternatively, you can specify it on the compile command line, or copy the
file to the local directory and rename it to app.aspec.

Now you can compile your script and run it under the standalone application.

```
    $ aspc my_script.asp
    $ asps my_script
```

To display command line options for the standalone application, use the -h
option.

```
    $ asps -h
```

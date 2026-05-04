# The Asp Scripting Platform for Embedded Systems

## Description

### Asp is for Script Writers

The Asp language resembles basic Python (with some small differences), making
it easy to learn for those who are already familiar with Python. It supports
conditionals (`if`/`elif`/`else`), loops (`while`, `for`), and functions
(`def`). It supports basic data types plus tuples, lists, sets, dictionaries,
and ranges/slices. To keep things small, Asp does not support classes,
exception handling, and many other advanced features.

### Asp is Designed for Embedded Systems

Scripts are compiled to compact byte-code, which is checked for compatibility
with the application before being allowed to run in the engine.

The Asp engine has a small code memory footprint (less than 100 KB when
compiled optimizing for space). It avoids use of dynamic memory allocation and
recursion, resulting in very little impact on the memory of the host
application. Applications run scripts one instruction at a time, retaining a
high frequency of control of the CPU. The engine supports blocking script
functions without blocking the application. It works with bare-metal or
OS-based applications. The engine is implemented entirely in C.

## Build instructions

1.  Asp is built using CMake. Issue the following commands to prepare the
    build system.

    ```
    $ mkdir build
    $ cd build
    $ cmake ..
    ```

2.  Make any configuration changes, if not already done with options to the
    `cmake` command, above. This may be done using `ccmake`, `cmake-gui`, etc.

    ```
    $ ccmake . # Optional. Review/update project settings.
               # If making changes, use c to configure, then g to generate.
    ```

    The following options should be considered.

    - `BUILD_FOR_HOST` and `BUILD_FOR_TARGET` - If one of these two options is
      on and the other is off, only the applicable targets will be built. If
      they are both on (or both off) all targets are built.
    - `INSTALL_DEV` - Enables installation of development libraries. If not
      enabled, only tools will be installed.
    - `BUILD_SHARED_LIBS` - Builds shared libraries vs. static libraries.
    - `ENABLE_FEATURE_CLASS` - Enables support for classes. Even when enabled
       for the build, class support can be enable/disabled via the app spec,
       but disabling it from the build produces a smaller engine library.
    - `ENABLE_DEBUG` - Adds debug API functions in the engine. Also, when on,
      the file names of some targets are appended with "-d".


3.  Make the engine library, compiler, standalone application, and other tools
    for the host environment. For Linux systems, use the `make` command.

    ```
    $ make
    ```

    For Windows systems, instead of using `make`, issue the following command
    to build the release configuration. Alternatively, the build can be
    performed from within the Visual Studio IDE. This step is not required if
    installing; see the next step.

    ```
    C:> msbuild asp.sln /property:Configuration=Release
    ```

    Use `BUILD_FOR_TARGET` and CMake's cross compilation facility to build the
    engine library for your target.

4.  The binaries may be installed on a Linux system using the following command
    after the build is complete.

    ```
    $ sudo make install
    ```

    For Windows systems, use the following command instead. Alternatively,
    as above, the install build may be performed from within the Visual Studio
    IDE. In either case, the build must typically be issued from a process with
    administrator priveleges.

    ```
    C:> msbuild INSTALL.vcxproj /property:Configuration=Release
    ```

    The following files are installed. PREFIX is e.g., /usr or /usr/local on
    Linux systems, "C:/Program Files (x86)/asp" on Windows systems. The version
    of ABI is designated by X.Y.

    - ${PREFIX}/bin/aspc - Asp compiler.
    - ${PREFIX}/bin/aspg - Asp application specification file generator.
    - ${PREFIX}/bin/asps\[-d\] - Standalone application.
    - ${PREFIX}/bin/aspinfo - Asp info utility.
    - ${PREFIX}/etc/asp/standalone.aspec - Standalone application spec.
    - ${PREFIX}/lib/libaspe\[-d\].so - Asp engine library (shared build).
    - ${PREFIX}/lib/libaspe\[-d\].a - Asp engine library (static build).
    - ${PREFIX}/lib/libaspm\[-d\].so - Asp engine math library (shared build).
    - ${PREFIX}/lib/libaspm\[-d\].a - Asp engine math library (static build).
    - ${PREFIX}/lib/libaspd.so - Asp info library (shared build).
    - ${PREFIX}/lib/libaspd.a - Asp info library (static build).
    - ${PREFIX}/include/asp-X.Y/asp\*.h - Headers for application development.
    - ${PREFIX}/include/asps/X.Y/\*.asps - Application spec include files.

    Note that most files also have "-X.Y" added to the their file names and the
    base names are actually symbolic links to these version-specific files,
    allowing multiple versions to be installed on the same system.

## Packaging instructions

Asp packages are built using CPack. A Python script, `package.py`, is provided
that builds a standard set of packages for use on various platforms. A package
may be used to install the software (source or binaries) instead of building
and installing as described above.

## Using the standalone application

Before running a script in the standalone application, set the `ASP_SPEC_FILE`
environment variable to the standalone application specification file.

```
$ export ASP_SPEC_FILE=/usr/etc/asp/standalone.aspec
```

Alternatively, you can specify it on the compile command line, or copy/link the
file to the local directory and rename it to app.aspec.

Now you can compile your script and run it under the standalone application.
For Windows, you will need to have the appropriate bin directory in the PATH
environment variable.

```
$ aspc my_script.asp
$ asps my_script
```

To display command line options for the any of the tools, use the `-h` option.

```
$ aspc -h
$ aspg -h
$ asps -h
$ aspinfo -h
```

## More information

- Web site: https://www.asplang.org/
- E-mail: info@asplang.org
- Source respositories:
    - https://bitbucket.org/asplang/ (development)
    - https://github.com/asplang/ (stable)
- Online documentation: https://asplang.bitbucket.io/
- License: See the LICENSE.txt file.

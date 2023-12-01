Asp change log
==============

The following is a history of changes to the Asp software, in reverse
chronological order.

Changes
-------

Version 0.7.2.1 (compiler 0.7.1.3, engine 0.7.4.1):
- Engine:
  - Added AspInitializeEx which accepts a routine for converting from IEEE 754
    binary64 format to the native floating-point format for platforms that
    implement custom floating-point formats.

Version 0.7.2.0 (compiler 0.7.1.3, engine 0.7.4.0):
- Initial public release.
- Engine:
  - Refined support for shared libraries, especially under Windows.

Version 0.7.1.8 (compiler 0.7.1.3, engine 0.7.3.7):
- Engine:
  - Fixed slicing issues with slice components of mixed sign.
  - Fixed a bug with assignment to list slices. Assignment to tuple slices was
    unaffected.
  - Fixed a bug with sequence assignment with a list as the target.
- Build:
  - Fixed an issue with the version header file generation that caused
    problems with parallel make.

Version 0.7.1.7 (compiler 0.7.1.3, engine 0.7.3.6):
- Engine:
  - Added AspIsReady and AspIsRunnable functions to the programmer API.
  - Fixed a bug with the AspStringElement programmer API function.

Version 0.7.1.6 (compiler 0.7.1.3, engine 0.7.3.5):
- Engine:
  - Added conversion of objects to canonical string representation.
    - Added the AspToRepr function to the programmer API.
    - Added the repr script library function.
    - Added support for 'r' and 'a' conversion types for the string formatting
      operator (str % tuple).

Version 0.7.1.5 (compiler 0.7.1.3, engine 0.7.3.4):
- Compiler:
  - Added an error for the wildcard form of the from...import statement.
- Engine:
  - Changed the way null characters in strings are rendered in conversions
    to string. Was '\x00', is now '\0'.
  - Made MKR* instructions more memory efficient when explicit default values
    are encountered.
  - Implemented the string formatting operator (str % tuple).
  - Made some library script functions more like Python:
    - Added an optional base parameter to the int function. The default base
      for string conversions is 10.
    - Added an optional base parameter to the log function. The default base
      is e, yielding the natural logarithm.
  - Added tuple and list script functions. Note that conversion from
    dictionaries is different than Python.
  - Added API functions for creating ranges.
- Standalone application:
  - Added optional sep and end parameters to the print function to make it
    more like Python.

Version 0.7.1.4 (compiler 0.7.1.2, engine 0.7.3.3):
- Engine:
  - Added codec script library routines for encoding/decoding numeric data
    to/from binary representations for use in applications that require this
    functionality. Note that these routines are not available in the standalone
    application.

Version 0.7.1.3 (compiler 0.7.1.2, engine 0.7.3.2):
- Added ability to build under Windows.
- Reformatted help information displayed by the compiler, the standalone
  application, and the info utility.
- Standalone application:
  - Added sleep script function.

Version 0.7.1.2 (compiler 0.7.1.1, engine 0.7.3.1):
- Compiler:
  - Added generation of the source info file (.aspd).
  - Added -v option to print version information and exit.
- Info library and utility (new):
  - Added a library for translating error codes into text and translating
    a script's program counter into source location to be used in error
    reporting and tracing applications.
  - Added a utility for performing the functions of the above library.
- Standalone application:
  - Added error code and program counter translation via the new info library.

Version 0.7.1.1 (compiler 0.7.1.0, engine 0.7.3.1):
- Engine:
  - Ensured that global overrides for variables that do not exist locally
    do not leave a vestigial local variable when the global override is removed.
- Application specification generator:
  - Fixed a bug that caused the generator to hang when a lone period was
    encountered.

Version 0.7.1.0 (compiler 0.7.1.0, engine 0.7.3.0):
- Compiler:
  - Fixed a bug that disallowed the comment character (#) inside strings.
  - Added allowing of a comment following the line continuation character (\\).
  - Enhanced the 'for' statement to allow tuple assignment of arbitrary depth,
    like with assignments.
  - Fixed a code generation bug with tuple constants whose first element is
    a tuple constant (in parentheses).
- Engine:
  - Finished comparison logic to include sequences.
    - Tuples may now be used as keys (for sets and dictionaries).
  - Implemented substring search for the 'in' operator with string operands.
  - Added erase functions to programmer API.
  - Added direct access to iterators for scripts and in programmer API.
  - Finished implementation of AspToString API function.
  - Made some minor changes to type conversions involving floating-point values.
  - Obsoleted the AspTypeString function. Use AspToString instead.

Version 0.7.0.0 (compiler 0.7.0.0, engine 0.7.0.0):
- Initial release.

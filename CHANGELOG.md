Asp change log
==============

The following is a history of changes to the Asp software, in reverse
chronological order.

Changes
-------

Version 0.7.1.5 (compiler 0.7.1.3, engine 0.7.3.4):
- Compiler:
  - Added an error for the wildcard form of the from...import statement.
- Engine:
  - Changed the way null characters in strings are rendered in conversions
    to string. Was '\x00', is now '\0'.
  - Made MKR* instructions more memory efficient when explicit default values
    are encountered.
  - Implemented the string formatting operator (str % tuple).

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

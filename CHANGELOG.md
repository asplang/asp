Asp change log
==============

The following is a history of changes to the Asp software, in reverse
chronological order.

Changes
-------

Version 0.7.1.1 (compiler 0.7.1.0, engine 0.7.3.1):
- Engine:
  - Ensured that global overrides for variables that do not exist locally
    do not leave a vestigial local variable when the global override is removed.
- Application specification generator:
  - Fixed a bug that caused the generator to hang when a lone period was
    encountered.

Version 0.7.1.0 (compiler 0.7.1.0, engine 0.7.3.0):
- Engine:
  - Finished comparison logic to include sequences.
    - Tuples may now be used as keys (for sets and dictionaries).
  - Implemented substring search for the 'in' operator with string operands.
  - Added erase functions to programmer API.
  - Added direct access to iterators for scripts and in programmer API.
  - Finished implementation of AspToString API function.
  - Made some minor changes to type conversions involving floating-point values.
  - Obsoleted the AspTypeString function. Use AspToString instead.
- Compiler:
  - Fixed a bug that disallowed the comment character (#) inside strings.
  - Added allowing of a comment following the line continuation character (\\).
  - Enhanced the 'for' statement to allow tuple assignment of arbitrary depth,
    like with assignments.
  - Fixed a code generation bug with tuple constants whose first element is
    a tuple constant (in parentheses).

Version 0.7.0.0 (compiler 0.7.0.0, engine 0.7.0.0):
- Initial release.

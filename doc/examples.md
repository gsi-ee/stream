\page stream_examples Examples

\subpage stream_example_custom

\subpage stream_example_autotdc

\subpage stream_example_tum


\page stream_example_custom Example with custom HADAQ processor

Here typical initialization script

\include custom/first.C

And here is custom processor, which defined in `custom.h` file

\include custom/custom.h


\page stream_example_autotdc Example with automatic TDC creation

\include autotdc/first.C


\page stream_example_tum Example with second.C and reading of created ROOT file

Here normal `first.C`

\include tum/first.C

Then  `second.C`

\include tum/second.C

And script `read.C` to read data from created ROOT file

\include tum/read.C

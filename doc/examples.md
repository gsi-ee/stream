\page stream_examples Examples

# Documented examples {#stream_examples}

\subpage stream_example_custom

\subpage stream_example_autotdc

\subpage stream_example_tum


\page stream_example_custom Custom example

# Example with custom HADAQ processor {#stream_example_custom}

Here typical initialization script

\include custom/first.C

And here is custom processor, which defined in `custom.h` file

\include custom/custom.h


\page stream_example_autotdc Autotdc example

# Example with automatic TDC creation {#stream_example_autotdc}

\include autotdc/first.C


\page stream_example_tum first.C/second.C example

# Example with second.C and reading of created ROOT file {#stream_example_tum}

Here normal `first.C`

\include tum/first.C

Then  `second.C`

\include tum/second.C

And script `read.C` to read data from created ROOT file

\include tum/read.C

# Convert calibrations into ROOT file

To start, run

    root -l -b root_converter.cxx -q

This will convert all calibration files with mask "local*.cal" into "calibr.root" file.
Prefix name and output file name can be changed inroot_converter.cxx source


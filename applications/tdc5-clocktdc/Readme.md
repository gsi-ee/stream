# Use with TDC5 clock TDC

Here number of fine bins is much less so one need to adjust different parameters
that analysis works properly.

      hadaq::TdcProcessor::SetDefaults(16, 50, 1);

Here one set number of fine bins, range for "normal" ToT histogram and so-called reduction
factor for 2D histograms. While histogram binning will be reduced because of only 16 possible
finecounter values, factor 1 is ok for 2D.

      hadaq::TdcProcessor::SetToTCalibr(1000, 0.5);

This is statistic numbers and allowed RMS value when calibrating ToT offset with 0xD trigger.
While clock TDC has much less bins, 0.5 ns is good estimation

      tdc->SetCustomMhz(150.);
      tdc->SetToTRange(19.2, 15., 45.);

This is individual TDC settings. Second call configures expected ToT offset and range for
histogram used for 0xD trigger histogram accumulation which then used for ToT offset calibation
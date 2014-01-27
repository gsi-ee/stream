// macro shows how artificially produce statistic and create calibration file for TDC

void calibr()
{

      // create processor for hits from TDC
      // par1 - pointer on trb3 board
      // par2 - TDC number (lower 8 bit from subevent header
      // par3 - number of channels in TDC
      // par4 - edges mask 0x1 - rising, 0x2 - trailing, 0x3 - both edges
      hadaq::TdcProcessor* tdc = new hadaq::TdcProcessor(0, 0xc001, 65, 0x1);
      
      for (unsigned ch=0;ch<65;ch++)
         for (unsigned fine=0;fine<500;fine++)
            tdc->IncCalibration(ch, true, fine, 128);
            
      tdc->ProduceCalibration();
            
      tdc->StoreCalibration("test_c001.cal");
      
      delete tdc;
}



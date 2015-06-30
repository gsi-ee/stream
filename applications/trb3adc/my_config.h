#include <map>
#include <string>

struct my_config_t {
   double SamplingPeriod; // in ns
   double AdcEpochCounterFixThreshold; // in ns
   my_config_t() {}
   my_config_t(double samplingPeriod, double adcEpochCounterFixThreshold) :
      SamplingPeriod(samplingPeriod),
      AdcEpochCounterFixThreshold(adcEpochCounterFixThreshold)
   {}
};



static std::map<std::string, my_config_t> create_my_config() {
   std::map<std::string, my_config_t> map;
   const double sampling80 = 12.5;   
   
   map["scratch/CBTagg_9221.dat.hld"]     = my_config_t(sampling80, 17.5);
   map["scratch/CBTaggTAPS_9227.dat.hld"] = my_config_t(sampling80, 17.5);
   map["scratch/CBTaggTAPS_9339.dat.hld"] = my_config_t(sampling80, 22.5);
   
   return map;
}

const static std::map<std::string, my_config_t>  my_config =  create_my_config();

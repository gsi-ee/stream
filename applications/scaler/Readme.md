# Example of hadaq::ScalerProcessor

Demonstrates how scaler processor can be created after all TDCs were created.
One should call UserPreLoop() in such case.

Output stored as simple subevent with two scalers values.
It can be accessed from second.C or from stored ROOT TTree.

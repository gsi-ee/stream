#include <iostream>
#include <sstream>
#include <iomanip>
#include <TFile.h>
#include <TTree.h>
#include <vector>

// source streamlogin and have proper rootlogon.C
// to make Go4 stuff work
#include <hadaq/AdcSubEvent.h>
#include <hadaq/TdcSubEvent.h>
#include <hadaq/TrbProcessor.h>

using namespace std;

vector<unsigned short> getAcquADCValues(
        unsigned short id,
        vector< unsigned short >* acqu_ID,
        vector< vector<unsigned short> >* acqu_Values) {
    if(acqu_ID->size() != acqu_Values->size()) {
        cerr << "Something wrong in the acqu tree, sizes don't match..." << endl;
        exit(1);
    }
    for(size_t i=0;i<acqu_ID->size();i++) {
        if(id == (*acqu_ID)[i]) {
            return (*acqu_Values)[i];
        }
    }
    // return empty if not found
    vector<unsigned short> v;
    return v;
}

void Correlate(const char* fname = "scratch/CBTagg_9221.dat") {

#if defined(__CINT__) && !defined(__MAKECINT__)
	cout << "Needs to be compiled with ACLIC, append + to filename" << endl;
	return;
#endif

	stringstream fname1;
	fname1 << fname << ".root";

	stringstream fname2;
	fname2 << fname << ".hld.root";
	
	
	TFile* f1 = new TFile(fname1.str().c_str());
	TTree* t1 = (TTree*)f1->Get("rawADC");
	t1->SetMakeClass(1); // very important for reading STL stuff

	TFile* f2 = new TFile(fname2.str().c_str());
	TTree* t2 = (TTree*)f2->Get("T");
    // t2->SetMakeClass(1); // Go4 stuff already has compiled classes
    
	Long64_t n1 = t1->GetEntries(); 
    Long64_t n2 = t2->GetEntries(); 
    // t1 contains the events from Acqu
	// t2 contains the events from Go4
	cout << "Entries: Acqu=" << n1 << " Go4=" << n2 << endl;
		
	vector< unsigned short >* acqu_ID = 0;             // ADC ID
	vector< vector<unsigned short> >* acqu_Values = 0; // Value
	t1->SetBranchAddress("ID",    &acqu_ID);
	t1->SetBranchAddress("Value", &acqu_Values);

//    TBranch* b = t2->GetBranch("TRB_8000");
//    cout << "Branch class name: " << b->GetClassName() << endl;
    
    hadaq::TrbMessage* go4_trb = 0;
    t2->SetBranchAddress("TRB_8000", &go4_trb);
        
	vector< hadaq::AdcMessage >* go4_adc = 0;
	t2->SetBranchAddress("ADC_0200", &go4_adc);
    
    //vector<hadaq::TdcMessageExt>* go4_tdc = 0;
	//t2->SetBranchAddress("TDC_0200", &go4_tdc);
    
	
    Long64_t i1 = 0;
    Long64_t i2 = 0;    
    Long64_t matches = 0;
    while(i1 < n1 && i2 < n2) {
        t1->GetEntry(i1);
        t2->GetEntry(i2);
        // match the the serial ID
        // go4 is easily available
        if(!go4_trb->fTrigSyncIdFound) {
            cerr << "We should always have found some serial ID" << endl;
            exit(1);
        }
        const unsigned go4_EventId = go4_trb->fTrigSyncId;
        vector<unsigned short> EventId = getAcquADCValues(400, acqu_ID, acqu_Values);
        const unsigned acqu_EventId = EventId[0]; // convert 16bit to 32bit...
        
        if(go4_EventId>acqu_EventId) {
            i1++;
            continue;
        }
        else if(go4_EventId<acqu_EventId) {
            i2++;
        }
        
        matches++;
        cout << "Found match=" << matches <<", i1=" << i1 << " i2=" << i2 << endl;
        
        // dump events
        cout << ">>> TRB3 says:" << endl;
        for(size_t i=0;i<go4_adc->size();i++) {
            const hadaq::AdcMessage& msg = (*go4_adc)[i];
            cout << "    Ch=" << setw(4) << msg.getCh() << " Integral=" << msg.fIntegral << endl;
        }
        cout << ">>> Acqu says: " << endl;
        for(unsigned short ch=3368;ch<3383;ch++) {
            vector<unsigned short> values = getAcquADCValues(ch, acqu_ID, acqu_Values);
            if(values.size() != 3)
                continue;
            cout << "    Ch=" << setw(4) << ch << " Integral=" << values[1] << endl;
        }
        
        cout << endl;
        
        if(matches>100)
            break;
        
        i1++;
        i2++;
    }
    

   
	//cout << go4_adc->size() << endl;
    
    
	//cout << acqu_ID->size() << endl;
	
	
}

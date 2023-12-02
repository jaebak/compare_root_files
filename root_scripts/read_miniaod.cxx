#include <iostream>
#include "TROOT.h"
#include "TSystem.h"
#include "JTreeReaderHelper.h"

using std::cout;
using std::endl;

#if defined(__CINT__) && !defined(__MAKECINT__)
class loadFWLite {
  public:
    loadFWLite() {
      gSystem->Load("libFWCoreFWLite");
      FWLiteEnabler::enable();
    }
};
static loadFWLite lfw;
#endif

#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/FWLite/interface/Event.h"

int read_miniaod(string miniaod_filename_a="", string root_filename_b="", string tree_name="tree") {
  time_t begtime, endtime;
  time(&begtime);

  //cout<<miniaod_filename_a<<endl;
  //cout<<root_filename_b<<endl;
  //cout<<tree_name<<endl;

  //gSystem->Load("libFWCoreFWLite.so");
  //gSystem->Load("libDataFormatsFWLite.so");
  //FWLiteEnabler::enable();
  //#include "DataFormats/FWLite/interface/Handle.h"
  TFile miniaod_file(miniaod_filename_a.c_str());
  fwlite::Event events(&miniaod_file);

  fwlite::Handle<double> fixedGridRhoAll;
  fwlite::Handle<vector<pat::Photon>> slimmedPhotons;

  //Photon_energyRaw, 
  //Photon_esEffSigmaRR, 
  //Photon_esEnergyOverRawE, 
  //Photon_etaWidth, 
  //Photon_pfChargedIso, 
  //Photon_pfChargedIsoWorstVtx, 
  //Photon_pfPhoIso03, 
  //Photon_phiWidth, 
  //Photon_s4, 
  //Photon_scEta, 
  //Photon_sieip, 
  //fixedGridRhoAll,

  for( events.toBegin(); ! events.atEnd(); ++events) {
    edm::EventID id = events.id();
    cout << " Run " << id.run() << " Luminosity block "<< id.luminosityBlock() << " event " << id.event() << endl;
    fixedGridRhoAll.getByLabel(events, "fixedGridRhoAll");
    cout<<fixedGridRhoAll.ref()<<endl;
    slimmedPhotons.getByLabel(events, "slimmedPhotons");
    for (unsigned iPhoton = 0; iPhoton < slimmedPhotons.ref().size(); ++iPhoton) {
      //slimmedPhotons.ref()[iPhoton].superCluster().rawEnergy();
      cout<<slimmedPhotons.ref()[iPhoton].superCluster()->rawEnergy()<<endl;
    }
    break;
  };

  time(&endtime); 
  cout<<endl<<"Took "<<difftime(endtime, begtime)<<" seconds"<<endl<<endl;
  return 0;
}

//================================================================================================
//
// Select Z->mumu candidates
//
//  * outputs ROOT files of events passing selection
//
//________________________________________________________________________________________________
#if !defined(__CINT__) || defined(__MAKECINT__)
#include <TROOT.h>                  // access to gROOT, entry point to ROOT system
#include <TSystem.h>                // interface to OS
#include <TFile.h>                  // file handle class
#include <TTree.h>                  // class to access ntuples
#include <TClonesArray.h>           // ROOT array class
#include <TBenchmark.h>             // class to track macro running statistics
#include <TVector2.h>               // 2D vector class
#include <TMath.h>                  // ROOT math library
#include <vector>                   // STL vector class
#include <iostream>                 // standard I/O
#include <iomanip>                  // functions to format standard I/O
#include <fstream>                  // functions for file I/O
#include "TLorentzVector.h"         // 4-vector class
#include "TH1D.h"
#include "TRandom.h"

#include "ConfParse.hh"             // input conf file parser
#include "../Utils/CSample.hh"      // helper class to handle samples
#include "../Utils/LeptonCorr.hh"   // muon scale and resolution corrections

// define structures to read in ntuple
#include "BaconAna/DataFormats/interface/BaconAnaDefs.hh"
#include "BaconAna/DataFormats/interface/TEventInfo.hh"
#include "BaconAna/DataFormats/interface/TGenEventInfo.hh"
#include "BaconAna/DataFormats/interface/TGenParticle.hh"
#include "BaconAna/DataFormats/interface/TMuon.hh"
#include "BaconAna/DataFormats/interface/TVertex.hh"
#include "BaconAna/DataFormats/interface/TPhoton.hh"
#include "BaconAna/DataFormats/interface/TJet.hh"
#include "BaconAna/Utils/interface/TTrigger.hh"

// lumi section selection with JSON files
#include "BaconAna/Utils/interface/RunLumiRangeMap.hh"

#include "../Utils/LeptonIDCuts.hh" // helper functions for lepton ID selection
#include "../Utils/MyTools.hh"      // various helper functions
#include "../Utils/PrefiringEfficiency.cc"      // prefiring efficiency functions
#endif


//=== MAIN MACRO ================================================================================================= 

void selectZmm(const TString conf       ="zmm.conf", // input file
               const TString outputDir  =".",   // output directory
               const Bool_t  doScaleCorr=0,    // apply energy scale corrections
               const Bool_t  doPU       =0,
               const Bool_t  is13TeV    =1,
               const Int_t   NSEC       =1,
               const Int_t   ITH        =0 ) {
  gBenchmark->Start("selectZmm");

std::cout << "is 13 TeV " << is13TeV << std::endl;
  //--------------------------------------------------------------------------------------------------------------
  // Settings 
  //============================================================================================================== 

  const Double_t MASS_LOW  = 40;
  const Double_t MASS_HIGH = 200;
  const Double_t PT_CUT    = 25;
  const Double_t ETA_CUT   = 2.4;
  const Double_t MUON_MASS = 0.105658369;

  const Int_t BOSON_ID  = 23;
  const Int_t LEPTON_ID = 13;

  // load trigger menu                                                                                                  
  const baconhep::TTrigger triggerMenu("../../BaconAna/DataFormats/data/HLT_50nsGRun");
  
  const TString prefireFileName = "../Utils/All2017Gand2017HPrefiringMaps.root";
  PrefiringEfficiency pfire( prefireFileName.Data() , (is13TeV ? "2017H" : "2017G"));
  
 
  // load pileup reweighting file                                                                                       
  TFile *f_rw = TFile::Open("../Tools/puWeights_76x.root", "read");

  TH1D *h_rw = (TH1D*) f_rw->Get("puWeights");
  TH1D *h_rw_up = (TH1D*) f_rw->Get("puWeightsUp");
  TH1D *h_rw_down = (TH1D*) f_rw->Get("puWeightsDown");

  if (h_rw==NULL || h_rw_up==NULL || h_rw_down==NULL) cout<<"WARNING h_rw == NULL"<<endl;

  //--------------------------------------------------------------------------------------------------------------
  // Main analysis code 
  //==============================================================================================================  

  enum { eMuMu2HLT=1, eMuMu1HLT1L1, eMuMu1HLT, eMuMuNoSel, eMuSta, eMuTrk };  // event category enum
  
  vector<TString>  snamev;      // sample name (for output files)  
  vector<CSample*> samplev;     // data/MC samples

  //
  // parse .conf file
  //
  confParse(conf, snamev, samplev);
  const Bool_t hasData = (samplev[0]->fnamev.size()>0);

  // Create output directory
  gSystem->mkdir(outputDir,kTRUE);
  // const TString ntupDir = outputDir + TString("/ntuples");
  const TString ntupDir = outputDir + TString("/ntuples_") + Form("%d",ITH) + TString("_") + Form("%d",NSEC);
  gSystem->mkdir(ntupDir,kTRUE);
  
  //
  // Declare output ntuple variables
  //
  UInt_t  runNum, lumiSec, evtNum;
  UInt_t  matchGen;
  UInt_t  category;
  UInt_t  npv, npu;
  TLorentzVector *genV=0;
  Float_t genVPt, genVPhi, genVy, genVMass;
  Float_t genWeight, PUWeight;
  Float_t scale1fb,scale1fbUp,scale1fbDown;
  Float_t prefireWeight=1, prefireUp=1,    prefireDown=1;
  Float_t prefirePhoton=1, prefirePhotUp=1, prefirePhotDown=1;
  Float_t prefireJet=1,    prefireJetUp=1,  prefireJetDown=1;
  Float_t met, metPhi, u1, u2;
  Float_t puppiMet, puppiMetPhi, puppiU1, puppiU2;
  Int_t   q1, q2;
  Float_t genMuonPt1, genMuonPt2;
  TLorentzVector *dilep=0, *lep1=0, *lep2=0;
  TLorentzVector *genlep1=0;
  TLorentzVector *genlep2=0;
  ///// muon specific /////
  Float_t trkIso1, emIso1, hadIso1, pfChIso1, pfGamIso1, pfNeuIso1, pfCombIso1;
  Float_t trkIso2, emIso2, hadIso2, pfChIso2, pfGamIso2, pfNeuIso2, pfCombIso2;
  Float_t d01, dz1, d02, dz2;
  Float_t muNchi21,  muNchi22;
  UInt_t nPixHits1, nTkLayers1, nPixHits2, nTkLayers2;
  UInt_t nValidHits1, nMatch1, nValidHits2, nMatch2;
  UInt_t typeBits1, typeBits2;
  TLorentzVector *sta1=0, *sta2=0;
  Int_t glepq1=-99;
	Int_t glepq2=-99;
  
  
  // Data structures to store info from TTrees
  baconhep::TEventInfo *info   = new baconhep::TEventInfo();
  baconhep::TGenEventInfo *gen = new baconhep::TGenEventInfo();
  TClonesArray *genPartArr = new TClonesArray("baconhep::TGenParticle");
  TClonesArray *muonArr    = new TClonesArray("baconhep::TMuon");
  TClonesArray *vertexArr  = new TClonesArray("baconhep::TVertex");
  TClonesArray *scArr          = new TClonesArray("baconhep::TPhoton");
  TClonesArray *jetArr         = new TClonesArray("baconhep::TJet");
  
  TFile *infile=0;
  TTree *eventTree=0;

    
  //
  // loop over samples
  //  
  for(UInt_t isam=0; isam<samplev.size(); isam++) {
    // Assume data sample is first sample in .conf file
    // If sample is empty (i.e. contains no ntuple files), skip to next sample
    Bool_t isData=kFALSE;
    if(isam==0 && !hasData) continue;
    else if (isam==0) isData=kTRUE;

    Bool_t isSignal        = (snamev[isam].Contains("zmm"));
    Bool_t isWboson        = (snamev[isam].Contains("wx"));
    Bool_t isWrongFlavor   = (snamev[isam].Contains("zxx"));
    Bool_t isRecoil = (isWboson||isSignal||isWrongFlavor);
    Bool_t noGen    = (snamev[isam].Contains("zz")||snamev[isam].Contains("wz")||snamev[isam].Contains("ww"));
    cout << "isREcoil " << isRecoil << endl;
    
    CSample* samp = samplev[isam];
    
    //
    // Set up output ntuple
    //
    TString outfilename = ntupDir + TString("/") + snamev[isam] + TString("_select.root");
    if(isam!=0 && !doScaleCorr) outfilename = ntupDir + TString("/") + snamev[isam] + TString("_select.raw.root");
    
    TFile *outFile = new TFile(outfilename,"RECREATE"); 
    TTree *outTree = new TTree("Events","Events");
    outTree->Branch("runNum",      &runNum,     "runNum/i");      // event run number
    outTree->Branch("lumiSec",     &lumiSec,    "lumiSec/i");     // event lumi section
    outTree->Branch("evtNum",      &evtNum,     "evtNum/i");      // event number
    outTree->Branch("matchGen",    &matchGen,   "matchGen/i");    // event has both leptons matched to MC Z->ll
    outTree->Branch("category",    &category,   "category/i");    // dilepton category
    outTree->Branch("npv",         &npv,        "npv/i");         // number of primary vertices
    outTree->Branch("npu",         &npu,        "npu/i");         // number of in-time PU events (MC)
    outTree->Branch("genV",        "TLorentzVector",  &genV);     // GEN boson 4-vector (signal MC)
    outTree->Branch("genVPt",      &genVPt,     "genVPt/F");      // GEN boson pT (signal MC)
    outTree->Branch("genVPhi",     &genVPhi,    "genVPhi/F");     // GEN boson phi (signal MC)
    outTree->Branch("genVy",       &genVy,      "genVy/F");       // GEN boson rapidity (signal MC)
    outTree->Branch("genVMass",    &genVMass,   "genVMass/F");    // GEN boson mass (signal MC)
    outTree->Branch("genWeight",   &genWeight,  "genWeight/F");
    outTree->Branch("PUWeight",    &PUWeight,   "PUWeight/F");
    outTree->Branch("scale1fb",    &scale1fb,   "scale1fb/F");    // event weight per 1/fb (MC)
    outTree->Branch("scale1fbUp",    &scale1fbUp,   "scale1fbUp/F");    // event weight per 1/fb (MC)
    outTree->Branch("scale1fbDown",    &scale1fbDown,   "scale1fbDown/F");    // event weight per 1/fb (MC)
    outTree->Branch("prefireWeight", &prefireWeight, "prefireWeight/F");
    outTree->Branch("prefireUp",     &prefireUp,     "prefireUp/F");
    outTree->Branch("prefireDown",   &prefireDown,   "prefireDown/F");
    outTree->Branch("prefirePhoton", &prefirePhoton, "prefirePhoton/F");
    outTree->Branch("prefirePhotUp",     &prefirePhotUp,     "prefirePhotUp/F");
    outTree->Branch("prefirePhotDown",   &prefirePhotDown,   "prefirePhotDown/F");
    outTree->Branch("prefireJet",    &prefireJet,    "prefireJet/F");
    outTree->Branch("prefireJetUp",  &prefireJetUp,  "prefireJetUp/F");
    outTree->Branch("prefireJetDown",&prefireJetDown,"prefireJetDown/F");
    outTree->Branch("met",      &met,        "met/F");         // MET
    outTree->Branch("metPhi",   &metPhi,     "metPhi/F");      // phi(MET)
    outTree->Branch("u1",       &u1,         "u1/F");          // parallel component of recoil
    outTree->Branch("u2",       &u2,         "u2/F");          // perpendicular component of recoil
    outTree->Branch("puppiMet",    &puppiMet,   "puppiMet/F");      // Puppi MET
    outTree->Branch("puppiMetPhi", &puppiMetPhi,"puppiMetPhi/F");   // phi(Puppi MET)
    outTree->Branch("puppiU1",     &puppiU1,    "puppiU1/F");       // parallel component of recoil (Puppi MET)
    outTree->Branch("puppiU2",     &puppiU2,    "puppiU2/F");       // perpendicular component of recoil (Puppi MET)
    outTree->Branch("q1",          &q1,         "q1/I");          // charge of tag lepton
    outTree->Branch("q2",          &q2,         "q2/I");          // charge of probe lepton
    outTree->Branch("glepq1",      &glepq1,     "glepq1/I");          // charge of tag lepton
    outTree->Branch("glepq2",      &glepq2,     "glepq2/I");          // charge of probe lepton
    outTree->Branch("genMuonPt1",  &genMuonPt1,  "genMuonPt1/F");          // charge of probe lepton
    outTree->Branch("genMuonPt2",  &genMuonPt2,  "genMuonPt2/F");          // charge of probe lepton
    outTree->Branch("dilep",       "TLorentzVector", &dilep);     // di-lepton 4-vector
    outTree->Branch("lep1",        "TLorentzVector", &lep1);      // tag lepton 4-vector
    outTree->Branch("lep2",        "TLorentzVector", &lep2);      // probe lepton 4-vector
    outTree->Branch("genlep1",     "TLorentzVector",  &genlep1);     // tag lepton 4-vector
    outTree->Branch("genlep2",     "TLorentzVector",  &genlep2);     // probe lepton 4-vector
    ///// muon specific /////
    outTree->Branch("trkIso1",     &trkIso1,     "trkIso1/F");       // track isolation of tag lepton
    outTree->Branch("trkIso2",     &trkIso2,     "trkIso2/F");       // track isolation of probe lepton
    outTree->Branch("emIso1",      &emIso1,      "emIso1/F");        // ECAL isolation of tag lepton
    outTree->Branch("emIso2",      &emIso2,      "emIso2/F");        // ECAL isolation of probe lepton
    outTree->Branch("hadIso1",     &hadIso1,     "hadIso1/F");       // HCAL isolation of tag lepton
    outTree->Branch("hadIso2",     &hadIso2,     "hadIso2/F");       // HCAL isolation of probe lepton
    outTree->Branch("pfChIso1",    &pfChIso1,    "pfChIso1/F");      // PF charged hadron isolation of tag lepton
    outTree->Branch("pfChIso2",    &pfChIso2,    "pfChIso2/F");      // PF charged hadron isolation of probe lepton
    outTree->Branch("pfGamIso1",   &pfGamIso1,   "pfGamIso1/F");     // PF photon isolation of tag lepton
    outTree->Branch("pfGamIso2",   &pfGamIso2,   "pfGamIso2/F");     // PF photon isolation of probe lepton
    outTree->Branch("pfNeuIso1",   &pfNeuIso1,   "pfNeuIso1/F");     // PF neutral hadron isolation of tag lepton
    outTree->Branch("pfNeuIso2",   &pfNeuIso2,   "pfNeuIso2/F");     // PF neutral hadron isolation of probe lepton
    outTree->Branch("pfCombIso1",  &pfCombIso1,  "pfCombIso1/F");    // PF combined isolation of tag lepton
    outTree->Branch("pfCombIso2",  &pfCombIso2,  "pfCombIso2/F");    // PF combined isolation of probe lepton    
    outTree->Branch("d01",         &d01,         "d01/F");           // transverse impact parameter of tag lepton
    outTree->Branch("d02",         &d02,         "d02/F");           // transverse impact parameter of probe lepton	 
    outTree->Branch("dz1",         &dz1,         "dz1/F");           // longitudinal impact parameter of tag lepton
    outTree->Branch("dz2",         &dz2,         "dz2/F");           // longitudinal impact parameter of probe lepton	 
    outTree->Branch("muNchi21",    &muNchi21,    "muNchi21/F");      // muon fit normalized chi^2 of tag lepton
    outTree->Branch("muNchi22",    &muNchi22,    "muNchi22/F");      // muon fit normalized chi^2 of probe lepton
    outTree->Branch("nPixHits1",   &nPixHits1,	 "nPixHits1/i");     // number of pixel hits of tag muon
    outTree->Branch("nPixHits2",   &nPixHits2,	 "nPixHits2/i");     // number of pixel hits of probe muon
    outTree->Branch("nTkLayers1",  &nTkLayers1,  "nTkLayers1/i");    // number of tracker layers of tag muon
    outTree->Branch("nTkLayers2",  &nTkLayers2,  "nTkLayers2/i");    // number of tracker layers of probe muon
    outTree->Branch("nMatch1",     &nMatch1,	 "nMatch1/i");       // number of matched segments of tag muon
    outTree->Branch("nMatch2",     &nMatch2,	 "nMatch2/i");       // number of matched segments of probe muon 
    outTree->Branch("nValidHits1", &nValidHits1, "nValidHits1/i");   // number of valid muon hits of tag muon
    outTree->Branch("nValidHits2", &nValidHits2, "nValidHits2/i");   // number of valid muon hits of probe muon
    outTree->Branch("typeBits1",   &typeBits1,   "typeBits1/i");     // muon type of tag muon
    outTree->Branch("typeBits2",   &typeBits2,   "typeBits2/i");     // muon type of probe muon
    outTree->Branch("sta1",        "TLorentzVector", &sta1);         // tag standalone muon 4-vector
    outTree->Branch("sta2",        "TLorentzVector", &sta2);         // probe standalone muon 4-vector
    
    TH1D* hGenWeights = new TH1D("hGenWeights","hGenWeights",10,-10.,10.);
    //
    // loop through files
    //
    const UInt_t nfiles = samp->fnamev.size();
    for(UInt_t ifile=0; ifile<nfiles; ifile++) {  
      
      // Read input file and get the TTrees
      cout << "Processing " << samp->fnamev[ifile] << " [xsec = " << samp->xsecv[ifile] << " pb] ... "; cout.flush();
      infile = TFile::Open(samp->fnamev[ifile]); 
      assert(infile);
      if (samp->fnamev[ifile] == "/dev/null") 
	      {
          cout <<"-> Ignoring null input "<<endl; 
          continue;
	      }


      Bool_t hasJSON = kFALSE;
      baconhep::RunLumiRangeMap rlrm;
      if(samp->jsonv[ifile].Contains("NONE")!=0) { 
        hasJSON = kTRUE;
        rlrm.addJSONFile(samp->jsonv[ifile].Data()); 
      }
  
      eventTree = (TTree*)infile->Get("Events"); assert(eventTree);  
      
      Bool_t hasJet = eventTree->GetBranchStatus("AK4");
      
      eventTree->SetBranchAddress("Info", &info);      TBranch *infoBr = eventTree->GetBranch("Info");
      eventTree->SetBranchAddress("Muon", &muonArr);   TBranch *muonBr = eventTree->GetBranch("Muon");
      eventTree->SetBranchAddress("PV",   &vertexArr); TBranch *vertexBr = eventTree->GetBranch("PV");
      eventTree->SetBranchAddress("Photon",   &scArr);       TBranch *scBr       = eventTree->GetBranch("Photon");
      if(hasJet) eventTree->SetBranchAddress("AK4",      &jetArr     ); TBranch *jetBr      = eventTree->GetBranch("AK4");
      Bool_t hasGen = (eventTree->GetBranchStatus("GenEvtInfo")&&!noGen);
      TBranch *genBr=0, *genPartBr=0;
      if(hasGen) {
        eventTree->SetBranchAddress("GenEvtInfo", &gen); genBr = eventTree->GetBranch("GenEvtInfo");
        eventTree->SetBranchAddress("GenParticle",&genPartArr); genPartBr = eventTree->GetBranch("GenParticle");
      }

      // Compute MC event weight per 1/fb
      const Double_t xsec = samp->xsecv[ifile];
      // Double_t totalWeight=0, totalWeightUp=0,  totalWeightDown=0;
      Double_t puWeight=0, puWeightUp=0,  puWeightDown=0;
      Double_t nsel=0, nselvar=0;

      //
      // loop over events
      //

      double frac = 1.0/NSEC;
      UInt_t IBEGIN = frac*ITH*eventTree->GetEntries();
      UInt_t IEND = frac*(ITH+1)*eventTree->GetEntries();
      
      for(UInt_t ientry=IBEGIN; ientry < IEND; ientry++) {
        infoBr->GetEntry(ientry);

        int printIndex = (int)(eventTree->GetEntries()*0.01);
        if(ientry%printIndex==0) cout << "Processing event " << ientry << ". " << (int)(100*(ientry/(double)eventTree->GetEntries())) << " percent done with this file." << endl;

        Double_t weight = xsec;
        Double_t weightUp = xsec;
        Double_t weightDown = xsec;
        if(hasGen) {
          genPartArr->Clear();
          genBr->GetEntry(ientry);
          genPartBr->GetEntry(ientry);
          puWeight = doPU ? h_rw->GetBinContent(h_rw->FindBin(info->nPUmean)) : 1.;
          puWeightUp = doPU ? h_rw_up->GetBinContent(h_rw_up->FindBin(info->nPUmean)) : 1.;
          puWeightDown = doPU ? h_rw_down->GetBinContent(h_rw_down->FindBin(info->nPUmean)) : 1.;
          hGenWeights->Fill(0.0,gen->weight);
          weight*=gen->weight*puWeight;
          weightUp*=gen->weight*puWeightUp;
          weightDown*=gen->weight*puWeightDown;
        } else { // i guess should fix this if it needs PU weighting
          hGenWeights->Fill(0.0,1.0);
        }
          

        // cout << "hello" << endl;
        // veto z -> xx decays for signal and z -> mm for bacground samples (needed for inclusive DYToLL sample)
        if (isWrongFlavor && hasGen && fabs(toolbox::flavor(genPartArr, BOSON_ID))==LEPTON_ID) continue;
        else if (isSignal && hasGen && fabs(toolbox::flavor(genPartArr, BOSON_ID))!=LEPTON_ID) continue;
     
        // check for certified lumi (if applicable)
        baconhep::RunLumiRangeMap::RunLumiPairType rl(info->runNum, info->lumiSec);      
        if(hasJSON && !rlrm.hasRunLumi(rl)) continue;

        // trigger requirement               
        if (!isMuonTrigger(triggerMenu, info->triggerBits,isData,is13TeV)) continue;

        // good vertex requirement
        if(!(info->hasGoodPV)) continue;
        


        muonArr->Clear();
        muonBr->GetEntry(ientry);
        scArr->Clear();
        scBr->GetEntry(ientry);
        
        jetArr->Clear();
        if(hasJet)jetBr->GetEntry(ientry);

        TLorentzVector vTag(0,0,0,0);
        TLorentzVector vTagSta(0,0,0,0);
        Double_t tagPt=0;
        Double_t Pt1=0;
        Double_t Pt2=0;
        Int_t itag=-1;
	
        for(Int_t i1=0; i1<muonArr->GetEntriesFast(); i1++) {
          const baconhep::TMuon *tag = (baconhep::TMuon*)((*muonArr)[i1]);

          // apply scale and resolution corrections to MC
          Double_t tagpt_corr = tag->pt;
          if(doScaleCorr && !snamev[isam].Contains("data")){
            tagpt_corr = gRandom->Gaus(tag->pt*getMuScaleCorr(tag->eta,0),getMuResCorr(tag->eta,0));
          }
	
          if(tagpt_corr     < PT_CUT)        continue;  // lepton pT cut
          if(fabs(tag->eta) > ETA_CUT)       continue;  // lepton |eta| cut
          if(!passMuonID(tag))               continue;  // lepton selection

          double Mu_Pt=0;
          if(doScaleCorr) Mu_Pt=gRandom->Gaus(tag->pt*getMuScaleCorr(tag->eta,0),getMuResCorr(tag->eta,0));
          else Mu_Pt=tag->pt;

          if(Mu_Pt>Pt1) {
            Pt2=Pt1;
            Pt1=Mu_Pt;
          } else if(Mu_Pt>Pt2&&Mu_Pt<Pt1){
            Pt2=Mu_Pt;
          }

          if(!isMuonTriggerObj(triggerMenu, tag->hltMatchBits, isData,is13TeV)) continue;
          if(Mu_Pt<tagPt) continue;
          tagPt=Mu_Pt;
          itag=i1;
        
          // apply scale and resolution corrections to MC
          if(doScaleCorr && !snamev[isam].Contains("data")) {
            vTag.SetPtEtaPhiM(tagpt_corr,tag->eta,tag->phi,MUON_MASS);
            vTagSta.SetPtEtaPhiM(gRandom->Gaus(tag->staPt*getMuScaleCorr(tag->eta,0),getMuResCorr(tag->eta,0)),tag->staEta,tag->staPhi,MUON_MASS);
          } else {
            vTag.SetPtEtaPhiM(tag->pt,tag->eta,tag->phi,MUON_MASS);
            vTagSta.SetPtEtaPhiM(tag->staPt,tag->staEta,tag->staPhi,MUON_MASS);
          }

          trkIso1     = tag->trkIso;
          emIso1      = tag->ecalIso;	    
          hadIso1     = tag->hcalIso;
          pfChIso1    = tag->chHadIso;
          pfGamIso1   = tag->gammaIso;
          pfNeuIso1   = tag->neuHadIso;
          pfCombIso1  = tag->chHadIso + TMath::Max(tag->neuHadIso + tag->gammaIso - 
                     0.5*(tag->puIso),Double_t(0));
          d01         = tag->d0;
          dz1         = tag->dz;
          muNchi21    = tag->muNchi2;
          nPixHits1   = tag->nPixHits;
          nTkLayers1  = tag->nTkLayers;
          nMatch1     = tag->nMatchStn;
          nValidHits1 = tag->nValidHits;
          typeBits1   = tag->typeBits;
          q1 = tag->q;
        }

        if(tagPt<Pt2) continue;

        TLorentzVector vProbe(0,0,0,0); TLorentzVector vProbeSta(0,0,0,0);
        Double_t probePt=0;
        Int_t passID=false;
        UInt_t icat=0;

        for(Int_t i2=0; i2<muonArr->GetEntriesFast(); i2++) {
          if(itag==i2) continue;
          const baconhep::TMuon *probe = (baconhep::TMuon*)((*muonArr)[i2]);
          

          // apply scale and resolution corrections to MC
          Double_t probept_corr = probe->pt;
          if(doScaleCorr && !snamev[isam].Contains("data"))
            probept_corr = gRandom->Gaus(probe->pt*getMuScaleCorr(probe->eta,0),getMuResCorr(probe->eta,0));

          if(probept_corr     < PT_CUT)  continue;  // lepton pT cut
          if(fabs(probe->eta) > ETA_CUT) continue;  // lepton |eta| cut

          double Mu_Pt=probept_corr;
        
          if(passID&&passMuonID(probe)&&Mu_Pt<probePt) continue;
          if(passID&&!passMuonID(probe)) continue;
          if(!passID&&!passMuonID(probe)&&Mu_Pt<probePt) continue;

          if(!passID&&passMuonID(probe)) passID=true;

          probePt=Mu_Pt;

          // apply scale and resolution corrections to MC
          if(doScaleCorr && !snamev[isam].Contains("data")) {
            vProbe.SetPtEtaPhiM(probept_corr,probe->eta,probe->phi,MUON_MASS);
            if(probe->typeBits & baconhep::EMuType::kStandalone)
              vProbeSta.SetPtEtaPhiM(gRandom->Gaus(probe->staPt*getMuScaleCorr(probe->eta,0),getMuResCorr(probe->eta,0)),probe->staEta,probe->staPhi,MUON_MASS);
          } else {
            vProbe.SetPtEtaPhiM(probe->pt,probe->eta,probe->phi,MUON_MASS);
            if(probe->typeBits & baconhep::EMuType::kStandalone)
            vProbeSta.SetPtEtaPhiM(probe->staPt,probe->staEta,probe->staPhi,MUON_MASS);
          }

          trkIso2     = probe->trkIso;
          emIso2      = probe->ecalIso;
          hadIso2     = probe->hcalIso;
          pfChIso2    = probe->chHadIso;
          pfGamIso2   = probe->gammaIso;
          pfNeuIso2   = probe->neuHadIso;
          pfCombIso2  = probe->chHadIso + TMath::Max(probe->neuHadIso + probe->gammaIso - 
                       0.5*(probe->puIso),Double_t(0));
          d02         = probe->d0;
          dz2         = probe->dz;
          muNchi22    = probe->muNchi2;
          nPixHits2   = probe->nPixHits;
          nTkLayers2  = probe->nTkLayers;
          nMatch2     = probe->nMatchStn;
          nValidHits2 = probe->nValidHits;
          typeBits2   = probe->typeBits;
          q2 = probe->q;

          // determine event category
          if(passMuonID(probe)) {
            if(isMuonTriggerObj(triggerMenu, probe->hltMatchBits, isData,is13TeV)) {
              icat=eMuMu2HLT;
            }
            else if(0) {
              icat=eMuMu1HLT1L1;
          }
            else {
              icat=eMuMu1HLT;
            }
          }
          else if(probe->typeBits & baconhep::EMuType::kGlobal) { icat=eMuMuNoSel; }
          else if(probe->typeBits & baconhep::EMuType::kStandalone) { icat=eMuSta; }
          else if(probe->nTkLayers>=6 && probe->nPixHits>=1)        { icat=eMuTrk; }
        }
        
        if(q1 == q2)         continue;  // opposite charge requirement
            
        // mass window
        TLorentzVector vDilep = vTag + vProbe;
        if((vDilep.M()<MASS_LOW) || (vDilep.M()>MASS_HIGH)) continue;
        if(icat==0) continue;
          
        if(!isData){
          pfire.setObjects(scArr,jetArr);
          pfire.computePhotonsOnly(prefirePhoton, prefirePhotUp, prefirePhotDown);
          pfire.computeJetsOnly   (prefireJet   , prefireJetUp , prefireJetDown );
          pfire.computeFullPrefire(prefireWeight, prefireUp    , prefireDown    );
        }

        /******** We have a Z candidate! HURRAY! ********/
        nsel+=isData ? 1 : weight;
        nselvar+=isData ? 1 : weight*weight;
        // Int_t glepq1=-99;
        // Int_t glepq2=-99;
        TLorentzVector *gvec=new TLorentzVector(0,0,0,0);
        TLorentzVector *glep1=new TLorentzVector(0,0,0,0);
        TLorentzVector *glep2=new TLorentzVector(0,0,0,0);
        TLorentzVector *gph=new TLorentzVector(0,0,0,0);
        Bool_t hasGenMatch = kFALSE;
        if(isRecoil && hasGen) {
          // cout << "what" << endl;
          toolbox::fillGen(genPartArr, BOSON_ID, gvec, glep1, glep2,&glepq1,&glepq2,1);
          // Test this mass cut
          if(gvec->M()<MASS_LOW || gvec->M()>MASS_HIGH) continue;
          
          Bool_t match1 = ( ((glep1) && toolbox::deltaR(vTag.Eta(), vTag.Phi(), glep1->Eta(), glep1->Phi())<0.5) ||
                ((glep2) && toolbox::deltaR(vTag.Eta(), vTag.Phi(), glep2->Eta(), glep2->Phi())<0.5) );
          
          Bool_t match2 = ( ((glep1) && toolbox::deltaR(vProbe.Eta(), vProbe.Phi(), glep1->Eta(), glep1->Phi())<0.5) ||
                ((glep2) && toolbox::deltaR(vProbe.Eta(), vProbe.Phi(), glep2->Eta(), glep2->Phi())<0.5) );

          TLorentzVector tvec=*glep1+*glep2;
          genV=new TLorentzVector(0,0,0,0);
          genV->SetPtEtaPhiM(tvec.Pt(), tvec.Eta(), tvec.Phi(), tvec.M());
          genVPt   = tvec.Pt();
          genVPhi  = tvec.Phi();
          genVy    = tvec.Rapidity();
          genVMass = tvec.M();
          genlep1=new TLorentzVector(0,0,0,0);
          genlep2=new TLorentzVector(0,0,0,0);
          genlep1->SetPtEtaPhiM(glep1->Pt(),glep1->Eta(),glep1->Phi(),glep1->M());
          genlep2->SetPtEtaPhiM(glep2->Pt(),glep2->Eta(),glep2->Phi(),glep2->M());

          delete gvec;
          // delete glep1;
          // delete glep2;
          // glep1=0; glep2=0; gvec=0;
          
          if(match1 && match2) {
            hasGenMatch = kTRUE;
          }
        }
	
        if (hasGen) {
          genMuonPt1 = toolbox::getGenLep(genPartArr, vTag);
          genMuonPt2 = toolbox::getGenLep(genPartArr, vProbe);
        }
	
        //
        // Fill tree
        //
        runNum   = info->runNum;
        lumiSec  = info->lumiSec;
        evtNum   = info->evtNum;
        
        if (hasGenMatch) matchGen=1;
        else matchGen=0;
        
        category = icat;
        
        vertexArr->Clear();
        vertexBr->GetEntry(ientry);
        
        npv      = vertexArr->GetEntries();
        npu      = info->nPUmean;
        genWeight= hasGen ? gen->weight: 1.;
        PUWeight = puWeight;
        scale1fb = weight;
        scale1fbUp = weightUp;
        scale1fbDown = weightDown;
        met      = info->pfMETC;
        metPhi   = info->pfMETCphi;

        puppiMet   = info->puppET;
        puppiMetPhi = info->puppETphi;
        lep1       = &vTag;
        lep2       = &vProbe;
        dilep      = &vDilep;
        sta1       = &vTagSta;
        sta2       = &vProbeSta;
        
        
        TVector2 vZPt((vDilep.Pt())*cos(vDilep.Phi()),(vDilep.Pt())*sin(vDilep.Phi()));
        
        // maybe someone should try using something called a function
        
        TVector2 vMet((info->pfMETC)*cos(info->pfMETCphi), (info->pfMETC)*sin(info->pfMETCphi));
        TVector2 vU = -1.0*(vMet+vZPt);
        u1 = ((vDilep.Px())*(vU.Px()) + (vDilep.Py())*(vU.Py()))/(vDilep.Pt());  // u1 = (pT . u)/|pT|
        u2 = ((vDilep.Px())*(vU.Py()) - (vDilep.Py())*(vU.Px()))/(vDilep.Pt());  // u2 = (pT x u)/|pT|
        
        TVector2 vPuppiMet((info->puppET)*cos(info->puppETphi), (info->puppET)*sin(info->puppETphi));
        TVector2 vPuppiU = -1.0*(vPuppiMet+vZPt);
        puppiU1 = ((vDilep.Px())*(vPuppiU.Px()) + (vDilep.Py())*(vPuppiU.Py()))/(vDilep.Pt());  // u1 = (pT . u)/|pT|
        puppiU2 = ((vDilep.Px())*(vPuppiU.Py()) - (vDilep.Py())*(vPuppiU.Px()))/(vDilep.Pt());  // u2 = (pT x u)/|pT|
	
        outTree->Fill();
        delete genV;
        delete genlep1;
        delete genlep2;
        genV=0, dilep=0, lep1=0, lep2=0, sta1=0, sta2=0, genlep1=0, genlep2=0;
        // reset everything to 1
        prefirePhoton=1; prefirePhotUp=1; prefirePhotDown=1;
        prefireJet   =1; prefireJetUp =1; prefireJetDown =1;
        prefireWeight=1; prefireUp    =1; prefireDown    =1;
      }
      delete infile;
      infile=0, eventTree=0;    
      
      cout << nsel  << " +/- " << sqrt(nselvar);
      if(!isData) cout << " per 1/fb";
      cout << endl;
    }
    outFile->cd();
    hGenWeights->Write();
    outFile->Write();
    outFile->Close(); 
  }
  delete h_rw;
  delete h_rw_up;
  delete h_rw_down;
  delete f_rw;
  delete info;
  delete gen;
  delete genPartArr;
  delete muonArr;
  delete vertexArr;
    
  //--------------------------------------------------------------------------------------------------------------
  // Output
  //==============================================================================================================
   
  cout << "*" << endl;
  cout << "* SUMMARY" << endl;
  cout << "*--------------------------------------------------" << endl;
  cout << " Z -> mu mu" << endl;
  cout << "  Mass window: [" << MASS_LOW << ", " << MASS_HIGH << "]" << endl;
  cout << "  pT > " << PT_CUT << endl;
  cout << "  |eta| < " << ETA_CUT << endl;
  cout << endl;
  
  cout << endl;
  cout << "  <> Output saved in " << outputDir << "/" << endl;    
  cout << endl;  
      
  gBenchmark->Show("selectZmm"); 
}

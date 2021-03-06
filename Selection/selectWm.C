//================================================================================================
//
// Select W->munu candidates
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

// define structures to read in ntuple
#include "BaconAna/DataFormats/interface/BaconAnaDefs.hh"
#include "BaconAna/DataFormats/interface/TEventInfo.hh"
#include "BaconAna/DataFormats/interface/TGenEventInfo.hh"
#include "BaconAna/DataFormats/interface/TGenParticle.hh"
#include "BaconAna/DataFormats/interface/TMuon.hh"
#include "BaconAna/DataFormats/interface/TPhoton.hh"
#include "BaconAna/DataFormats/interface/TVertex.hh"
#include "BaconAna/DataFormats/interface/TJet.hh"
#include "BaconAna/Utils/interface/TTrigger.hh"

// lumi section selection with JSON files
#include "BaconAna/Utils/interface/RunLumiRangeMap.hh"

#include "ConfParse.hh"             // input conf file parser
#include "../Utils/CSample.hh"      // helper class to handle samples
#include "../Utils/LeptonCorr.hh"   // muon scale and resolution corrections
#include "../EleScale/EnergyScaleCorrection.h" //EGMSmear

#include "../Utils/LeptonIDCuts.hh" // helper functions for lepton ID selection
#include "../Utils/MyTools.hh"      // various helper functions
#include "../Utils/PrefiringEfficiency.cc"      // prefiring efficiency functions
#endif

//=== MAIN MACRO ================================================================================================= 

void selectWm(const TString conf       ="wm.conf", // input file
              const TString outputDir  =".",  // output directory
	            const Bool_t  doScaleCorr=0,   // apply energy scale corrections?
              const Bool_t  doPU       =0,
              const Bool_t  is13TeV    =1,
              const Int_t   NSEC       =1,
              const Int_t   ITH        =0 ) {
  gBenchmark->Start("selectWm");

  //--------------------------------------------------------------------------------------------------------------
  // Settings 
  //============================================================================================================== 

  const Double_t PT_CUT    = 20;
  const Double_t ETA_CUT   = 2.4;
  const Double_t MUON_MASS = 0.105658369;
  const Double_t ELE_MASS  = 0.000511;

  const Double_t VETO_PT   = 10;
  const Double_t VETO_ETA  = 2.4;

  const Int_t BOSON_ID  = 24;
  const Int_t LEPTON_ID = 13;

  // load trigger menu
  const baconhep::TTrigger triggerMenu("../../BaconAna/DataFormats/data/HLT_50nsGRun");

  const TString prefireFileName = "../Utils/All2017Gand2017HPrefiringMaps.root";
  PrefiringEfficiency pfire( prefireFileName.Data() , (is13TeV ? "2017H" : "2017G"));

  const TString corrFiles = "/afs/cern.ch/work/s/sabrandt/public/SM/LowPU/CMSSW_9_4_12/src/MitEwk13TeV/EleScale/Run2017_LowPU_v2";
  EnergyScaleCorrection eleCorr( corrFiles.Data(), EnergyScaleCorrection::ECALELF); 
  
  // load pileup reweighting file
  TFile *f_rw = TFile::Open("../Tools/puWeights_76x.root", "read");

  TH1D *h_rw = (TH1D*) f_rw->Get("puWeights");
  TH1D *h_rw_up = (TH1D*) f_rw->Get("puWeightsUp");
  TH1D *h_rw_down = (TH1D*) f_rw->Get("puWeightsDown");

  //--------------------------------------------------------------------------------------------------------------
  // Main analysis code 
  //==============================================================================================================  

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
  UInt_t  npv, npu;
  TLorentzVector *genV=0, *genLep=0, *genNu = 0;//, *genMuonMatch=0;
  Float_t genMuonPt;
  Float_t genVPt, genVPhi, genVy, genVMass;
  Float_t genLepPt, genLepPhi;
  Float_t genNuPt,  genNuPhi;
  Float_t scale1fb, scale1fbUp, scale1fbDown;
  Float_t prefireWeight=1, prefireUp=1,    prefireDown=1;
  Float_t prefirePhoton=1, prefirePhotUp=1, prefirePhotDown=1;
  Float_t prefireJet=1,    prefireJetUp=1,  prefireJetDown=1;
  Float_t met, metPhi, mt, u1, u2;
  Float_t puppiMet, puppiMetPhi, puppiMt, puppiU1, puppiU2;
  Int_t   q;
  TLorentzVector *lep=0;
  Int_t lepID;
  ///// muon specific /////
  Float_t trkIso, emIso, hadIso;
  Float_t pfChIso, pfGamIso, pfNeuIso, pfCombIso;
  Float_t d0, dz;
  Float_t muNchi2;
  UInt_t nPixHits, nTkLayers, nValidHits, nMatch, typeBits;
  // Bool_t passHLT;
  // Bool_t passVeto=kTRUE;
  
  // Data structures to store info from TTrees
  baconhep::TEventInfo *info    = new baconhep::TEventInfo();
  baconhep::TGenEventInfo *gen  = new baconhep::TGenEventInfo();
  TClonesArray *genPartArr      = new TClonesArray("baconhep::TGenParticle");
  TClonesArray *muonArr         = new TClonesArray("baconhep::TMuon");
  TClonesArray *electronArr     = new TClonesArray("baconhep::TElectron");
  TClonesArray *vertexArr       = new TClonesArray("baconhep::TVertex");
  TClonesArray *scArr           = new TClonesArray("baconhep::TPhoton");
  TClonesArray *jetArr          = new TClonesArray("baconhep::TJet");
  
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

    Bool_t isSignal        = (snamev[isam].Contains("wm"));
    Bool_t isWrongFlavor   = (snamev[isam].Contains("wx"));
    Bool_t isRecoil = (snamev[isam].Contains("zxx")||isSignal||isWrongFlavor);
    Bool_t noGen    = (snamev[isam].Contains("zz")||snamev[isam].Contains("wz")||snamev[isam].Contains("ww"));
    CSample* samp = samplev[isam];

    //
    // Set up output ntuple
    //
    TString outfilename = ntupDir + TString("/") + snamev[isam] + TString("_select.root");
    if(isam!=0 && !doScaleCorr) outfilename = ntupDir + TString("/") + snamev[isam] + TString("_select.raw.root");
    cout << outfilename << endl;
    TFile *outFile = new TFile(outfilename,"RECREATE"); 
    TTree *outTree = new TTree("Events","Events");
    outTree->Branch("runNum",     &runNum,     "runNum/i");      // event run number
    outTree->Branch("lumiSec",    &lumiSec,    "lumiSec/i");     // event lumi section
    outTree->Branch("evtNum",     &evtNum,     "evtNum/i");      // event number
    outTree->Branch("npv",        &npv,        "npv/i");         // number of primary vertices
    outTree->Branch("npu",        &npu,        "npu/i");         // number of in-time PU events (MC)
    outTree->Branch("genV",       "TLorentzVector", &genV);      // GEN boson 4-vector (signal MC)
    outTree->Branch("genLep",     "TLorentzVector", &genLep);    // GEN lepton 4-vector (signal MC)
    outTree->Branch("genNu",      "TLorentzVector", &genNu);    // GEN lepton 4-vector (signal MC)
    outTree->Branch("genMuonPt",  &genMuonPt,     "genMuonPt/F");      // GEN boson pT (signal MC)
    outTree->Branch("genVPt",     &genVPt,     "genVPt/F");      // GEN boson pT (signal MC)
    outTree->Branch("genVPhi",    &genVPhi,    "genVPhi/F");     // GEN boson phi (signal MC)
    outTree->Branch("genVy",      &genVy,      "genVy/F");       // GEN boson rapidity (signal MC)
    outTree->Branch("genVMass",   &genVMass,   "genVMass/F");    // GEN boson mass (signal MC)
    outTree->Branch("genLepPt",   &genLepPt,   "genLepPt/F");    // GEN lepton pT (signal MC)
    outTree->Branch("genLepPhi",  &genLepPhi,  "genLepPhi/F");   // GEN lepton phi (signal MC)
    outTree->Branch("genNuPt",    &genNuPt,    "genNuPt/F");    // GEN lepton pT (signal MC)
    outTree->Branch("genNuPhi",   &genNuPhi,   "genNuPhi/F");   // GEN lepton phi (signal MC)
    outTree->Branch("prefireWeight", &prefireWeight, "prefireWeight/F");
    outTree->Branch("prefireUp",     &prefireUp,     "prefireUp/F");
    outTree->Branch("prefireDown",   &prefireDown,   "prefireDown/F");
    outTree->Branch("prefirePhoton", &prefirePhoton, "prefirePhoton/F");
    outTree->Branch("prefirePhotUp",     &prefirePhotUp,     "prefirePhotUp/F");
    outTree->Branch("prefirePhotDown",   &prefirePhotDown,   "prefirePhotDown/F");
    outTree->Branch("prefireJet",    &prefireJet,    "prefireJet/F");
    outTree->Branch("prefireJetUp",  &prefireJetUp,  "prefireJetUp/F");
    outTree->Branch("prefireJetDown",&prefireJetDown,"prefireJetDown/F");
    outTree->Branch("scale1fb",   &scale1fb,   "scale1fb/F");    // event weight per 1/fb (MC)
    outTree->Branch("scale1fbUp",   &scale1fbUp,   "scale1fbUp/F");    // event weight per 1/fb (MC)
    outTree->Branch("scale1fbDown",   &scale1fbDown,   "scale1fbDown/F");    // event weight per 1/fb (MC)
    outTree->Branch("met",        &met,        "met/F");         // MET
    outTree->Branch("metPhi",     &metPhi,     "metPhi/F");      // phi(MET)
    outTree->Branch("mt",         &mt,         "mt/F");          // transverse mass
    outTree->Branch("u1",         &u1,         "u1/F");          // parallel component of recoil
    outTree->Branch("u2",         &u2,         "u2/F");          // perpendicular component of recoil
    outTree->Branch("puppiMet",    &puppiMet,   "puppiMet/F");      // Puppi MET
    outTree->Branch("puppiMetPhi", &puppiMetPhi,"puppiMetPhi/F");   // phi(Puppi MET)
    outTree->Branch("puppiU1",     &puppiU1,    "puppiU1/F");       // parallel component of recoil (Puppi MET)
    outTree->Branch("puppiU2",     &puppiU2,    "puppiU2/F");       // perpendicular component of recoil (Puppi MET)
    outTree->Branch("q",          &q,          "q/I");           // lepton charge
    outTree->Branch("lep",        "TLorentzVector", &lep);       // lepton 4-vector
    outTree->Branch("lepID",      &lepID,      "lepID/I");       // lepton PDG ID
    ///// muon specific /////
    outTree->Branch("trkIso",     &trkIso,     "trkIso/F");       // track isolation of lepton
    outTree->Branch("emIso",      &emIso,      "emIso/F");        // ECAL isolation of lepton
    outTree->Branch("hadIso",     &hadIso,     "hadIso/F");       // HCAL isolation of lepton
    outTree->Branch("pfChIso",    &pfChIso,    "pfChIso/F");      // PF charged hadron isolation of lepton
    outTree->Branch("pfGamIso",   &pfGamIso,   "pfGamIso/F");     // PF photon isolation of lepton
    outTree->Branch("pfNeuIso",   &pfNeuIso,   "pfNeuIso/F");     // PF neutral hadron isolation of lepton
    outTree->Branch("pfCombIso",  &pfCombIso,  "pfCombIso/F");    // PF combined isolation of lepton
    outTree->Branch("d0",         &d0,         "d0/F");           // transverse impact parameter of lepton
    outTree->Branch("dz",         &dz,         "dz/F");           // longitudinal impact parameter of lepton
    outTree->Branch("muNchi2",    &muNchi2,    "muNchi2/F");      // muon fit normalized chi^2 of lepton
    outTree->Branch("nPixHits",   &nPixHits,   "nPixHits/i");	  // number of pixel hits of muon
    outTree->Branch("nTkLayers",  &nTkLayers,  "nTkLayers/i");	  // number of tracker layers of muon
    outTree->Branch("nMatch",     &nMatch,     "nMatch/i");	  // number of matched segments of muon	 
    outTree->Branch("nValidHits", &nValidHits, "nValidHits/i");   // number of valid muon hits of muon 
    outTree->Branch("typeBits",   &typeBits,   "typeBits/i");     // number of valid muon hits of muon 
    
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

      Bool_t hasJSON = kFALSE;
      baconhep::RunLumiRangeMap rlrm;
      if(samp->jsonv[ifile].CompareTo("NONE")!=0) { 
        hasJSON = kTRUE;
        rlrm.addJSONFile(samp->jsonv[ifile].Data()); 
      }

      eventTree = (TTree*)infile->Get("Events");
      assert(eventTree);  
      
      
      Bool_t hasJet = eventTree->GetBranchStatus("AK4");
      eventTree->SetBranchAddress("Info",     &info);         TBranch *infoBr     = eventTree->GetBranch("Info");
      eventTree->SetBranchAddress("Muon",     &muonArr);      TBranch *muonBr     = eventTree->GetBranch("Muon");
      eventTree->SetBranchAddress("Electron", &electronArr);  TBranch *electronBr = eventTree->GetBranch("Electron");
      eventTree->SetBranchAddress("PV",       &vertexArr);    TBranch *vertexBr   = eventTree->GetBranch("PV");
      eventTree->SetBranchAddress("Photon",   &scArr);        TBranch *scBr       = eventTree->GetBranch("Photon");
      if(hasJet) eventTree->SetBranchAddress("AK4", &jetArr); TBranch *jetBr      = eventTree->GetBranch("AK4");
      
      TBranch *genBr=0, *genPartBr=0;
      Bool_t hasGen = (eventTree->GetBranchStatus("GenEvtInfo")&&!noGen);
      if(hasGen) {
        eventTree->SetBranchAddress("GenEvtInfo", &gen);        genBr     = eventTree->GetBranch("GenEvtInfo");
        eventTree->SetBranchAddress("GenParticle",&genPartArr); genPartBr = eventTree->GetBranch("GenParticle");
      }

      // Compute MC event weight per 1/fb
      const Double_t xsec = samp->xsecv[ifile];
      Double_t totalWeight=0;
      Double_t totalWeightUp=0;
      Double_t totalWeightDown=0;
      Double_t puWeight=1;
      Double_t puWeightUp=1;
      Double_t puWeightDown=1;

      cout << "n sections " << NSEC << endl;
      double frac = 1.0/NSEC;
      cout << "n sections " << NSEC << "  frac " << frac << endl;
      UInt_t IBEGIN = frac*ITH*eventTree->GetEntries();
      UInt_t IEND = frac*(ITH+1)*eventTree->GetEntries();
      cout << "start, end " << IBEGIN << " " << IEND << endl;
    //
    // loop over events
    //
    Double_t nsel=0, nselvar=0;
    for(UInt_t ientry=IBEGIN; ientry < IEND; ientry++) {
    // for(UInt_t ientry=0; ientry<eventTree->GetEntries(); ientry++) {
    // for(UInt_t ientry=0; ientry<(uint)(0.001*eventTree->GetEntries()); ientry++) {
        infoBr->GetEntry(ientry);
        
        int printIndex = (int)(eventTree->GetEntries()*0.01);
        if(ientry%printIndex==0) cout << "Processing event " << ientry << ". " << (int)(100*(ientry/(double)eventTree->GetEntries())) << " percent done with this file." << endl;
        
        Double_t weight=xsec;
        Double_t weightUp=xsec;
        Double_t weightDown=xsec;

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
        }else {
          hGenWeights->Fill(0.0,1.0);
        }
        scArr->Clear();
        scBr->GetEntry(ientry);

	    // veto w -> xv decays for signal and w -> mv for bacground samples (needed for inclusive WToLNu sample)
        if (isWrongFlavor && hasGen && fabs(toolbox::flavor(genPartArr, BOSON_ID))==LEPTON_ID) continue;
        else if (isSignal && hasGen && fabs(toolbox::flavor(genPartArr, BOSON_ID))!=LEPTON_ID) continue;
        // check for certified lumi (if applicable)
        baconhep::RunLumiRangeMap::RunLumiPairType rl(info->runNum, info->lumiSec);      
        if(hasJSON && !rlrm.hasRunLumi(rl)) continue;  
        // trigger requirement               
        if (!isMuonTrigger(triggerMenu, info->triggerBits,isData,is13TeV)) continue;
        // good vertex requirement
        if(!(info->hasGoodPV)) continue;
        
        // SELECTION PROCEDURE:
        //  (1) Look for 1 good muon matched to trigger
        //  (2) Reject event if another lepton is present passing looser cuts
        muonArr->Clear();
        muonBr->GetEntry(ientry);
        
        electronArr->Clear();
        electronBr->GetEntry(ientry);
      
        jetArr->Clear();
        if(hasJet)jetBr->GetEntry(ientry);

        // Int_t nLooseLep=0;
        const baconhep::TMuon *goodMuon=0;
        Bool_t passSel=kFALSE;
        // Bool_t passVeto=kTRUE;
    
        Int_t nLooseLep=0;
        TLorentzVector vEle(0,0,0,0);
        // Set up the electron veto...
        for(Int_t i=0; i<electronArr->GetEntriesFast(); i++) {
          const baconhep::TElectron *ele = (baconhep::TElectron*)((*electronArr)[i]);
          vEle.SetPtEtaPhiM(ele->pt, ele->eta, ele->phi, ELE_MASS);
          if((ele->r9 < 1.)){
            float eleAbsEta   = fabs(vEle.Eta());
            double eTregress =  ele->ecalEnergy/cosh(fabs(ele->eta));
            if(snamev[isam].Contains("data")){//Data
              int runNumber = is13TeV ? info->runNum : 306936;
              float eleScale = eleCorr.scaleCorr(runNumber, eTregress, eleAbsEta, ele->r9);
              (vEle) *= eleScale;
            } else {//MC
              float eleSmear = eleCorr.smearingSigma(info->runNum, eTregress, eleAbsEta, ele->r9, 12, 0., 0.);
              (vEle) *= 1. + eleSmear * gRandom->Gaus(0,1);
            }
          }
          if(fabs(vEle.Eta()) > VETO_ETA) continue; // loose lepton |eta| cut
          if(vEle.Pt()     < VETO_PT)  continue; // loose lepton pT cut
          if(passEleLooseID(ele,vEle, info->rhoIso)) nLooseLep++;
          if(nLooseLep>0) {  // extra lepton veto
            passSel=kFALSE;
            break;
          }
        }
        // if(!passVeto) continue;

        for(Int_t i=0; i<muonArr->GetEntriesFast(); i++) {
          const baconhep::TMuon *mu = (baconhep::TMuon*)((*muonArr)[i]);

          // apply scale and resolution corrections to MC
          Double_t mupt_corr = mu->pt;
          if(doScaleCorr && !snamev[isam].Contains("data"))
            mupt_corr = gRandom->Gaus(mu->pt*getMuScaleCorr(mu->eta,0),getMuResCorr(mu->eta,0));

          if(fabs(mu->eta) > VETO_ETA) continue; // loose lepton |eta| cut
          if(mupt_corr     < VETO_PT)  continue; // loose lepton pT cut
          if(passMuonLooseID(mu)) nLooseLep++;   // loose lepton selection
          if(nLooseLep>1) {  // extra lepton veto
            passSel=kFALSE;
            break;
          }
          
          if(fabs(mu->eta) > ETA_CUT)         continue;  // lepton |eta| cut
          if(mupt_corr     < PT_CUT)          continue;  // lepton pT cut   
          // std::cout << "veto7" << std::endl;
          if(!passMuonID(mu))                 continue;  // lepton selection
          if(!isMuonTriggerObj(triggerMenu, mu->hltMatchBits,isData,is13TeV)) continue;
          // std::cout << "veto8" << std::endl;

	  passSel=kTRUE;
	  goodMuon = mu;
	}



	if(passSel) {
	  /******** We have a W candidate! HURRAY! ********/
	  nsel+=weight;
    nselvar+=weight*weight;
        
    if(!isData){
      pfire.setObjects(scArr,jetArr);
      pfire.computePhotonsOnly(prefirePhoton, prefirePhotUp, prefirePhotDown);
      pfire.computeJetsOnly   (prefireJet   , prefireJetUp , prefireJetDown );
      pfire.computeFullPrefire(prefireWeight, prefireUp    , prefireDown    );
    }

    // apply scale and resolution corrections to MC
    Double_t goodMuonpt_corr = goodMuon->pt;
    if(doScaleCorr && !snamev[isam].Contains("data"))
    goodMuonpt_corr = gRandom->Gaus(goodMuon->pt*getMuScaleCorr(goodMuon->eta,0),getMuResCorr(goodMuon->eta,0));

	  TLorentzVector vLep; 
	  vLep.SetPtEtaPhiM(goodMuonpt_corr, goodMuon->eta, goodMuon->phi, MUON_MASS); 
	  

	  //
	  // Fill tree
	  //
	  runNum    = info->runNum;
	  lumiSec   = info->lumiSec;
	  evtNum    = info->evtNum;
	  
	  vertexArr->Clear();
	  vertexBr->GetEntry(ientry);

	  npv       = vertexArr->GetEntries();
	  npu	    = info->nPUmean;
	  genV      = new TLorentzVector(0,0,0,0);
	  genLep    = new TLorentzVector(0,0,0,0);
	  genNu    = new TLorentzVector(0,0,0,0);
	  genVPt    = -999;
	  genVPhi   = -999;
	  genVy     = -999;
	  genVMass  = -999;
	  genLepPt  = -999;
	  genLepPhi = -999;
	  genNuPt  = -999;
	  genNuPhi = -999;
	  u1        = -999;
	  u2        = -999;

    genMuonPt = 0;
    if(hasGen) genMuonPt = toolbox::getGenLep(genPartArr, vLep);

	  if(isRecoil && hasGen) {
      Int_t glepq1=-99;
      Int_t glepq2=-99;
	    TLorentzVector *gvec=new TLorentzVector(0,0,0,0);
      TLorentzVector *glep1=new TLorentzVector(0,0,0,0);
      TLorentzVector *glep2=new TLorentzVector(0,0,0,0);
	    toolbox::fillGen(genPartArr, BOSON_ID, gvec, glep1, glep2,&glepq1,&glepq2,1);
      if(snamev[isam].Contains("zxx")){ // DY only
          toolbox::fillGen(genPartArr, 23, gvec, glep1, glep2,&glepq1,&glepq2,1);
      }
   
      TLorentzVector tvec=*glep1+*glep2;
      genV=new TLorentzVector(0,0,0,0);
      genV->SetPtEtaPhiM(tvec.Pt(), tvec.Eta(), tvec.Phi(), tvec.M());
      genVPt   = tvec.Pt();
      genVPhi  = tvec.Phi();
      genVy    = tvec.Rapidity();
      genVMass = tvec.M();
      

      if (gvec && glep1) {
        genLep    = new TLorentzVector(0,0,0,0);
        if(toolbox::flavor(genPartArr, BOSON_ID)*glepq1<0){
          genLep->SetPtEtaPhiM(glep1->Pt(),glep1->Eta(),glep1->Phi(),glep1->M());
          genNu->SetPtEtaPhiM(glep2->Pt(),glep2->Eta(),glep2->Phi(),glep2->M());
        }
        if(toolbox::flavor(genPartArr, BOSON_ID)*glepq2<0){
          genLep->SetPtEtaPhiM(glep2->Pt(),glep2->Eta(),glep2->Phi(),glep2->M());
          genNu->SetPtEtaPhiM(glep1->Pt(),glep1->Eta(),glep1->Phi(),glep1->M());
        }
        genLepPt  = genLep->Pt();
        genLepPhi = genLep->Phi();
        
        genNuPt  = genNu->Pt();
        genNuPhi = genNu->Phi();

  
        TVector2 vWPt((genVPt)*cos(genVPhi),(genVPt)*sin(genVPhi));
        TVector2 vLepPt(vLep.Px(),vLep.Py());

        TVector2 vMet((info->pfMETC)*cos(info->pfMETCphi), (info->pfMETC)*sin(info->pfMETCphi));
        TVector2 vU = -1.0*(vMet+vLepPt);
        u1 = ((vWPt.Px())*(vU.Px()) + (vWPt.Py())*(vU.Py()))/(genVPt);  // u1 = (pT . u)/|pT|
        u2 = ((vWPt.Px())*(vU.Py()) - (vWPt.Py())*(vU.Px()))/(genVPt);  // u2 = (pT x u)/|pT|

        TVector2 vPuppiMet((info->puppET)*cos(info->puppETphi), (info->puppET)*sin(info->puppETphi));
        TVector2 vPuppiU = -1.0*(vPuppiMet+vLepPt);
        puppiU1 = ((vWPt.Px())*(vPuppiU.Px()) + (vWPt.Py())*(vPuppiU.Py()))/(genVPt);  // u1 = (pT . u)/|pT|
        puppiU2 = ((vWPt.Px())*(vPuppiU.Py()) - (vWPt.Py())*(vPuppiU.Px()))/(genVPt);  // u2 = (pT x u)/|pT|
  
      }

      // Clean up
      delete gvec;
      delete glep1;
      delete glep2;
      gvec=0; glep1=0; glep2=0;
	  }
	  scale1fb = weight;
    scale1fbUp = weightUp;
    scale1fbDown = weightDown;
    met	   = info->pfMETC;
	  metPhi   = info->pfMETCphi;
	  mt       = sqrt( 2.0 * (vLep.Pt()) * (info->pfMETC) * (1.0-cos(toolbox::deltaPhi(vLep.Phi(),info->pfMETCphi))) );
    puppiMet = info->puppET;
    puppiMetPhi = info->puppETphi;
	  puppiMt     = sqrt( 2.0 * (vLep.Pt()) * (info->puppET) * (1.0-cos(toolbox::deltaPhi(vLep.Phi(),info->puppETphi))) );
	  q        = goodMuon->q;
	  lep      = &vLep;
	  
	  ///// muon specific /////
	  trkIso     = goodMuon->trkIso;
	  emIso      = goodMuon->ecalIso;
	  hadIso     = goodMuon->hcalIso;
	  pfChIso    = goodMuon->chHadIso;
	  pfGamIso   = goodMuon->gammaIso;
	  pfNeuIso   = goodMuon->neuHadIso;
	  pfCombIso  = goodMuon->chHadIso + TMath::Max(goodMuon->neuHadIso + goodMuon->gammaIso -
						   0.5*(goodMuon->puIso),Double_t(0));
	  d0         = goodMuon->d0;
	  dz         = goodMuon->dz;
	  muNchi2    = goodMuon->muNchi2;
	  nPixHits   = goodMuon->nPixHits;
	  nTkLayers  = goodMuon->nTkLayers;
	  nMatch     = goodMuon->nMatchStn;
	  nValidHits = goodMuon->nValidHits;
	  typeBits   = goodMuon->typeBits;
	  outTree->Fill();
	  delete genV;
	  delete genLep;
	  genV=0, genLep=0, lep=0;
          // reset everything to 1
      prefirePhoton=1; prefirePhotUp=1; prefirePhotDown=1;
      prefireJet   =1; prefireJetUp =1; prefireJetDown =1;
      prefireWeight=1; prefireUp    =1; prefireDown    =1;
        }
      }
      delete infile;
      infile=0, eventTree=0;    

      cout << nsel  << " +/- " << sqrt(nselvar);
      if(isam!=0) cout << " per 1/pb";
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
  cout << " W -> mu nu" << endl;
  cout << "  pT > " << PT_CUT << endl;
  cout << "  |eta| < " << ETA_CUT << endl;
  cout << endl;
  
  cout << endl;
  cout << "  <> Output saved in " << outputDir << "/" << endl;    
  cout << endl;  
      
  gBenchmark->Show("selectWm"); 
}

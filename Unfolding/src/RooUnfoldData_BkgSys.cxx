#if !defined(__CINT__) || defined(__MAKECINT__)
#include <iostream>
#include <fstream>
#include <sstream>
using std::cout;
using std::endl;

#include "TRandom.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TVectorD.h"
#include "TGraphErrors.h"
#include "TGraphAsymmErrors.h"
#include "TROOT.h"


#include "RooUnfoldResponse.h"
#include "RooUnfoldBayes.h"

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

#endif


//Include Headers
#include "/afs/cern.ch/user/x/xniu/WZXSection/Copy/CMSSW_7_6_3_patch2/src/BootStrap/interface/BootStrap.hpp"

#ifdef __CINT__
gSystem->Load("/afs/cern.ch/user/x/xniu/WZXSection/Copy/CMSSW_7_6_3_patch2/src/BootStrap/bin/libBootStrap.so")
#endif

string int2string(int i) {
  stringstream ss;
  string ret;
  ss << i;
  ss >> ret;
  return ret;
}

//==============================================================================
// Global definitions
//==============================================================================

const Double_t cutdummy= -99999.0;


TH1D *hReco, *hTruth, *hData, *hTop, *hEWK, *hMeas, *hUnfold;
TH2D *hMatrix, *hMatrix_hilf;

TGraphAsymmErrors *gUnfold;

//==============================================================================
// Train unfolding algorithm
//==============================================================================

Int_t Train (const Int_t uncert, const TString variable, const TString unfoldinputFile)
{
  cout <<"================ TRAIN ======================="<<endl;
  TFile *file1 = new TFile(unfoldinputFile);
  if (file1 == NULL) cout <<"File does not exists"<<endl;
  hTruth = (TH1D*)file1->Get("h"+variable+"Truth");
  hReco = (TH1D*)file1->Get("h"+variable+"Reco");
  hMatrix_hilf = (TH2D*)file1->Get("h"+variable+"Matrix");

  if (hTruth==NULL) cout<< "hTruth does not exist"<<endl;
  if (hReco==NULL) cout<< "hReco does not exist"<<endl;
  if (hMatrix_hilf==NULL) cout<< "hMatrix does not exist"<<endl;

  hMatrix= (TH2D*)hMatrix_hilf->Clone("hMatrix");
  for(int j=0;j!=hMatrix_hilf->GetNbinsX();++j)
    {
      for(int k=0;k!=hMatrix_hilf->GetNbinsY();++k)
	{
	  hMatrix->SetBinContent(j+1,k+1,hMatrix_hilf->GetBinContent(k+1,j+1));
	  hMatrix->SetBinError(j+1,k+1,hMatrix_hilf->GetBinError(k+1,j+1));
	}
    }


  cout<<"Training finished"<<endl;
  return 1;
}

//==============================================================================
// Test distribution
//==============================================================================

Int_t Test (const Int_t uncert, const TString variable, const TString  bkgtype, const Double_t bkgunc, const TString sigextinputFile)
{
  cout <<"==================== TEST ===================="<<endl;
  TFile *f=new TFile(sigextinputFile);
  
  if (f == NULL) cout<<"file does not exists"<<endl;
  hData = (TH1D*)f->Get("hData"+variable);
  hTop = (TH1D*)f->Get("hTop"+variable);
  hEWK = (TH1D*)f->Get("hEWK"+variable);
  
  hMeas = (TH1D*)f->Get("hData"+variable);
  hMeas->Sumw2();

  double topunc, ewkunc = 0.;
  if(bkgtype == "EWK") ewkunc = bkgunc;
  if(bkgtype == "Top") topunc = bkgunc;
  if(uncert==1)
    {
      hMeas->Add(hTop,-(1.+topunc));
      hMeas->Add(hEWK,-(1.+ewkunc));
    }
  else if(uncert==-1)
    {
      hMeas->Add(hTop,-(1.-topunc));
      hMeas->Add(hEWK,-(1.-ewkunc));
    }


  cout<<"Test finished"<<endl;
  return 1;
}

//==============================================================================
// Example Unfolding
//==============================================================================

void RooUnfoldExample(const TString  unfoldinputFile,
                      const TString  sigextinputFile,
                      const TString  outputDir,    // output directory
		      const TString  variable,
		      const TString  bkgtype,
		      const Double_t bkgunc,
		      const Double_t lumi,         // integrated luminosity (/pb
		      const Int_t uncert,          // 1: up, 2:down
		      const Int_t iterations)
{
#ifdef __CINT__
  gSystem->Load("libRooUnfold");
#endif
 

  TH1::AddDirectory(kFALSE);

  if(!mkdir(outputDir,0755))
    cout<<"Created output directory"<<endl;

  double LUMI=lumi;
 
  Train(uncert,variable,unfoldinputFile);
  Test(uncert,variable,bkgtype,bkgunc,sigextinputFile);
  
  cout << "==================================== UNFOLD ===================================" << endl;

  TFile* his=new TFile();

  if (uncert==1)his=new TFile(outputDir+(string("/UnfoldingOutput"))+variable+bkgtype+(string("BkgUp.root")).c_str(), "recreate");
  if (uncert==-1)his=new TFile(outputDir+(string("/UnfoldingOutput"))+variable+bkgtype+(string("BkgDown.root")).c_str(), "recreate");

  gRandom->SetSeed(1234);

  RooUnfoldResponse response(hReco,hTruth,hMatrix);

  response.UseOverflow();


  RooUnfoldBayes   unfold (&response, hMeas, iterations);

  BootStrap b;
  b.SetUnfoldType(BootStrap::kBayes);
  b.SetRegParam(iterations);
  
  b.SetUMatrix(hReco,hTruth,hMatrix);
  b.SetData( (TH1D*)hMeas->Clone("bootstrap_data") );
  b.SetToyType(BootStrap::kBootstrap);
  b.run();

  TGraphAsymmErrors *g = b.result(BootStrap::kMin,.68);
  g->SetName("gBootStrap");

  unfold.SetNToys(1000);
  
  hUnfold=(TH1D*) unfold.Hreco(RooUnfold::kCovToy);
  
  for(int j=0;j!=hUnfold->GetNbinsX();++j)
    {
      hUnfold->SetBinContent(j+1,hUnfold->GetBinContent(j+1)/hUnfold->GetBinWidth(j+1));
      hUnfold->SetBinError(j+1,hUnfold->GetBinError(j+1)/hUnfold->GetBinWidth(j+1));
    }

  for(int j=0;j!=hTruth->GetNbinsX();++j)
    {
      hTruth->SetBinContent(j+1,hTruth->GetBinContent(j+1)/hTruth->GetBinWidth(j+1));
      hTruth->SetBinError(j+1,hTruth->GetBinError(j+1)/hTruth->GetBinWidth(j+1));
    }

  hUnfold->Scale(1./LUMI);
  hTruth->Scale(1./LUMI);

  his->cd();
  hTruth->Write("hTruth");
  hUnfold->Write("hUnfold");
  g->Write();
  his->Close();
}

#ifndef __CINT__
int main (int argc,char **argv) {RooUnfoldExample(argv[1],argv[2],argv[3],argv[4],argv[5],atof(argv[6]),atof(argv[7]),atof(argv[8]),atof(argv[9])); return 0; }  // Main program when run stand-alone
#endif

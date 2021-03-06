#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <math.h>
#include <fstream>

#include "TH1D.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TGraph.h"
#include "TFile.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TMinuit.h"
 
// partial directory names for each of the options
TString inDir = "/afs/cern.ch/user/s/sabrandt/work/public/FilesSM2017GH/Acceptance/BLAH2/";

// TString sigDirFSR="_aMCxPythia_v2_aMCxPythia_v2/";// change this once i finish all my shit
// TString sigDirMC="_aMCxPythia_v2_aMCxPythia_v2/";
// TString bkgDir="_aMCxPythia_v2_aMCxPythia_v2/";// should be exp vs Powerlaw

TString outDir = "/afs/cern.ch/user/s/sabrandt/work/public/FilesSM2017GH/Acceptance/BLAH2/";
int NQCD=6;
int NPDF=100;

// function to do some division

// function to compute QCD uncertainty
double uncQCD(vector<double> accV);
// function to compute PDF uncertainty
double uncPDF(vector<double> accV);
// function to write the uncertainty tables
void writeUncTable(TString filename, vector<double> e, vector<double> m);
void writeMegaUncTable(TString filename, vector<double> e13, vector<double> m13, vector<double> e5, vector<double> m5, vector<double> e135, vector<double> m135);

void writeAccTable(TString filename, vector<double>& vAcc13, vector<double>& vAcc13D);

vector<double> divide(vector<double> n, vector<double> d);

void readFile(TString filename, vector<double> &vec);

void fillAccVec(vector<double> &main, vector<double> v1, vector<double> v2, vector<double> v3, vector<double> v4, vector<double> v5, vector<double> v6, vector<double> v7, vector<double> v8);
void fillUncVec(vector<double> &main, vector<double> v1, vector<double> v2, vector<double> v3, vector<double> v4, vector<double> v5, vector<double> v6, vector<double> v7, vector<double> v8);

void calcUnc(){
  // This is horrible, but too bad.
  vector<double> vWep13, vWem13, vWe13, vZee13;
  vector<double> vWep5, vWem5, vWe5, vZee5;
  vector<double> vWmp13, vWmm13, vWm13, vZmm13;
  vector<double> vWmp5, vWmm5, vWm5, vZmm5;
  vector<double> vWep13D, vWem13D, vWe13D, vZee13D;
  vector<double> vWep5D, vWem5D, vWe5D, vZee5D;
  vector<double> vWmp13D, vWmm13D, vWm13D, vZmm13D;
  vector<double> vWmp5D, vWmm5D, vWm5D, vZmm5D;

  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("WepGen5TeV_Undressed/qcd_vars.txt",vWep5);
  readFile("WemGen5TeV_Undressed/qcd_vars.txt",vWem5);
  readFile("WeGen5TeV_Undressed/qcd_vars.txt",vWe5);
  readFile("WmmGen5TeV_Undressed/qcd_vars.txt",vWmm5);
  readFile("WmpGen5TeV_Undressed/qcd_vars.txt",vWmp5);
  readFile("WmGen5TeV_Undressed/qcd_vars.txt",vWm5);
  
  std::cout << "load 5 TeV dress " << std::endl;
  readFile("WemGen5TeV_Dressed/qcd_vars.txt",vWem5D);
  readFile("WepGen5TeV_Dressed/qcd_vars.txt",vWep5D);
  readFile("WeGen5TeV_Dressed/qcd_vars.txt",vWe5D);
  readFile("WmmGen5TeV_Dressed/qcd_vars.txt",vWmm5D);
  readFile("WmpGen5TeV_Dressed/qcd_vars.txt",vWmp5D);
  readFile("WmGen5TeV_Dressed/qcd_vars.txt",vWm5D);
  
  std::cout << "load 5 TeV z u " << std::endl;
  readFile("ZeeGen5TeV_Undressed/qcd_vars.txt",vZee5);
  readFile("ZmmGen5TeV_Undressed/qcd_vars.txt",vZmm5);
  
  std::cout << "load 5 TeV z d " << std::endl;
  readFile("ZeeGen5TeV_Dressed/qcd_vars.txt",vZee5D);
  readFile("ZmmGen5TeV_Dressed/qcd_vars.txt",vZmm5D);
  
  std::cout << "load 13 w u " << std::endl;
  readFile("WepGen13TeV_Undressed/qcd_vars.txt",vWep13);
  readFile("WemGen13TeV_Undressed/qcd_vars.txt",vWem13);
  readFile("WeGen13TeV_Undressed/qcd_vars.txt",vWe13);
  readFile("WmmGen13TeV_Undressed/qcd_vars.txt",vWmm13);
  readFile("WmpGen13TeV_Undressed/qcd_vars.txt",vWmp13);
  readFile("WmGen13TeV_Undressed/qcd_vars.txt",vWm13);
  
  std::cout << "load 13 w d " << std::endl;
  readFile("WemGen13TeV_Dressed/qcd_vars.txt",vWem13D);
  readFile("WepGen13TeV_Dressed/qcd_vars.txt",vWep13D);
  readFile("WeGen13TeV_Dressed/qcd_vars.txt",vWe13D);
  readFile("WmmGen13TeV_Dressed/qcd_vars.txt",vWmm13D);
  readFile("WmpGen13TeV_Dressed/qcd_vars.txt",vWmp13D);
  readFile("WmGen13TeV_Dressed/qcd_vars.txt",vWm13D);
  
  std::cout << "load 13 z u " << std::endl;
  readFile("ZeeGen13TeV_Undressed/qcd_vars.txt",vZee13);
  readFile("ZmmGen13TeV_Undressed/qcd_vars.txt",vZmm13);
  
  std::cout << "load 13 z d " << std::endl;
  readFile("ZeeGen13TeV_Dressed/qcd_vars.txt",vZee13D);
  readFile("ZmmGen13TeV_Dressed/qcd_vars.txt",vZmm13D);


  std::cout << "load 5 w u pdf " << std::endl;
  readFile("WepGen5TeV_Undressed/pdf_vars.txt",vWep5);
  readFile("WemGen5TeV_Undressed/pdf_vars.txt",vWem5);
  readFile("WeGen5TeV_Undressed/pdf_vars.txt",vWe5);
  readFile("WmmGen5TeV_Undressed/pdf_vars.txt",vWmm5);
  readFile("WmpGen5TeV_Undressed/pdf_vars.txt",vWmp5);
  readFile("WmGen5TeV_Undressed/pdf_vars.txt",vWm5);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("WemGen5TeV_Dressed/pdf_vars.txt",vWem5D);
  readFile("WepGen5TeV_Dressed/pdf_vars.txt",vWep5D);
  readFile("WeGen5TeV_Dressed/pdf_vars.txt",vWe5D);
  readFile("WmmGen5TeV_Dressed/pdf_vars.txt",vWmm5D);
  readFile("WmpGen5TeV_Dressed/pdf_vars.txt",vWmp5D);
  readFile("WmGen5TeV_Dressed/pdf_vars.txt",vWm5D);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("ZeeGen5TeV_Undressed/pdf_vars.txt",vZee5);
  readFile("ZmmGen5TeV_Undressed/pdf_vars.txt",vZmm5);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("ZeeGen5TeV_Dressed/pdf_vars.txt",vZee5D);
  readFile("ZmmGen5TeV_Dressed/pdf_vars.txt",vZmm5D);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("WepGen13TeV_Undressed/pdf_vars.txt",vWep13);
  readFile("WemGen13TeV_Undressed/pdf_vars.txt",vWem13);
  readFile("WeGen13TeV_Undressed/pdf_vars.txt",vWe13);
  readFile("WmmGen13TeV_Undressed/pdf_vars.txt",vWmm13);
  readFile("WmpGen13TeV_Undressed/pdf_vars.txt",vWmp13);
  readFile("WmGen13TeV_Undressed/pdf_vars.txt",vWm13);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("WemGen13TeV_Dressed/pdf_vars.txt",vWem13D);
  readFile("WepGen13TeV_Dressed/pdf_vars.txt",vWep13D);
  readFile("WeGen13TeV_Dressed/pdf_vars.txt",vWe13D);
  readFile("WmmGen13TeV_Dressed/pdf_vars.txt",vWmm13D);
  readFile("WmpGen13TeV_Dressed/pdf_vars.txt",vWmp13D);
  readFile("WmGen13TeV_Dressed/pdf_vars.txt",vWm13D);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("ZeeGen13TeV_Undressed/pdf_vars.txt",vZee13);
  readFile("ZmmGen13TeV_Undressed/pdf_vars.txt",vZmm13);
  
  std::cout << "load 5 TeV undressed " << std::endl;
  readFile("ZeeGen13TeV_Dressed/pdf_vars.txt",vZee13D);
  readFile("ZmmGen13TeV_Dressed/pdf_vars.txt",vZmm13D);

  std::cout << "ratio 5" << std::endl;
  vector<double> vWepWem5 = divide(vWep5,vWem5);
  vector<double> vWepZee5 = divide(vWep5,vZee5);
  vector<double> vWemZee5 = divide(vWem5,vZee5);
  vector<double> vWeZee5  = divide(vWe5 ,vZee5);
  std::cout << "ratio 13" << std::endl;
  vector<double> vWepWem13 = divide(vWep13,vWem13);
  vector<double> vWepZee13 = divide(vWep13,vZee13);
  vector<double> vWemZee13 = divide(vWem13,vZee13);
  vector<double> vWeZee13  = divide(vWe13 ,vZee13);
  std::cout << "ratio 13 to 5" << std::endl;
  vector<double> vWepWem135 = divide(vWepWem13,vWepWem5);
  vector<double> vWepZee135 = divide(vWepZee13,vWepZee5);
  vector<double> vWemZee135 = divide(vWemZee13,vWemZee5);
  vector<double> vWeZee135  = divide(vWeZee13 ,vWeZee5);
  vector<double> vWep135   = divide(vWep13 ,vWep5);
  vector<double> vWem135   = divide(vWem13 ,vWem5);
  vector<double> vWe135    = divide(vWe13  ,vWe5 );
  vector<double> vZee135   = divide(vZee13 ,vZee5);
  
  std::cout << "ratio 5" << std::endl;
  vector<double> vWmpWmm5 = divide(vWmp5,vWmm5);
  std::cout << "ratio a" << std::endl;
  vector<double> vWmpZmm5 = divide(vWmp5,vZmm5);
  std::cout << "ratio b" << std::endl;
  vector<double> vWmmZmm5 = divide(vWmm5,vZmm5);
  std::cout << "ratio c" << std::endl;
  vector<double> vWmZmm5  = divide(vWm5 ,vZmm5);
  
  std::cout << "ratio 13" << std::endl;
  vector<double> vWmpWmm13 = divide(vWmp13,vWmm13);
  vector<double> vWmpZmm13 = divide(vWmp13,vZmm13);
  vector<double> vWmmZmm13 = divide(vWmm13,vZmm13);
  vector<double> vWmZmm13  = divide(vWm13 ,vZmm13);
  
  std::cout << "ratio 13 to 5" << std::endl;
  vector<double> vWmpWmm135 = divide(vWmpWmm13,vWmpWmm5);
  vector<double> vWmpZmm135 = divide(vWmpZmm13,vWmpZmm5);
  vector<double> vWmmZmm135 = divide(vWmmZmm13,vWmmZmm5);
  vector<double> vWmZmm135  = divide(vWmZmm13 ,vWmZmm5);
  vector<double> vWmp135   = divide(vWmp13 ,vWmp5);
  vector<double> vWmm135   = divide(vWmm13 ,vWmm5);
  vector<double> vWm135    = divide(vWm13  ,vWm5 );
  vector<double> vZmm135   = divide(vZmm13 ,vZmm5);
  
  vector<double> vAccMu13, vAccEle13;//, vUncEle13;
  vector<double> vAccMu5, vAccEle5;//, vUncEle5PDF, vUncEle5QCD;
  // vector<double> vUncEle135PDF, vUncEle135QCD;
  vector<double> vUncEle13, vUncMu13, vUncEle5, vUncMu5, vUncEle135, vUncMu135;
  // vector<double> vUncMu13PDF, vUncMu13QCD, vUncMu5PDF, vUncMu5QCD;
  
  std::cout << "electrons 13" << std::endl;
  fillUncVec(vUncEle13, vWep13, vWem13, vWe13, vZee13, vWepWem13, vWepZee13, vWemZee13, vWeZee13);
  std::cout << "electrons 5" << std::endl;
  fillUncVec(vUncEle5, vWep5, vWem5, vWe5, vZee5, vWepWem5, vWepZee5, vWemZee5, vWeZee5);
  std::cout << "electrons 13/5" << std::endl;
  fillUncVec(vUncEle135, vWep135, vWem135, vWe135, vZee135, vWepWem135, vWepZee135, vWemZee135, vWeZee135);
  
  fillUncVec(vUncMu13, vWmp13, vWmm13, vWm13, vZmm13, vWmpWmm13, vWmpZmm13, vWmmZmm13, vWmZmm13);
 
  fillUncVec(vUncMu5, vWmp5, vWmm5, vWm5, vZmm5, vWmpWmm5, vWmpZmm5, vWmmZmm5, vWmZmm5);
  
  fillUncVec(vUncMu135, vWmp135, vWmm135, vWm135, vZmm135, vWmpWmm135, vWmpZmm135, vWmmZmm135, vWmZmm135);


  fillAccVec(vAccEle13, vWep13, vWem13, vWe13, vZee13, vWep13D, vWem13D, vWe13D, vZee13D);
  fillAccVec(vAccMu13,  vWmp13, vWmm13, vWm13, vZmm13, vWmp13D, vWmm13D, vWm13D, vZmm13D);
  fillAccVec(vAccEle5,  vWep5, vWem5, vWe5, vZee5, vWep5D, vWem5D, vWe5D, vZee5D);
  fillAccVec(vAccMu5,   vWmp5, vWmm5, vWm5, vZmm5, vWmp5D, vWmm5D, vWm5D, vZmm5D);

  // vUncEle5.push_back(uncPDF(vWep5));
  // vUncEle5.push_back(uncQCD(vWep5));
  
  writeAccTable("TEST_13TeV_Acc.txt",vAccEle13,vAccMu13);
  writeAccTable("TEST_5TeV_Acc.txt",vAccEle5,vAccMu5);
  
  // write each of the uncertainty tables
  writeUncTable("TEST_5TeV.txt",vUncEle5,vUncMu5);
  writeUncTable("TEST_13TeV.txt",vUncEle13,vUncMu13);
  writeUncTable("TEST_13to5TeV.txt",vUncEle135,vUncMu135);
  writeMegaUncTable("TEST_Mega.txt",vUncEle13,vUncMu13,vUncEle5,vUncMu5,vUncEle135,vUncMu135);

}

vector<double> divide(vector<double> n, vector<double> d){
  vector<double> r;
  std::cout << "------------" << std::endl;
  for(uint i=0;i < n.size();++i){
    r.push_back(n[i]/d[i]);
    if(i<7)std::cout << n[i]<< "  " << d[i] << "  " << r[i] << std::endl;
  }
  return r;
}

void readFile(TString filename, vector<double> &vec){
  double value=0;
  string line;
  char infilename[250];
  sprintf(infilename,"%s/%s",inDir.Data(),filename.Data());
  ifstream myfile(infilename);
  assert(myfile);
  while(myfile>>value){
    vec.push_back(value);
    std::cout << value << std::endl;
    std::cout << vec.back() << std::endl;
  }
}


double uncQCD(vector<double> accv){
  double accUncQCD = 0;
  for(int i=0;i < NQCD+1;i++){
    if(fabs(accv[i]-accv[0])/(accv[0]) > accUncQCD) accUncQCD = fabs(accv[i]-accv[0])/(accv[0]);
    std::cout << 100*(accv[i]-accv[0])/(accv[0]) << "  "<<std::endl;
  }
  // std::cout << 100*accUncQCD << std::endl;
  return 100*accUncQCD;
}

double uncPDF(vector<double> accv){
  double accUncPDF = 0;
  // std::cout << accv.size() << std::endl;
  for(int i=NQCD+1;i < NPDF+NQCD+1+1;i++){
    accUncPDF+=(accv[i]-accv[0])*(accv[i]-accv[0])/(NPDF*accv[0]*accv[0]);
    std::cout << 100*(accv[i]-accv[0])/accv[0] << " "<< std::endl;//"  " << accv[i] << " " <<  accv[0] << "  " << accUncPDF << std::endl;
  }
  // std::cout << 100*accUncPDF << std::endl;
  return 100*sqrt(accUncPDF);
}

void fillAccVec(vector<double> &main, vector<double> v1, vector<double> v2, vector<double> v3, vector<double> v4, vector<double> v5, vector<double> v6, vector<double> v7, vector<double> v8){
  main.push_back(v1[0]);
  main.push_back(v2[0]);
  main.push_back(v3[0]);
  main.push_back(v4[0]);
  main.push_back(v5[0]);
  main.push_back(v6[0]);
  main.push_back(v7[0]);
  main.push_back(v8[0]);
  return;
}


void fillUncVec(vector<double> &main, vector<double> v1, vector<double> v2, vector<double> v3, vector<double> v4, vector<double> v5, vector<double> v6, vector<double> v7, vector<double> v8){
  // std::cout << "wp pdf" << std::endl;
  main.push_back(uncPDF(v1));
  // std::cout << "wm pdf" << std::endl;
  main.push_back(uncPDF(v2));
  // std::cout << "w pdf" << std::endl;
  main.push_back(uncPDF(v3));
  // std::cout << "z pdf" << std::endl;
  main.push_back(uncPDF(v4));
  // std::cout << "w+/w- pdf" << std::endl;
  main.push_back(uncPDF(v5));
  // std::cout << "w+/z pdf" << std::endl;
  main.push_back(uncPDF(v6));
  // std::cout << "w-/z pdf" << std::endl;
  main.push_back(uncPDF(v7));
  // std::cout << "w/z pdf" << std::endl;
  main.push_back(uncPDF(v8));
  std::cout << "\n";
  
  // std::cout << "wp qcd" << std::endl;
  main.push_back(uncQCD(v1));
  // // std::cout << "wm qcd" << std::endl;
  main.push_back(uncQCD(v2));
  // std::cout << "w qcd" << std::endl;
  main.push_back(uncQCD(v3));
  // std::cout << "z qcd" << std::endl;
  main.push_back(uncQCD(v4));
  // std::cout << "w+/w- qcd" << std::endl;
  main.push_back(uncQCD(v5));
  // std::cout << "w+/z qcd" << std::endl;
  main.push_back(uncQCD(v6));
  // std::cout << "w-/z qcd" << std::endl;
  main.push_back(uncQCD(v7));
  // std::cout << "w/z qcd" << std::endl;
  main.push_back(uncQCD(v8));
  
  std::cout << "\n";
  
  return;
}

void writeAccTable(TString filename, vector<double>& e, vector<double>& m){
  char txtfname[250];
  sprintf(txtfname,"%s/%s.tex",outDir.Data(),filename.Data());
  // ofstream txtfile;
  // txtfile.open(txtfname);
  
  FILE *txtfile;
  txtfile = fopen(txtfname,"w");
  fprintf(txtfile,"\\begin{center}\n");
  fprintf(txtfile,"\\scalebox{0.8}{\n");
  fprintf(txtfile,"\\begin{tabular}{|c|c|c|c|c|}\n");
  fprintf(txtfile,"\\hline\n");
  fprintf(txtfile,"Process & $A_{Gen}(\\mathrm{Post-FSR})$ & $A_{Gen}(\\mathrm{Dressed}$) \\\\");
  fprintf(txtfile,"\\hline \\hline\n");
  fprintf(txtfile,"$W\\rightarrow e^+\\nu$     & %.3f & %.3f \\\\\n",e[0],  e[4]);
  fprintf(txtfile,"$W\\rightarrow e^-\\nu$     & %.3f & %.3f \\\\\n",e[1],  e[5]);
  fprintf(txtfile,"$W\\rightarrow e\\nu$       & %.3f & %.3f \\\\\n",e[2],  e[6]);
  fprintf(txtfile,"$Z\\rightarrow ee$          & %.3f & %.3f \\\\\n",e[3],  e[7]);
  fprintf(txtfile,"\\hline\n" );
  fprintf(txtfile,"$W\\rightarrow \\mu^+\\nu$   & %.3f & %.3f \\\\\n",m[0],  m[4]);
  fprintf(txtfile,"$W\\rightarrow \\mu^-\\nu$   & %.3f & %.3f \\\\\n",m[1],  m[5]);
  fprintf(txtfile,"$W\\rightarrow \\mu\\nu$     & %.3f & %.3f \\\\\n",m[2],  m[6]);
  fprintf(txtfile,"$Z\\rightarrow \\mu\\mu$     & %.3f & %.3f \\\\\n",m[3],  m[7]);
  fprintf(txtfile,"\\hline\n" );
  fprintf(txtfile,"\\end{tabular} }\n" );
  fprintf(txtfile,"\\end{center}\n" );
  // fprintf(txtfile,"*" );
  fclose(txtfile);
  // txtfile.close();
}

void writeUncTable(TString filename, vector<double> e, vector<double> m){
  char txtfname[250];
  sprintf(txtfname,"%s/%s.tex",outDir.Data(),filename.Data());
  // ofstream txtfile;
  // txtfile.open(txtfname);
  FILE *txtfile;
  txtfile = fopen(txtfname,"w");
  fprintf(txtfile,"\\begin{center}\n");
  fprintf(txtfile,"\\scalebox{0.8}{\n");
  fprintf(txtfile,"\\begin{tabular}{|c|c|c|c|c|}\n");
  fprintf(txtfile,"\\hline\n");
  fprintf(txtfile,"Process &\\multicolumn{2}{|c|}{QCD[\\%]}&\\multicolumn{2}{|c|}{PDF[\\%]}\\\\\\cline{2-5}\n");
  fprintf(txtfile,"( TeV) & Ele & Mu & Ele & Mu \\\\\n");
  fprintf(txtfile,"\\hline \\hline\n");
  fprintf(txtfile,"$W^+$     & %.3f & %.3f & %.3f & %.3f\\\\\n",e[8],  m[8],  e[0],m[0] );
  fprintf(txtfile,"$W^-$     & %.3f & %.3f & %.3f & %.3f\\\\\n",e[9],  m[9],  e[1],m[1] );
  fprintf(txtfile,"$W$       & %.3f & %.3f & %.3f & %.3f\\\\\n",e[10], m[10], e[2],m[2] );
  fprintf(txtfile,"$Z$       & %.3f & %.3f & %.3f & %.3f\\\\\n",e[11], m[11], e[3],m[3] );
  fprintf(txtfile,"$W^+/W^-$ & %.3f & %.3f & %.3f & %.3f\\\\\n",e[12], m[12], e[4],m[4] );
  fprintf(txtfile,"$W^+/Z$   & %.3f & %.3f & %.3f & %.3f\\\\\n",e[13], m[13], e[5],m[5] );
  fprintf(txtfile,"$W^-/Z$   & %.3f & %.3f & %.3f & %.3f\\\\\n",e[14], m[14], e[6],m[6] );
  fprintf(txtfile,"$W/Z$     & %.3f & %.3f & %.3f & %.3f\\\\\n",e[15], m[15], e[7],m[7] );
  fprintf(txtfile,"\\hline\n" );
  fprintf(txtfile,"\\end{tabular} }\n" );
  fprintf(txtfile,"\\end{center}\n" );
  // fprintf(txtfile,"*" );
  fclose(txtfile);
  // txtfile.close();
}

void writeMegaUncTable(TString filename, vector<double> e13, vector<double> m13, vector<double> e5, vector<double> m5, vector<double> e135, vector<double> m135){
  char txtfname[250];
  sprintf(txtfname,"%s/%s.tex",outDir.Data(),filename.Data());
  // ofstream txtfile;
  // txtfile.open(txtfname);
  FILE *txtfile;
  txtfile = fopen(txtfname,"w");
  fprintf(txtfile,"\\begin{center}\n");
  fprintf(txtfile,"\\scalebox{0.8}{\n");
  fprintf(txtfile,"\\begin{tabular}{|c||c|c|c|c||c|c|c|c|}\n");
  fprintf(txtfile,"\\hline\n");
  fprintf(txtfile," &\\multicolumn{4}{|c|}{13 TeV}&\\multicolumn{4}{|c|}{5 TeV}\\\\\\cline{2-9}\n");
  fprintf(txtfile,"Process &\\multicolumn{2}{|c|}{QCD[\\%]}&\\multicolumn{2}{|c||}{PDF[\\%]}&\\multicolumn{2}{|c|}{QCD[\\%]}&\\multicolumn{2}{|c|}{PDF[\\%]}\\\\\\cline{2-9}\n");
  fprintf(txtfile,"  & Ele & Mu & Ele & Mu & Ele & Mu & Ele & Mu \\\\\n");
  fprintf(txtfile,"\\hline \\hline\n");
  fprintf(txtfile,"$W^+$     & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[8],  m13[8],  e13[0],m13[0],e5[8],  m5[8],  e5[0],m5[0] );
  fprintf(txtfile,"$W^-$     & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[9],  m13[9],  e13[1],m13[1],e5[9],  m5[9],  e5[1],m5[1] );
  fprintf(txtfile,"$W$       & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[10], m13[10], e13[2],m13[2],e5[10], m5[10], e5[2],m5[2] );
  fprintf(txtfile,"$Z$       & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[11], m13[11], e13[3],m13[3],e5[11], m5[11], e5[3],m5[3] );
  fprintf(txtfile,"$W^+/W^-$ & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[12], m13[12], e13[4],m13[4],e5[12], m5[12], e5[4],m5[4] );
  fprintf(txtfile,"$W^+/Z$   & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[13], m13[13], e13[5],m13[5],e5[13], m5[13], e5[5],m5[5] );
  fprintf(txtfile,"$W^-/Z$   & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[14], m13[14], e13[6],m13[6],e5[14], m5[14], e5[6],m5[6] );
  fprintf(txtfile,"$W/Z$     & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f & %.3f\\\\\n",e13[15], m13[15], e13[7],m13[7],e5[15], m5[15], e5[7],m5[7] );
  fprintf(txtfile,"\\hline\n" );
  fprintf(txtfile,"\\end{tabular} }\n" );
  fprintf(txtfile,"\\end{center}\n" );
  // fprintf(txtfile,"*" );
  fclose(txtfile);
}

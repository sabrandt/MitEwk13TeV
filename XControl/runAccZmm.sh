#!/bin/bash
workdir="/afs/cern.ch/user/x/xniu/WZXSection/CMSSW_7_6_3_patch2/src/MitEwk13TeV"
filedir="/afs/cern.ch/work/x/xniu/public/WZXSection/wz-efficiency"

root -l -b <<EOF
gSystem->Load("${workdir}/Acceptance/CEffUser1D_cc.so")
gSystem->Load("${workdir}/Acceptance/CEffUser2D_cc.so")

gSystem->Load("${workdir}/Acceptance/computeAccSelZmmBinned_C.so")
computeAccSelZmmBinned("${workdir}/Acceptance/zmm.conf", "${filedir}/Results/Zmm/Ori/", 1)

gSystem->Load("${workdir}/Acceptance/computeAccSelZmmBinned_Sys_C.so")
computeAccSelZmmBinned_Sys("${workdir}/Acceptance/zmm.conf", "${filedir}/Results/Zmm/SigUp", 1, "${filedir}/Results/MuSITSigSys.root", "${filedir}/Results/MuStaSigSys.root")
computeAccSelZmmBinned_Sys("${workdir}/Acceptance/zmm.conf", "${filedir}/Results/Zmm/BkgUp", 1, "${filedir}/Results/MuSITBkgSys.root", "${filedir}/Results/MuStaBkgSys.root")

gSystem->Load("${workdir}/Acceptance/computeAccSelZmmBinned_Bin_C.so")
computeAccSelZmmBinned_Bin("${workdir}/Acceptance/zmm.conf", "${filedir}/Results/Zmm/Bin", 1)

.q
EOF

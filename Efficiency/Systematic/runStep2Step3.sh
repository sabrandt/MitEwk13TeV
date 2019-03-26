#!/bin/bash
# lepton=$1
# efftype=$2
# binvar=$3
ClusterID=$1
binnum=$2
NTOYS=$3
FOLDER=$4
EFFTYPE=$5
CHARGE=$6

WORKDIR="/afs/cern.ch/work/s/sabrandt/public/SM/LowPU/CMSSW_9_4_12/src/MitEwk13TeV"
# FILEDIR="/afs/cern.ch/work/s/sabrandt/public/LowPU_13TeV_Efficiency_v1/results"
FILEDIR="/afs/cern.ch/work/s/sabrandt/public/SM/LowPU/CMSSW_9_4_12/src/MitEwk13TeV/Efficiency/testReweights_v2_2/results"

CMSSW_BASE="/afs/cern.ch/work/s/sabrandt/public/SM/LowPU/CMSSW_9_4_12/src/"
TOP="/afs/cern.ch/work/s/sabrandt/public/SM/LowPU/CMSSW_9_4_12/src/"
# TOP="$PWD"
# echo ${ClusterID}
echo "running job w Cluster ID ${ClusterID}, binnum ${binnum}, NTOYS ${NTOYS}, FOLDER ${FOLDER}, EFFTYPE ${EFFTYPE}, Charge ${CHARGE}"

POSTFIX=_POWxPythia_v1
POSTFIX_alt=_POWxPhotos_v1
#
# CMSSW_BASE="/afs/cern.ch/work/x/xniu/public/Lumi/Ele/CMSSW_7_6_3_patch2/src/"
# TOP="$PWD"
#
BINVAR=etapt # probably don't need to change
OUTPUTDIR=${FILEDIR}/${EFFTYPE}${POSTFIX}${POSTFIX_alt}/${CHARGE}/
# STAGEDIR=${TOP}/${EFFTYPE}/${CHARGE}/Step2Output/${POSTFIX}v${POSTFIX_alt}
STAGEDIR=.
# FOLDER=Zmm
# EFFTYPE=MuHLTEff #MuHLTEff, MuSelEff, MuStaEff
# NBINS=3 #Muons have 63 bins
# CHARGE=Negative
# NTOYS=1000
# binnum=1
# toynum=0
DIR1=${FILEDIR}/${FOLDER}/Data/${EFFTYPE}${POSTFIX_alt}/${CHARGE}/plots/
DIR2=${FILEDIR}/${FOLDER}/Data/${EFFTYPE}${POSTFIX}/${CHARGE}/plots/
cd $CMSSW_BASE
eval `scramv1 runtime -sh`
cd $TOP
mkdir -p ${OUTPUTDIR}
# mkdir -p ${TOP}/${EFFTYPE}/${CHARGE}/Step2Output/${POSTFIX}v${POSTFIX_alt}
#TOP="/afs/cern.ch/work/x/xniu/public/WZXSection/wz-efficiency"

root -l -b << EOF
gSystem->Load("${WORKDIR}/Efficiency/TagAndProbe/RooCMSShape_cc.so")
toyGenAndPull("${DIR1}","${DIR2}","${DIR2}","${BINVAR}_${binnum}","${OUTPUTDIR}","pull_${binnum}",${binnum},${binnum},${NTOYS})
gSystem->Load("${WORKDIR}/Efficiency/TagAndProbe/toyGenAndPull_C.so")
.q
EOF
# root -l -b << EOF
# gSystem->Load("${WORKDIR}/Efficiency/TagAndProbe/RooCMSShape_cc.so")
# gSystem->Load("${WORKDIR}/Efficiency/TagAndProbe/makePseudoData_C.so")
# makePseudoData("${DIR1}","${DIR2}","${BINVAR}_${binnum}","${OUTPUTDIR}","_${BINVAR}_${binnum}",${binnum},${binnum},${NTOYS})
# .q
# EOF
# echo "MADE IT TO STEP3" 

# for ((toynum=0; toynum<${NTOYS};toynum++)); do
# root -l -b << EOF
# gSystem->Load("${WORKDIR}/Efficiency/TagAndProbe/RooCMSShape_cc.so")
# gSystem->Load("${WORKDIR}/Efficiency/TagAndProbe/doStep3_C.so")
# doStep3("${STAGEDIR}","${BINVAR}_${binnum}_${toynum}.dat","${DIR2}/${BINVAR}_${binnum}.root","${OUTPUTDIR}","_${BINVAR}_${binnum}")
# .q
# EOF
# rm ${STAGEDIR}/${BINVAR}_${binnum}_${toynum}.dat
# done
# makePseudoData("${filedir}/${lepton}${efftype}Eff/CB/plots/", "${filedir}/${lepton}${efftype}Eff/MG/plots/", "${binvar}_${binnum}", "${TOP}/${lepton}${efftype}Eff/Step2Output/CB/",-1,${binnum},${toynum})
# doStep3("${TOP}/${lepton}${efftype}Eff/Step2Output/CB","${binvar}_${binnum}_${psenum}.dat","${filedir}/${lepton}${efftype}Eff/MG/plots/${binvar}_${binnum}.root", "${filedir}/${lepton}${efftype}Eff","_${binvar}_${binnum}")
. /cvmfs/cms.cern.ch/cmsset_default.sh
export SCRAM_ARCH=slc7_amd64_gcc700
cd /net/cms29/cms29r0/pico/cc7/CMSSW_10_2_11_patch1/src
eval `scramv1 runtime -sh`
cd -
export SCONSFLAGS="-j 4" # multi-core build
export SET_ENV_PATH=set_cmssw.sh # environment to use for build


starting_dir=`pwd`
echo "----------------------------------"
cd mu2e_pcie_utils
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_components
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_utilities
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e_calorimeter
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e_trigger
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e_crv
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e_extmon
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e_stm
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

echo "----------------------------------"
cd otsdaq_mu2e_tracker
echo `pwd`
git fetch
git status
git remote -v
cd $starting_dir

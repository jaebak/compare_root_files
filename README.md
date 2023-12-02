Compare two root files. 

- `root_scripts/compare_root_files_hist.cxx`: Compares two root files. Makes histograms for each branch.
  - Usage: `root compare_root_files_hist.cxx+(root_filename_a, root_filename_b, tree_name)`
- `src/compare_root_files_value.cxx`: Compares every branch value between two root files.
  - Usage: `run/compare_root_files_value.exe --root_a filename --root_b filename --tree_name tree --output output --nevents -1`
  - Limits: Due to way how inc/JTreeReaderHelper.h, currently can only compare one file.
- `root_scripts/read_miniaod.cxx`: Reads miniaod file
  - Usage: `root read_miniaod.cxx(root_filename_a, root_filename_b, tree_name)`
  - Requires: `source set_cmssw.sh`

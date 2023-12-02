#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <regex>
#include <getopt.h>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TMath.h"
#include "TSystem.h"
#include "TH1D.h"
#include "THStack.h"
#include "TChain.h"
#include "TTreeReader.h"
#include "TCanvas.h"
#include "TLorentzVector.h"
#include "TLegend.h"
#include "TPaveStats.h"
#include "TGraph.h"
#include "TLine.h"

#include "JTreeReaderHelper.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::set;
using std::tuple;
using std::pair;
using std::min;
using std::max;
using std::make_tuple;
using std::get;
using std::to_string;

namespace{
  string root_filename_a = "";
  string root_filename_b = "";
  string tree_name = "Events";
  string root_a_tag = "a";
  string root_b_tag = "b";
  string output_filename = "temp";
  long nEvents = -1;
  //long nEvents = 100;
}
void GetOptions(int argc, char *argv[]){
  while(true){
    static struct option long_options[] = {
      {"root_a", required_argument, 0, 'a'},
      {"root_b", required_argument, 0, 'b'},
      {"tree_name", required_argument, 0, 't'},
      {"tag_a", required_argument, 0, 0},
      {"tag_b", required_argument, 0, 0},
      {"output", required_argument, 0, 'o'},
      {"nevents", optional_argument, 0, 'n'},
      {0, 0, 0, 0}
    };

    char opt = -1;
    int option_index;
    opt = getopt_long(argc, argv, "a:b:o:n:t:", long_options, &option_index);

    if( opt == -1) break;

    string optname;
    switch(opt){
      case 'a':
        root_filename_a = optarg;
        break;
      case 'b':
        root_filename_b = optarg;
        break;
      case 't':
        tree_name = optarg;
        break;
      case 'o':
        output_filename = optarg;
        break;
      case 'n':
        nEvents = atoi(optarg);
        break;
      case 0:
        optname = long_options[option_index].name;
        if (optname == "tag_a") {
          root_a_tag = optarg;
        } else if (optname == "tag_b") {
          root_b_tag = optarg;
        } else {
          printf("Bad option! Found option name %s\n", optname.c_str());
        }
        break;
      default:
        printf("Bad option! getopt_long returned character code 0%o\n", opt);
        break;
    }
  }
}

void fill_map_event_to_entry(JTreeReader & reader, JTreeReaderHelper & helper, map<string, Long64_t> & event_to_entry){
  int index = 0;
  while (1) {
    bool is_next = reader.Next();
    if ((is_next)==0) break;
    Long64_t entry = reader.getEntryNumber();
    //cout<<"entry: "<<entry_a<<endl;
    //cout<<branchNameToType_a["event"]<<" "<<helper_a.s_ulong64("event")<<endl;
    //cout<<branchNameToType_a["luminosityBlock"]<<" "<<helper_a.s_uint("luminosityBlock")<<endl;
    //cout<<branchNameToType_a["run"]<<" "<<helper_a.s_uint("run")<<endl;
    string run_lumi_event = to_string(helper.s_uint("run"))+"_"+to_string(helper.s_uint("luminosityBlock"))+"_"+to_string(helper.s_ulong64("event"));
    event_to_entry[run_lumi_event] = entry;
    //cout<<run_lumi_event<<endl;
    //if (index==2) break;
    //else index++;
  }
}

int main(int argc, char *argv[]){

  time_t begtime, endtime;
  time(&begtime);

  GetOptions(argc, argv);

  gROOT->SetBatch(kTRUE);
  gStyle->SetOptStat(1111111);

  // Get files
  TChain * chain_a = new TChain(tree_name.c_str());
  chain_a->Add(root_filename_a.c_str());
  JTreeReaderHelper helper_a;
  JTreeReader reader_a(chain_a);
  map<string, string> branchNameToType_a = helper_a.getBranchNameToType(chain_a);
  reader_a.setBranches(branchNameToType_a, &helper_a);

  TChain * chain_b = new TChain(tree_name.c_str());
  chain_b->Add(root_filename_b.c_str());
  JTreeReaderHelper helper_b;
  JTreeReader reader_b(chain_b);
  map<string, string> branchNameToType_b = helper_b.getBranchNameToType(chain_b);
  reader_b.setBranches(branchNameToType_b, &helper_b);

  // Get all branch names
  set<string> all_branch_names;
  for (auto itBranch : branchNameToType_a) all_branch_names.insert(itBranch.first);
  for (auto itBranch : branchNameToType_b) all_branch_names.insert(itBranch.first);
  // Compare branch names and types
  set<string> only_a_branch_names;
  set<string> only_b_branch_names;
  set<string> common_branch_names;
  for (auto branch_name : all_branch_names) {
    bool is_branch_a = branchNameToType_a.find(branch_name) != branchNameToType_a.end();
    bool is_branch_b = branchNameToType_b.find(branch_name) != branchNameToType_b.end();
    if (!is_branch_a) only_b_branch_names.insert(branch_name);
    if (!is_branch_b) only_a_branch_names.insert(branch_name);
    if (is_branch_a && is_branch_b) {
      if (branchNameToType_a[branch_name] != branchNameToType_a[branch_name]) {
        cout<<"[Info] type of branch "<<branch_name<<" are different. ("<<branchNameToType_a[branch_name]<<","<<branchNameToType_a[branch_name]<<"). Will not be compared."<<endl;
      } else {
        common_branch_names.insert(branch_name);
      }
    }
  }
  // Print only branches
  if (only_a_branch_names.size() !=0) cout<<"[Info] root file ("<<root_filename_a<<") is has following additional branches: ";
  for (auto branch_name : only_a_branch_names) cout<<branch_name<<", ";
  if (only_a_branch_names.size() !=0) cout<<endl;
  if (only_b_branch_names.size() !=0) cout<<"[Info] root file ("<<root_filename_b<<") is has following additional branches: ";
  for (auto branch_name : only_b_branch_names) cout<<branch_name<<", ";
  if (only_b_branch_names.size() !=0) cout<<endl;
  //// Print common branches
  //cout<<"Common branches"<<endl;
  //for (auto branch_name : common_branch_names) cout<<"  "<<branch_name<<" "<<branchNameToType_a[branch_name]<<endl;

  // Get run, lumiblock, and events for files
  cout<<"Finding common events"<<endl;
  cout<<"Finding events"<<endl;
  map<string, Long64_t> event_to_entry_a;
  fill_map_event_to_entry(reader_a, helper_a, event_to_entry_a);
  map<string, Long64_t> event_to_entry_b;
  fill_map_event_to_entry(reader_b, helper_b, event_to_entry_b);
  // Find common events
  cout<<"Finding common"<<endl;
  std::vector<std::string> common_events;
  for (auto const & pair : event_to_entry_a) {
    if (event_to_entry_b.find(pair.first) != event_to_entry_b.end()) common_events.push_back(pair.first);
  }
  //for (auto entry : common_events) cout<<entry<<endl;
  cout<<"Found "<<common_events.size()<<" events"<<endl;

  // Compare branches for common events
  cout<<"Finding differences"<<endl;
  // different_branches[branch_name] = [(iEntry, iValue)], where iValue is -1 for scalar case and -2 is for vector case where size is different
  map<string, vector< tuple<Long64_t, int, float, float> > > different_branches;
  Long64_t ievent = 0;
  float target_percent = 0;
  for (string const & event : common_events) {

    // Print 10% at a time
    if (ievent*1./common_events.size() > target_percent) {
      cout<<"[Info] ievent: "<<ievent<<" percent: "<<target_percent<<"%"<<endl;
      target_percent += 0.1;
    }

    Long64_t entry_a = event_to_entry_a[event];
    Long64_t entry_b = event_to_entry_b[event];
    //cout<<"entry: "<<entry_a<<" "<<entry_b<<endl;

    reader_a.SetEntry(entry_a);
    reader_b.SetEntry(entry_b);

    //cout<<branchNameToType_a["event"]<<" "<<helper_a.s_ulong64("event")<<endl;
    //cout<<branchNameToType_a["luminosityBlock"]<<" "<<helper_a.s_uint("luminosityBlock")<<endl;
    //cout<<branchNameToType_a["run"]<<" "<<helper_a.s_uint("run")<<endl;

    //cout<<branchNameToType_b["event"]<<" "<<helper_b.s_ulong64("event")<<endl;
    //cout<<branchNameToType_b["luminosityBlock"]<<" "<<helper_b.s_uint("luminosityBlock")<<endl;
    //cout<<branchNameToType_b["run"]<<" "<<helper_b.s_uint("run")<<endl;


    for (auto branch_name : common_branch_names) { // Loop over branches
      //if (branch_name != "ll_charge") continue;
      //cout<<"Comapre "<<branch_name<<" "<<branchNameToType_a[branch_name]<<endl;
      if (branchNameToType_a[branch_name] == "Bool_t") { // Compare Bool_t 
        if (!std::isnan(helper_a.s_bool(branch_name)) || !std::isnan(helper_b.s_bool(branch_name))) {
          if (helper_a.s_bool(branch_name) != helper_b.s_bool(branch_name)) {
            if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_bool(branch_name)<<" "<<helper_b.s_bool(branch_name)<<endl;
            different_branches[branch_name].push_back({entry_a, -1, helper_a.s_bool(branch_name), helper_b.s_bool(branch_name)});
          }
        }
      } else if (branchNameToType_a[branch_name] == "UInt_t") { // Compare UInt_t
        if (!std::isnan(helper_a.s_uint(branch_name)) || !std::isnan(helper_b.s_uint(branch_name))) {
          if (helper_a.s_uint(branch_name) != helper_b.s_uint(branch_name)) {
            if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_uint(branch_name)<<" "<<helper_b.s_uint(branch_name)<<endl;
            different_branches[branch_name].push_back({entry_a, -1, helper_a.s_uint(branch_name), helper_b.s_uint(branch_name)});
          }
        }
      } else if (branchNameToType_a[branch_name] == "ULong64_t") { // Compare ULong64_t
        if (!std::isnan(helper_a.s_ulong64(branch_name)) || !std::isnan(helper_b.s_ulong64(branch_name))) {
          if (helper_a.s_ulong64(branch_name) != helper_b.s_ulong64(branch_name)) {
            if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_ulong64(branch_name)<<" "<<helper_b.s_ulong64(branch_name)<<endl;
            different_branches[branch_name].push_back({entry_a, -1, helper_a.s_ulong64(branch_name), helper_b.s_ulong64(branch_name)});
          }
        }
      } else if (branchNameToType_a[branch_name] == "Long64_t") { // Compare Long64_t
        if (!std::isnan(helper_a.s_long64(branch_name)) || !std::isnan(helper_b.s_long64(branch_name))) {
          if (helper_a.s_long64(branch_name) != helper_b.s_long64(branch_name)) {
            if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_long64(branch_name)<<" "<<helper_b.s_long64(branch_name)<<endl;
            different_branches[branch_name].push_back({entry_a, -1, helper_a.s_long64(branch_name), helper_b.s_long64(branch_name)});
          }
        }
      } else if (branchNameToType_a[branch_name] == "Int_t") { // Compare Int_t
        if (!std::isnan(helper_a.s_int(branch_name)) || !std::isnan(helper_b.s_int(branch_name))) {
         if (helper_a.s_int(branch_name) != helper_b.s_int(branch_name)) {
           if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_int(branch_name)<<" "<<helper_b.s_int(branch_name)<<endl;
           different_branches[branch_name].push_back({entry_a, -1, helper_a.s_int(branch_name), helper_b.s_int(branch_name)});
         }
        }
      } else if (branchNameToType_a[branch_name] == "Float_t") { // Compare Float_t
        if (!std::isnan(helper_a.s_float(branch_name)) || !std::isnan(helper_b.s_float(branch_name))) {
          if (helper_a.s_float(branch_name) != helper_b.s_float(branch_name)) {
            if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_float(branch_name)<<" "<<helper_b.s_float(branch_name)<<endl;
            different_branches[branch_name].push_back({entry_a, -1, helper_a.s_float(branch_name), helper_b.s_float(branch_name)});
          }
        }
      } else if (branchNameToType_a[branch_name] == "Double_t") { // Compare Double_t
        if (!std::isnan(helper_a.s_double(branch_name)) || !std::isnan(helper_b.s_double(branch_name))) {
          if (helper_a.s_double(branch_name) != helper_b.s_double(branch_name)) {
            if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_double(branch_name)<<" "<<helper_b.s_double(branch_name)<<endl;
            different_branches[branch_name].push_back({entry_a, -1, helper_a.s_double(branch_name), helper_b.s_double(branch_name)});
          }
        }
      } else if (branchNameToType_a[branch_name] == "UChar_t") { // Compare UChar_t
        if (!std::isnan(helper_a.s_uchar(branch_name)) || !std::isnan(helper_b.s_uchar(branch_name))) {
         if (helper_a.s_uchar(branch_name) != helper_b.s_uchar(branch_name)) {
           if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different values: "<<helper_a.s_uchar(branch_name)<<" "<<helper_b.s_uchar(branch_name)<<endl;
           different_branches[branch_name].push_back({entry_a, -1, helper_a.s_uchar(branch_name), helper_b.s_uchar(branch_name)});
         }
        }
      } else if (branchNameToType_a[branch_name] == "vector<bool>" || branchNameToType_a[branch_name] == "vector<Bool_t>") { // Compare vector<bool>
        if (helper_a.v_bool(branch_name).GetSize() != helper_b.v_bool(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_bool(branch_name).GetSize()<<" "<<helper_b.v_bool(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_bool(branch_name).GetSize(); ++iValue) {
            if (!std::isnan(helper_a.v_bool(branch_name)[iValue]) || !std::isnan(helper_b.v_bool(branch_name)[iValue])) {
              if (helper_a.v_bool(branch_name)[iValue] != helper_b.v_bool(branch_name)[iValue]) {
                if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values: "<<helper_a.v_bool(branch_name)[iValue]<<" "<<helper_b.v_bool(branch_name)[iValue]<<endl;
                different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_bool(branch_name)[iValue], helper_b.v_bool(branch_name)[iValue]});
              }
            }
          }
        }
      } else if (branchNameToType_a[branch_name] == "vector<char>" || branchNameToType_a[branch_name] == "vector<Char_t>") { // Compare vector<char>
        if (helper_a.v_char(branch_name).GetSize() != helper_b.v_char(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_char(branch_name).GetSize()<<" "<<helper_b.v_char(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_char(branch_name).GetSize(); ++iValue) {
            if (!std::isnan(helper_a.v_char(branch_name)[iValue]) || !std::isnan(helper_b.v_char(branch_name)[iValue])) {
             if (helper_a.v_char(branch_name)[iValue] != helper_b.v_char(branch_name)[iValue]) {
               if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values: "<<helper_a.v_char(branch_name)[iValue]<<" "<<helper_b.v_char(branch_name)[iValue]<<endl;
               different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_char(branch_name)[iValue], helper_b.v_char(branch_name)[iValue]});
             }
            }
          }
        }
      } else if (branchNameToType_a[branch_name] == "vector<UChar_t>") { // Compare vector<UChar_t>
        if (helper_a.v_uchar(branch_name).GetSize() != helper_b.v_uchar(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_uchar(branch_name).GetSize()<<" "<<helper_b.v_uchar(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_uchar(branch_name).GetSize(); ++iValue) {
            if (!std::isnan(helper_a.v_uchar(branch_name)[iValue]) || !std::isnan(helper_b.v_uchar(branch_name)[iValue])) {
             if (helper_a.v_uchar(branch_name)[iValue] != helper_b.v_uchar(branch_name)[iValue]) {
               if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values: "<<helper_a.v_uchar(branch_name)[iValue]<<" "<<helper_b.v_uchar(branch_name)[iValue]<<endl;
               different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_uchar(branch_name)[iValue], helper_b.v_uchar(branch_name)[iValue]});
             }
            }
          }
        }
      } else if (branchNameToType_a[branch_name] == "vector<int>" || branchNameToType_a[branch_name] == "vector<Int_t>") { // Compare vector<int>
        if (helper_a.v_int(branch_name).GetSize() != helper_b.v_int(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_int(branch_name).GetSize()<<" "<<helper_b.v_int(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_int(branch_name).GetSize(); ++iValue) {
            if (!std::isnan(helper_a.v_int(branch_name)[iValue]) || !std::isnan(helper_b.v_int(branch_name)[iValue])) {
             if (helper_a.v_int(branch_name)[iValue] != helper_b.v_int(branch_name)[iValue]) {
               if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values: "<<helper_a.v_int(branch_name)[iValue]<<" "<<helper_b.v_int(branch_name)[iValue]<<endl;
               different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_int(branch_name)[iValue], helper_b.v_int(branch_name)[iValue]});
             }
            }
          }
        }
      } else if (branchNameToType_a[branch_name] == "vector<float>" || branchNameToType_a[branch_name] == "vector<Float_t>") { // Compare vector<float>
        if (helper_a.v_float(branch_name).GetSize() != helper_b.v_float(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_float(branch_name).GetSize()<<" "<<helper_b.v_float(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_float(branch_name).GetSize(); ++iValue) {
            if (!std::isnan(helper_a.v_float(branch_name)[iValue]) || !std::isnan(helper_b.v_float(branch_name)[iValue])) {
              if (helper_a.v_float(branch_name)[iValue] != helper_b.v_float(branch_name)[iValue]) {
                if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values: "<<helper_a.v_float(branch_name)[iValue]<<" "<<helper_b.v_float(branch_name)[iValue]<<endl;
                different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_float(branch_name)[iValue], helper_b.v_float(branch_name)[iValue]});
              }
            }
          }
        }
      } else if (branchNameToType_a[branch_name] == "vector<double>" || branchNameToType_a[branch_name] == "vector<Double_t>") { // Compare vector<double>
        if (helper_a.v_double(branch_name).GetSize() != helper_b.v_double(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_double(branch_name).GetSize()<<" "<<helper_b.v_double(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_double(branch_name).GetSize(); ++iValue) {
            if (!std::isnan(helper_a.v_double(branch_name)[iValue]) || !std::isnan(helper_b.v_double(branch_name)[iValue])) {
              if (helper_a.v_double(branch_name)[iValue] != helper_b.v_double(branch_name)[iValue]) {
                if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values: "<<helper_a.v_double(branch_name)[iValue]<<" "<<helper_b.v_double(branch_name)[iValue]<<endl;
                different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_double(branch_name)[iValue], helper_b.v_double(branch_name)[iValue]});
              }
            }
          }
        }
      } else if (branchNameToType_a[branch_name] == "vector<TLorentzVector>") { // Compare vector<TLorentzVector>
        if (helper_a.v_TLorentzVector(branch_name).GetSize() != helper_b.v_TLorentzVector(branch_name).GetSize()) { // Compare size
          if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<" has different sizes: "<<helper_a.v_TLorentzVector(branch_name).GetSize()<<" "<<helper_b.v_TLorentzVector(branch_name).GetSize()<<endl;
          different_branches[branch_name].push_back({entry_a, -2, 0, 0});
        } else { // Compare values
          for (unsigned iValue=0; iValue < helper_a.v_TLorentzVector(branch_name).GetSize(); ++iValue) {
            if (helper_a.v_TLorentzVector(branch_name)[iValue] != helper_b.v_TLorentzVector(branch_name)[iValue]) {
              if (different_branches.find(branch_name) == different_branches.end()) cout<<"[Info] "<<branch_name<<"["<<iValue<<"] has different values. "<<helper_a.v_TLorentzVector(branch_name)[iValue].Pt()<<" "<<helper_b.v_TLorentzVector(branch_name)[iValue].Pt()<<endl;
              different_branches[branch_name].push_back({entry_a, iValue, helper_a.v_TLorentzVector(branch_name)[iValue].M(), helper_b.v_TLorentzVector(branch_name)[iValue].M()});
            }
          }
        }
      } else {
        cout<<"[Info] Unknown branch type ("<<branchNameToType_a[branch_name]<<") for "<<branch_name<<endl;
      }
    } // Loop over branches

    ievent++;
    //break;
  }

  // Print results of different events
  cout<<"Compared "<<common_events.size()<<endl;
  if (different_branches.size() != 0) {
    cout<<"[Info] Differences in following files."<<endl;
    cout<<"  - "<<root_filename_a<<endl;
    cout<<"  - "<<root_filename_b<<endl;
    cout<<"(entry, iValue), where iValue=-1 means the branch is a scalar and -2 means the vector size is different."<<endl;
    for (auto mBranch : different_branches) {
      cout<<"Branch :"<<mBranch.first<<" is different ("<<mBranch.second.size()<<" times): ";
      int iEntry = 0;
      for (auto entry_value: mBranch.second) {
        cout<<"("<<get<0>(entry_value)<<" "<<get<1>(entry_value)<<"), ";
        iEntry++;
        if (iEntry==50) break;
      }
      cout<<endl;
    }
  } else {
    cout<<"[Info] There were no differences in common branches."<<endl;
  }

  // Write to file
  std::ofstream output;
  output.open(output_filename);
  output<<"Compared "<<common_events.size()<<endl;
  if (different_branches.size() != 0) {
    output<<"[Info] Differences in following files."<<endl;
    output<<"  - "<<root_filename_a<<endl;
    output<<"  - "<<root_filename_b<<endl;
    output<<"(entry, iValue), where iValue=-1 means the branch is a scalar and -2 means the vector size is different."<<endl;
    for (auto mBranch : different_branches) {
      output<<"Branch :"<<mBranch.first<<" is different ("<<mBranch.second.size()<<" times): ";
      for (auto entry_value: mBranch.second) {
        output<<"("<<get<0>(entry_value)<<" "<<get<1>(entry_value)<<" A:"<<get<2>(entry_value)<<" B:"<<get<3>(entry_value)<<"), ";
      }
      output<<endl;
    }
  } else {
    output<<"[Info] There were no differences in common branches."<<endl;
  }

  time(&endtime); 
  cout<<endl<<"Took "<<difftime(endtime, begtime)<<" seconds"<<endl<<endl;
  return 0;
}

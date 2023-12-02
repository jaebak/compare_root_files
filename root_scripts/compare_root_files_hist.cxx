#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include "TChain.h"
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

TCanvas * new_canvas(string const & name = "", int size = 500)
{
  TSeqCollection * canvases = gROOT->GetListOfCanvases();
  double iCanvas = canvases->GetEntries();
  string canvasName;
  if (name == "") canvasName = "c_g_" + to_string(iCanvas++);
  else canvasName = name;
  return new TCanvas(canvasName.c_str(), canvasName.c_str(), size, size);
}

void setMaximum(float max)
{
  TList * list = gPad->GetListOfPrimitives();
  TIter next(list);
  while (TObject * obj = next())
  {
    std::string className = obj->ClassName();
    if (className.find("TH1") != std::string::npos)
    {
      TH1 * th1 = static_cast<TH1*>(obj);
      th1->SetMaximum(max);
    }
    if (className.find("THStack") != std::string::npos)
    {
      THStack * thstack = static_cast<THStack*>(obj);
      thstack->SetMaximum(max);
    }
  }
  gPad->Modified();
  gPad->Update();

  //TH1 * obj = (TH1*)(gPad->GetListOfPrimitives()->First());
  //obj->SetMaximum(max);
  //gPad->Update();
}

double getMaximumTH1()
{
  TList * list = gPad->GetListOfPrimitives();
  TIter next(list);
  int index = 0;
  double max = 0;
  while (TObject * obj = next())
  {
    std::string className = obj->ClassName();
    if (className.find("TH1") != std::string::npos)
    {
      TH1 * th1 = static_cast<TH1*>(obj);
      double t_max = th1->GetMaximum();
      if (t_max>max || index==0) max = t_max;
    }
    index++;
  }
  return max;
}

double setMaximumTH1(double maxFraction = 1.05)
{
  double max = getMaximumTH1() * maxFraction;
  setMaximum(max);
  return 1;
}

void scale_histogram(TH1F* hist) {
  hist->Scale(1./hist->GetXaxis()->GetBinWidth(1)/hist->Integral());
}

string get_histname(long pid, string variable_name) {
  return "pid_"+to_string(pid)+"_"+variable_name;
}

void fill_histograms(TTree * tree, map<string, tuple<string, double, double> > const & hist_definitions, map<string,TH1F*> & histograms, int nbins, vector<string> const & ignore_variables, string const & tag) {
  // Make histograms
  new_canvas();
  for (auto hist_definition : hist_definitions) {
    if (find(ignore_variables.begin(), ignore_variables.end(), get<0>(hist_definition.second)) != ignore_variables.end()) continue;
    histograms[hist_definition.first] = new TH1F((hist_definition.first+"_"+tag).c_str(), hist_definition.first.c_str(), nbins, get<1>(hist_definition.second), get<2>(hist_definition.second));
    //cout<<get<0>(hist_definition.second)<<" "<<hist_definition.first<<" pid=="+to_string(get<1>(hist_definition.second))<<endl;
    //tree->Scan((get<0>(hist_definition.second)+":pid:weight").c_str(), ("pid=="+to_string(get<1>(hist_definition.second))).c_str()); 
    tree->Draw((get<0>(hist_definition.second)+">>"+hist_definition.first+"_"+tag).c_str());
    //cout<<hist_definition.first<<" "<<"pid=="+to_string(get<1>(hist_definition.second))<<endl;
  }
  //cout<<"min: "<<get<2>(hist_definitions.at("weight"))<<" max: "<<get<3>(hist_definitions.at("weight"))<<endl;
  //cout<<"tree min: "<<tree->GetMinimum("weight")<<" tree max: "<<tree->GetMaximum("weight")<<endl;
}

vector<string> get_sorted_keys(map<string, TH1F*> const & t_map) {
  vector<string> keys;
  keys.reserve(t_map.size());
  for (auto item : t_map) {
    keys.push_back(item.first);
  }
  sort(keys.begin(), keys.end());
  return keys;
}


// Usage: root "compare_root_files_hist.cxx+(root_filename_a, root_filename_b, tree_name)"
void compare_root_files_hist(string root_filename_a="", string root_filename_b="", string tree_name="tree") {
  time_t begtime, endtime;
  time(&begtime);

  if (root_filename_a == "" || root_filename_b == "") {
    cout<<"Usage: root \"compare_root_tuples.cxx+(\\\"root_filename_a\\\", \\\"root_filename_b\\\", \\\"tree_name\\\")\""<<endl;
    return;
  }
  string a_tag = "a";
  string b_tag = "b";
  int nbins = 30;
  string output = "compare";

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

  // Make histogram definitions
  // hist_definitions[hist_name] = [var, min, max]
  cout<<"Making histogram definitions, by finding min and max"<<endl;
  map<string, tuple<string, double, double> > hist_definitions_a;
  map<string, tuple<string, double, double> > hist_definitions_b;
  cout<<"Common branches: ";
  for (auto branch_name : common_branch_names) {
    string hist_name = "h_"+branch_name;
    double branch_min = min(chain_a->GetMinimum(branch_name.c_str()), chain_b->GetMinimum(branch_name.c_str()));
    double branch_max = max(chain_a->GetMaximum(branch_name.c_str()), chain_b->GetMaximum(branch_name.c_str()));
    hist_definitions_a[hist_name] = {branch_name, branch_min, branch_max};
    hist_definitions_b[hist_name] = {branch_name, branch_min, branch_max};
    cout<<branch_name<<", ";
  }
  cout<<endl;
  cout<<"Only a branches: ";
  for (auto branch_name : only_a_branch_names) {
    string hist_name = "h_"+branch_name;
    double branch_min = chain_a->GetMinimum(branch_name.c_str());
    double branch_max = chain_b->GetMaximum(branch_name.c_str());
    hist_definitions_a[hist_name] = {branch_name, branch_min, branch_max};
    cout<<branch_name<<", ";
  }
  cout<<endl;
  cout<<"Only b branches: ";
  for (auto branch_name : only_b_branch_names) {
    string hist_name = "h_"+branch_name;
    double branch_min = chain_b->GetMinimum(branch_name.c_str());
    double branch_max = chain_b->GetMaximum(branch_name.c_str());
    hist_definitions_b[hist_name] = {branch_name, branch_min, branch_max};
    cout<<branch_name<<", ";
  }
  cout<<endl;
  // Fill histograms

  // Fill histograms
  map<string,TH1F*> histograms_a;
  map<string,TH1F*> histograms_b;
  vector<string> ignore_variables = {};
  //map<string,TH1F*> histograms_b;
  cout<<"Filling histograms from a"<<endl;
  fill_histograms(chain_a, hist_definitions_a, histograms_a, nbins, ignore_variables, /*tag*/ a_tag);
  cout<<"Filling histograms from b"<<endl;
  fill_histograms(chain_b, hist_definitions_b, histograms_b, nbins, ignore_variables, /*tag*/ b_tag);

  //for (auto & histogram_a : histograms_a) {
  //  cout<<histogram_a.first<<endl;
  //  TCanvas * canvas = new_canvas();
  //  histogram_a.second->SetMinimum(0);
  //  histogram_a.second->Draw();
  //  canvas->SaveAs(("plots/"+histogram_a.first+".pdf").c_str());
  //}

  // Draw histograms
  cout<<"Drawing histograms"<<endl;
  vector<string> keys_a = get_sorted_keys(histograms_a);
  vector<string> keys_b = get_sorted_keys(histograms_b);
  //for (auto key : keys_a) cout<<"keys_a: "<<key<<endl;
  //for (auto key : keys_b) cout<<"keys_b: "<<key<<endl;
  set<string> keys_set (keys_a.begin(), keys_a.end());
  copy(keys_b.begin(), keys_b.end(), inserter(keys_set, keys_set.end()));
  //for (auto key : keys_set) cout<<"set: "<<key<<endl;
  vector<string> keys;
  keys.assign(keys_set.begin(), keys_set.end());
  //for (auto key : keys) cout<<"vector: "<<key<<endl;

  //string paper_name = "plots/compare.pdf";
  string paper_name = output+".pdf";
  TCanvas * paper = new_canvas();
  paper->Print((paper_name+"[").c_str());

  // Draw common variables
  for (string const & key : keys) {
    int a_b_combine = 0;
    if (find(keys_a.begin(), keys_a.end(), key) != keys_a.end()) a_b_combine += 1;
    if (find(keys_b.begin(), keys_b.end(), key) != keys_b.end()) a_b_combine += 2;
    cout<<key<<" (1=a,2=b,3=ab): "<<a_b_combine<<endl;
    
    TCanvas * canvas = new_canvas();
    TPad * up_pad = new TPad("up_pad", "up_pad", /*xlow*/0, /*ylow*/0.3, /*xhigh*/1., /*yhigh*/1);
    TPad * low_pad = new TPad("low_pad", "low_pad", /*xlow*/0, /*ylow*/0, /*xhigh*/1., /*yhigh*/0.3);

    up_pad->Draw();
    low_pad->Draw();

    up_pad->cd();
    double x_min = 0, x_max = 0;
    if (a_b_combine == 1 || a_b_combine == 3) {
      histograms_a[key]->SetMinimum(0);
      histograms_a[key]->SetLineWidth(2);
      histograms_a[key]->SetLineColor(9);
      x_min = histograms_a[key]->GetXaxis()->GetBinLowEdge(histograms_a[key]->GetXaxis()->GetFirst());
      x_max = histograms_a[key]->GetXaxis()->GetBinUpEdge(histograms_a[key]->GetXaxis()->GetLast());
      char ytitle[64]; snprintf(ytitle, sizeof ytitle, "Events / %.3g", (x_max-x_min)/nbins);
      histograms_a[key]->GetYaxis()->SetTitle(ytitle);
      histograms_a[key]->Draw("HIST E");
      gPad->Update(); // To force draw for statbox
      TPaveStats *statbox_a = static_cast<TPaveStats*>(histograms_a[key]->FindObject("stats"));
      statbox_a->SetTextColor(9);
      statbox_a->SetY1NDC(0.7);
      statbox_a->SetY2NDC(0.9);
    } else if (a_b_combine == 2) {
      histograms_b[key]->SetMinimum(0);
      histograms_b[key]->SetLineWidth(2);
      histograms_b[key]->SetLineColor(9);
      x_min = histograms_b[key]->GetXaxis()->GetBinLowEdge(histograms_b[key]->GetXaxis()->GetFirst());
      x_max = histograms_b[key]->GetXaxis()->GetBinUpEdge(histograms_b[key]->GetXaxis()->GetLast());
      char ytitle[64]; snprintf(ytitle, sizeof ytitle, "Events / %.3g", (x_max-x_min)/nbins);
      histograms_b[key]->GetYaxis()->SetTitle(ytitle);
      histograms_b[key]->Draw("HIST E");
      gPad->Update(); // To force draw for statbox
      TPaveStats *statbox_b = static_cast<TPaveStats*>(histograms_b[key]->FindObject("stats"));
      statbox_b->SetTextColor(9);
      statbox_b->SetY1NDC(0.7);
      statbox_b->SetY2NDC(0.9);
    }
    if (a_b_combine == 3) {
      // Normalize b to a
    //if (path_b != "") {
      //histograms_b[key]->Scale(1./histograms_a[key]->GetXaxis()->GetBinWidth(1)/histograms_a[key]->Integral());
      //cout<<histograms_a[key]->GetXaxis()->GetBinWidth(1)<<" "<<histograms_a[key]->Integral()<<endl;
      //cout<<histograms_b[key]->GetXaxis()->GetBinWidth(1)<<" "<<histograms_b[key]->Integral()<<endl;
      histograms_b[key]->Scale(histograms_a[key]->Integral()/histograms_b[key]->Integral());
      histograms_b[key]->SetLineColor(kRed);
      histograms_b[key]->Draw("HIST E sames");
      gPad->Update(); // To force draw for statbox
      TPaveStats *statbox_b = static_cast<TPaveStats*>(histograms_b[key]->FindObject("stats"));
      statbox_b->SetTextColor(kRed);
      statbox_b->SetY1NDC(0.5);
      statbox_b->SetY2NDC(0.7);
    //}
    }

    setMaximumTH1(1.3);

    //TLegend * legend = new TLegend(/*x1*/0.1, /*y1*/0.8, /*x2*/0.3, /*y2*/0.9);
    //legend->AddEntry(histograms_a[key], a_tag.c_str());
    //legend->AddEntry(histograms_b[key], b_tag.c_str());
    //legend->Draw();

    if (a_b_combine == 3) {
    //if (path_b != "") {
      low_pad->cd();
      if (fabs(x_max - x_min)>0.000001) { // Prevent residuals for one bin histograms
        vector<Double_t> res_y(static_cast<unsigned>(nbins));
        vector<Double_t> res_x(static_cast<unsigned>(nbins));
        for (unsigned index = 0; index < static_cast<unsigned>(nbins); ++index) res_x[index] = x_min + (x_max-x_min)/nbins * index + (x_max-x_min)/nbins/2 ;
        gErrorIgnoreLevel = kError;
        double pvalue = histograms_b[key]->Chi2Test(histograms_a[key], "WW", &res_y[0]);
        gErrorIgnoreLevel = kPrint;
        TGraph * residual = new TGraph(nbins, &res_x[0], &res_y[0]);
        residual->SetTitle(0);
        // Set range on computed graph. Set twice because TGraph's axis looks strange otherwise
        residual->GetXaxis()->SetLimits(x_min, x_max);
        residual->GetYaxis()->SetRangeUser(-3.5, 3.5);
        residual->GetYaxis()->SetTitle("Normalized residuals");
        residual->GetYaxis()->SetTitleSize(0.07);
        residual->GetYaxis()->SetTitleOffset(0.5);
        residual->GetYaxis()->CenterTitle();
        residual->SetMarkerStyle(21);
        residual->SetMarkerSize(0.3);
        residual->Draw("AP");
        TLine * line = new TLine(/*x1*/x_min,/*y1*/0,/*x2*/x_max,/*y2*/0);
        line->Draw();
        TPaveText * text_box = new TPaveText(/*x1*/0.12, /*y1*/0.8, /*x2*/0.5, /*y2*/0.9, "NDC NB");
        text_box->SetFillColorAlpha(0,0);
        char c_pvalue[64]; snprintf(c_pvalue, sizeof c_pvalue, "#chi^{2} test (%s vs %s) p-value: %.3g", a_tag.c_str(), b_tag.c_str(), pvalue);
        text_box->AddText(c_pvalue);
        text_box->Draw();
      }
    //}
    }

    canvas->Print(paper_name.c_str());
  }
  paper->Print((paper_name+"]").c_str());

  time(&endtime);
  cout<<endl<<"Took "<<difftime(endtime, begtime)<<" seconds"<<endl<<endl;
}

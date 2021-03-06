#include <iostream>

#include <TH1F.h>
#include <TFile.h>
#include <TMath.h>
#include <TROOT.h>
#include <TString.h>
#include <TSystem.h>
#include <Rtypes.h>

#include <TAxis.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TAttLine.h>
#include <TPaveText.h>

#include "$CMSSW_BASE/src/HiggsAnalysis/HiggsToTauTau/interface/HttStyles.h"
#include "$CMSSW_BASE/src/HiggsAnalysis/HiggsToTauTau/src/HttStyles.cc"

static const float SIGNAL_SCALE = 1.;

/**
   \class   postfit postfit.C "HiggsAnalysis/HiggsToTauTau/macros/postfit.C"

   \brief   macro to make plots for di-tau masses for all classic htt channels

   This is a macro to create di-tau masses for all classic htt channels combined
*/

float maximum(TH1F* h, bool LOG=false){
  if(LOG){
    return 5.*h->GetMaximum();
  }
  else{
    if(h->GetMaximum()>  12){ return 10.*TMath::Nint((1.35*h->GetMaximum()/10.)); }
    if(h->GetMaximum()> 1.2){ return TMath::Nint((1.65*h->GetMaximum())); }
    return 1.6*h->GetMaximum(); 
  }
}

TH1F* refill(TH1F* hin, const char* sample)
/*
  refill histograms, for MC histograms set bin errors to zero.
*/
{
  if(hin==0){
    std::cout << "hist not found: " << sample << " -- this may happen for samples of type signal." << std::endl;
    bool skip = false;
    if(std::string(sample).find("ggH")==std::string::npos){ skip == true ; }
    if(skip || std::string(sample).find("Zmm")==std::string::npos){ skip == true; }
    if(skip || std::string(sample).find("Zee")==std::string::npos){ skip == true; }
    if(skip || std::string(sample).find("Fakes/QCD")==std::string::npos){ skip == true; }
    if(skip){
      std::cout << "hist is not of type signal, Fakes/QCD, Zmm in mumu, Zee in ee, close here" << std::endl;
      exit(1);
    }
    else{
      return hin;
    }
  }
  TH1F* hout = (TH1F*)hin->Clone(); hout->Clear();
  for(int i=0; i<hout->GetNbinsX(); ++i){
    // simple refill, histograms are already devided by bin width
    // but for a useful lotting the bin errors for MC need to be 
    // set to zero.
    hout->SetBinContent(i+1, hin->GetBinContent(i+1));
    hout->SetBinError(i+1, 0.);
  }
  return hout;
}

void 
postfit_use(const char* inputfile, const char* analysis = "SM", const char* dataset = "2011+2012", const char* extra="", const char* extra2="", float min=0.1, float max=-1., bool log=true)
{
  // defining the common canvas, axes pad styles
  SetStyle(); gStyle->SetLineStyleString(11,"20 10");
  // switch for MSSM/SM
  bool MSSM = std::string(analysis) == std::string("MSSM");
  // determine label
  if (std::string(dataset) == std::string("2011"     )){ dataset = "CMS Preliminary,  H#rightarrow#tau#tau, 4.9 fb^{-1} at 7 TeV"; }
  if (std::string(dataset) == std::string("2012"     )){ dataset = "CMS Preliminary,  H#rightarrow#tau#tau, 19.8 fb^{-1} at 8 TeV"; }
  if (std::string(dataset) == std::string("2011+2012")){ dataset = "CMS Preliminary,  H#rightarrow#tau#tau, 4.9 fb^{-1} at 7 TeV, 19.8 fb^{-1} at 8 TeV"; }
  // determine category tag
  const char* category_extra = "";
  if(std::string(extra2) == std::string("0jet_low"  )){ category_extra = "0 jet, low p_{T}";  }
  if(std::string(extra2) == std::string("0jet_high" )){ category_extra = "0 jet, high p_{T}"; }
  if(std::string(extra2) == std::string("0jet"      )){ category_extra = "0 jet";             }
  if(std::string(extra2) == std::string("1jet_low"  )){ category_extra = "1 jet, low p_{T}";  }
  if(std::string(extra2) == std::string("1jet_high" )){ category_extra = "1 jet, high p_{T}"; }
  if(std::string(extra2) == std::string("1jet"      )){ category_extra = "1 jet";             }
  if(std::string(extra2) == std::string("vbf"       )){ category_extra = "2 jet (VBF)";       }
  if(std::string(extra2) == std::string("nobtag"    )){ category_extra = "No B-Tag";          }
  if(std::string(extra2) == std::string("btag"      )){ category_extra = "B-Tag";             }

  TFile* input = new TFile(inputfile);
  TH1F* Fakes  = refill((TH1F*)input->Get("Fakes"   ), "Fakes/QCD"); 
  TH1F* EWK    = refill((TH1F*)input->Get("EWK"     ), "EWK"      ); 
  TH1F* ttbar  = refill((TH1F*)input->Get("ttbar"   ), "ttbar"    ); 
  TH1F* Ztt    = refill((TH1F*)input->Get("Ztt"     ), "Ztt"      ); 
  TH1F* Zmm    = refill((TH1F*)input->Get("Zmm"     ), "Zmm"      ); 
  TH1F* Zee    = refill((TH1F*)input->Get("Zee"     ), "Zee"      ); 
  TH1F* ggH    = refill((TH1F*)input->Get("ggH"     ), "ggH"      ); 
  TH1F* data   = (TH1F*)input->Get("data_obs"); 
  // determine channel for etau Z->ee (EWK) will be shown separated from the rest (EWK1)
  TH1F* EWK1   = 0;
  if(std::string(extra) == std::string("e#tau_{h}")){
    EWK1 = refill((TH1F*)input->Get("EWK1"),  "EWK1");
  }
  TH1F* errorBand = (TH1F*)input->Get("errorBand");

  /* 
    mass plot before and after fit
  */
  TCanvas *canv = MakeCanvas("canv", "histograms", 600, 600);
  if(log) canv->SetLogy(1);
  // reduce the axis range if necessary for linea plots and SM
  if(MSSM && !log){ data->GetXaxis()->SetRange(0, data->FindBin(345)); } else{ data->GetXaxis()->SetRange(0, data->FindBin(695)); };
  if(!MSSM){ data->GetXaxis()->SetRange(0, data->FindBin(345)); }
  data->SetNdivisions(505);
  data->SetMinimum(min);
  if(Zmm){
    data->SetMaximum(max>0 ? max : std::max(maximum(data, log), maximum(Zmm, log)));
  }
  else{
    data->SetMaximum(max>0 ? max : std::max(maximum(data, log), maximum(Ztt, log)));
  }
  data->Draw("e");

  if(log){
    if(Zmm){
      Zmm  ->Draw("same");
      Ztt  ->Draw("same");
      ttbar->Draw("same");
      Fakes->Draw("same");
      EWK  ->Draw("same");
    }
    else if(Zee){
      EWK  ->Draw("same");
      ttbar->Draw("same");
      Fakes->Draw("same");
      Zee  ->Draw("same");
      Ztt  ->Draw("same");
    }
    else{
      Ztt  ->Draw("same");
      ttbar->Draw("same");
      EWK  ->Draw("same");
      if(EWK1){
	EWK1->Draw("same");
      }
      if(Fakes){ Fakes->Draw("same"); }
    }
    if(ggH) ggH  ->Draw("histsame");
  }
  else{
    if(ggH) ggH  ->Draw("histsame");
    if(Zmm){
      Zmm->Draw("same");
      Ztt->Draw("same");
      ttbar->Draw("same");
      Fakes->Draw("same");
      EWK->Draw("same");
    }   
    else if(Zee){
      EWK->Draw("same");
      Fakes->Draw("same");
      ttbar->Draw("same");
      Zee->Draw("same");
      Ztt->Draw("same");
    }else{
      Ztt  ->Draw("same");
      ttbar->Draw("same");
      EWK  ->Draw("same");
      if(EWK1){
	EWK1->Draw("same");
      }
      if(Fakes){ Fakes->Draw("same"); }
    }
  }
  if(errorBand){
    errorBand->Draw("e2same");
  }
  data->Draw("esame");
  canv->RedrawAxis();

  //CMSPrelim(dataset, extra, 0.17, 0.835);
  CMSPrelim(dataset, "", 0.18, 0.835);  
  TPaveText* chan     = new TPaveText(0.20, 0.74+0.061, 0.32, 0.74+0.161, "NDC");
  chan->SetBorderSize(   0 );
  chan->SetFillStyle(    0 );
  chan->SetTextAlign(   12 );
  chan->SetTextSize ( 0.05 );
  chan->SetTextColor(    1 );
  chan->SetTextFont (   62 );
  chan->AddText(extra);
  chan->Draw();

  TPaveText* cat      = new TPaveText(0.20, 0.68+0.061, 0.32, 0.68+0.161, "NDC");
  cat->SetBorderSize(   0 );
  cat->SetFillStyle(    0 );
  cat->SetTextAlign(   12 );
  cat->SetTextSize ( 0.05 );
  cat->SetTextColor(    1 );
  cat->SetTextFont (   62 );
  cat->AddText(category_extra);
  cat->Draw();

  if(MSSM){
    float lower_bound = EWK1 ? 0.45 : 0.50;
    TPaveText* massA      = new TPaveText(0.55, lower_bound+0.061, 0.95, lower_bound+0.161, "NDC");
    massA->SetBorderSize(   0 );
    massA->SetFillStyle(    0 );
    massA->SetTextAlign(   12 );
    massA->SetTextSize ( 0.03 );
    massA->SetTextColor(    1 );
    massA->SetTextFont (   62 );
    massA->AddText("m^{h}_{max} (m_{A}=$MA GeV, tan#beta=$TANB)");
    massA->Draw();
  }    
  float lower_bound = EWK1 ? 0.60 : 0.65;
  TLegend* leg = new TLegend(MSSM ? 0.55 : 0.50, lower_bound, 0.93, 0.90);
  SetLegendStyle(leg);
  if(MSSM){
    leg->AddEntry(ggH  , "#phi#rightarrow#tau#tau", "L" );
  }
  else{
    if(ggH){
      if(SIGNAL_SCALE!=1){
	leg->AddEntry(ggH  , TString::Format("%.0f#timesH(125 GeV)#rightarrow#tau#tau", SIGNAL_SCALE) , "L" );
      }
      else{
	leg->AddEntry(ggH  , "H(125 GeV)#rightarrow#tau#tau" , "L" );
      }
    }
  }
  leg->AddEntry(data , "observed"                       , "LP");
  leg->AddEntry(Ztt  , "Z#rightarrow#tau#tau"           , "F" );
  if(Zmm){ leg->AddEntry(Zmm  , "Z#rightarrow#mu#mu"    , "F" ); }
  if(Zee){ leg->AddEntry(Zee  , "Z#rightarrowee"        , "F" ); }
  if(EWK1){
    leg->AddEntry(EWK  , "Z#rightarrow ee"              , "F" );
    leg->AddEntry(EWK1 , "electroweak"                  , "F" );
  }
  else{
    leg->AddEntry(EWK  , "electroweak"                  , "F" );
  }
  leg->AddEntry(ttbar, "t#bar{t}"                       , "F" );
  if(Fakes){ leg->AddEntry(Fakes, "QCD"                 , "F" ); }
  if(errorBand){
    leg->AddEntry(errorBand, "bkg. uncertainty" , "F" );
  }
  leg->Draw();

  /*
  TPaveText* ext0     = new TPaveText(0.50, lower_bound-0.08, 0.70, lower_bound-0.03, "NDC");
  ext0->SetBorderSize(   0 );
  ext0->SetFillStyle(    0 );
  ext0->SetTextAlign(   12 );
  ext0->SetTextSize ( 0.035 );
  ext0->SetTextColor(    1 );
  ext0->SetTextFont (   42 );
  ext0->AddText("CMS Preliminary");
  ext0->Draw();

  TPaveText* ext1     = new TPaveText(0.50, lower_bound-0.13, 0.70, lower_bound-0.08, "NDC");
  ext1->SetBorderSize(   0 );
  ext1->SetFillStyle(    0 );
  ext1->SetTextAlign(   12 );
  ext1->SetTextSize ( 0.035 );
  ext1->SetTextColor(    1 );
  ext1->SetTextFont (   42 );
  ext1->AddText("#sqrt{s} = 7 TeV, L = 4.9 fb^{-1}");
  ext1->Draw();

  TPaveText* ext2     = new TPaveText(0.50, lower_bound-0.18, 0.70, lower_bound-0.13, "NDC");
  ext2->SetBorderSize(   0 );
  ext2->SetFillStyle(    0 );
  ext2->SetTextAlign(   12 );
  ext2->SetTextSize ( 0.035 );
  ext2->SetTextColor(    1 );
  ext2->SetTextFont (   42 );
  ext2->AddText("#sqrt{s} = 8 TeV, L = 19.4 fb^{-1}");
  ext2->Draw();
  
  TPaveText* ext3     = new TPaveText(0.50, lower_bound-0.23, 0.70, lower_bound-0.18, "NDC");
  ext3->SetBorderSize(   0 );
  ext3->SetFillStyle(    0 );
  ext3->SetTextAlign(   12 );
  ext3->SetTextSize ( 0.035 );
  ext3->SetTextColor(    1 );
  ext3->SetTextFont (   42 );
  ext3->AddText("H#rightarrow#tau#tau");
  ext3->Draw();
  */

  /*
    prepare output
  */
  std::string newName = std::string(inputfile).substr(0, std::string(inputfile).find(".root"));
  //canv->Print(TString::Format("%s%s.png", newName.c_str(), log ? "_LOG" : "")); 
  //canv->Print(TString::Format("%s%s.pdf", newName.c_str(), log ? "_LOG" : "")); 
  //canv->Print(TString::Format("%s%s.eps", newName.c_str(), log ? "_LOG" : "")); 
  canv->Print(TString::Format("%s.png", newName.c_str())); 
  canv->Print(TString::Format("%s.pdf", newName.c_str())); 
  canv->Print(TString::Format("%s.eps", newName.c_str())); 


  /*
    Ratio Data over MC
  */
  TCanvas *canv0 = MakeCanvas("canv0", "histograms", 600, 400);
  canv0->SetGridx();
  canv0->SetGridy();
  canv0->cd(); 
  TH1F* zero = (TH1F*)Ztt->Clone("zero"); zero->Clear();
  TH1F* rat1 = (TH1F*)data->Clone("rat"); 
  if(Zmm){
    rat1->Divide(Zmm);
  }
  else{
    rat1->Divide(Ztt);
  }
  for(int ibin=0; ibin<rat1->GetNbinsX(); ++ibin){
    if(rat1->GetBinContent(ibin+1)>0){
      // catch cases of 0 bins, which would lead to 0-alpha*0-1
      rat1->SetBinContent(ibin+1, rat1->GetBinContent(ibin+1)-1.);
    }
    zero->SetBinContent(ibin+1, 0.);
  }
  rat1->SetLineColor(kBlack);
  rat1->SetFillColor(kGray );
  rat1->SetMaximum(+0.5);
  rat1->SetMinimum(-0.5);
  rat1->GetYaxis()->CenterTitle();
  rat1->GetYaxis()->SetTitle("#bf{Data/MC-1}");
  rat1->GetXaxis()->SetTitle("#bf{m_{#tau#tau} [GeV]}");
  rat1->Draw();
  zero->SetLineColor(kBlack);
  zero->Draw("same");
  canv0->RedrawAxis();

  /*
    prepare output
  */
  newName = std::string(inputfile).substr(0, std::string(inputfile).find(".root")) + "_datamc";
  canv0->Print(TString::Format("%s.png", newName.c_str())); 
  canv0->Print(TString::Format("%s.pdf", newName.c_str())); 
  canv0->Print(TString::Format("%s.eps", newName.c_str())); 
}

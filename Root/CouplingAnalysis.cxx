#include "HGamCouplingAnalysis/CouplingAnalysis.h"
#include "HGamAnalysisFramework/HgammaIncludes.h"
#include "HGamAnalysisFramework/HGamVariables.h"

// Additional Helpers
#include "HGamCouplingAnalysis/STXSHelpers.h"
#include "HGamCouplingAnalysis/HiggsHelpers.h"
#include <cmath>


// this is needed to distribute the algorithm to the workers
ClassImp(CouplingAnalysis)



CouplingAnalysis::CouplingAnalysis(const char *name)
: HgammaAnalysis(name)
{
  // Here you put any code for the base initialization of variables,
  // e.g. initialize all pointers to 0.  Note that you should only put
  // the most basic initialization here, since this method will be
  // called on both the submission and the worker node.  Most of your
  // initialization code will go into histInitialize() and
  // initialize().
}



CouplingAnalysis::~CouplingAnalysis()
{
  // Here you delete any memory you allocated during your analysis.
}



EL::StatusCode CouplingAnalysis::createOutput()
{
  // Loop Over Systematics? 
  m_useSystematics = config()->getBool("HGamCoupling.UseSystematics", false);

  // Only apply the reweighting to ggH MC
  //m_reweightHiggsPt = config()->getBool("HGamCoupling.ReweightHiggsPt", false);
  
  int mcid = (isData()) ? -999 : eventInfo()->mcChannelNumber();

  m_isGGH = false;
  m_isTWH = false;
  if (isMC()) {
    m_isGGH = getMCSampleName(mcid).Contains("ggH125");
    m_isTWH = getMCSampleName(mcid).Contains("tWH125");
    //m_reweightHiggsPt &= m_isGGH;
  } //else m_reweightHiggsPt = false;
    

  //if (m_reweightHiggsPt) std::cout << "*** !!! REWEIGHTING HIGGS PT !!! ***" << std::endl;
  //else               std::cout << "*** !!! NOT REWEIGHTING HIGGS PT !!! ***" << std::endl;
      
  m_usePDFUncerts = false;
  if (isMC()) {
    StrV higgsTypes = config()->getStrV("EventHandler.HiggsWeights.Types", {});
    for (TString sample: higgsTypes) {
      StrV dsids = config()->getStrV("EventHandler.HiggsWeights."+sample, {""});
      for (TString dsid: dsids) {
        if (mcid == std::atoi(dsid.Data())) {
          m_usePDFUncerts = true;
        }
      }
    }
  }
  m_usePDFUncerts &= config()->getBool("HGamCoupling.UsePDFSystematics", false);

  // Create Data TTree
  if (isData()) {
    m_tree = new TTree("physics","physics");
    wk()->addOutput(m_tree);
    m_tree->Branch( "category", &m_category );
    m_tree->Branch( "m_yy",     &m_myy );
  }

  // Create Histograms
  int nCats(31), nBins(42), nIndex(65);

  histoStore()->createTH1F( "h_truthAcc_fineIndex_weightMC", nIndex, -0.5, nIndex-0.5 );
  histoStore()->createTH1F( "h_truthAcc_fineIndex_weight", nIndex, -0.5, nIndex-0.5 );

  histoStore()->createTH1F( "h_truthAcc_weightMC", nBins, -0.5, nBins-0.5 );
  histoStore()->createTH1F( "h_truthAcc_weight", nBins, -0.5, nBins-0.5 );

  histoStore()->createTH1F( "h_initialEvents_weighted",      1, -10, 10 );
  histoStore()->createTH1F( "h_initialEvents_weighted_pTRW", 1, -10, 10 ); 

  histoStore()->createTH1F( "h_initialEvents_noDalitz_weighted",      1, -10, 10 );
  histoStore()->createTH1F( "h_initialEvents_noDalitz_weighted_pTRW", 1, -10, 10 ); 

  HG::SystematicList allSys = getSystematics();
  int Nsysts = (isData()) ? 0 : allSys.size()-1;
  if (Nsysts > 0) {
    std::cout << "Running over " << Nsysts << " systematics:" << std::endl;
    for (int i(0); i <= Nsysts; i++ )
      std::cout << "\t" << i << " " << allSys[i].name() << std::endl;
  }
  std::cout << std::endl;

  static int sysIndex = config()->getInt("HGamCoupling.SystematicIndex", -1);
  if (sysIndex > Nsysts) 
    std::cout << "Canceling job for syst index " << sysIndex 
              << ", there are only " << Nsysts << " variations." << std::endl;

  if (sysIndex < 0) sysList = allSys;
  else sysList.push_back( allSys[sysIndex] );

  if (Nsysts > 0) std::cout << "Running over " << Nsysts << " systematics:" << std::endl;

  TString suffix = ""; 
  for (auto sys: sysList) {
    if (sys.name() != "") {
      if (not m_useSystematics) break;
      if (isData()) break;

      TString sysName = sys.name();
      //if (sysName.Contains("Trig")) continue;
      //if (sysName.Contains("_CorrUncertainty")) continue;
      
      if (Nsysts > 0) std::cout << "\t" << sysName << std::endl;

      suffix = "_" + sys.name();
      suffix.ReplaceAll(" ","_");
    }

    histoStore()->createTH1F(  "h_catSTXS"+suffix,  nCats, 0.5, nCats+0.5 );
    histoStore()->createTH2F( "h2_catSTXS"+suffix,  nCats, 1, nCats+1, nBins, 0, nBins );
    histoStore()->createTH2F( "h2_fineIndex"+suffix,  nCats, 1, nCats+1, nIndex, 0, nIndex );
  }

  // Histograms below only for nominal samples
  if (sysIndex > 0) return EL::StatusCode::SUCCESS;
    
  if (m_usePDFUncerts) {
    for (TString suffix: {"_alphaS_up","_alphaS_dn"}) {
      histoStore()->createTH1F( "h_catSTXS"+suffix,  nCats, 0.5, nCats+0.5 );
      histoStore()->createTH2F( "h2_catSTXS"+suffix,  nCats, 1, nCats+1, nBins, 0, nBins );
      histoStore()->createTH2F( "h2_fineIndex"+suffix,  nCats, 1, nCats+1, nIndex, 0, nIndex );
    }

    for (int ipdf(0); ipdf < 30; ipdf++) {
      TString suffixPDF = TString::Format("_PDF%d",ipdf);
      histoStore()->createTH1F( "h_catSTXS"+suffixPDF,  nCats, 0.5, nCats+0.5 );
      histoStore()->createTH2F( "h2_catSTXS"+suffixPDF,  nCats, 1, nCats+1, nBins, 0, nBins );
      histoStore()->createTH2F( "h2_fineIndex"+suffixPDF,  nCats, 1, nCats+1, nIndex, 0, nIndex );
    }

    xAOD::HiggsWeights higgsWeights = eventHandler()->higgsWeights();
    m_NQCD = higgsWeights.qcd.size();
    for (int iqcd(0); iqcd < m_NQCD; iqcd++) {
      TString suffixQCD = TString::Format("_QCD%d",iqcd);
      histoStore()->createTH1F( "h_catSTXS"+suffixQCD,  nCats, 0.5, nCats+0.5 );
      histoStore()->createTH2F( "h2_catSTXS"+suffixQCD,  nCats, 1, nCats+1, nBins, 0, nBins );
      histoStore()->createTH2F( "h2_fineIndex"+suffixQCD,  nCats, 1, nCats+1, nIndex, 0, nIndex );
    }

    if (m_isGGH) {
      for (int iqcd(0); iqcd < 27; iqcd++) {
        TString suffixQCD = TString::Format("_NNLOPS_QCD%d",iqcd);
        histoStore()->createTH1F( "h_catSTXS"+suffixQCD,  nCats, 0.5, nCats+0.5 );
        histoStore()->createTH2F( "h2_catSTXS"+suffixQCD,  nCats, 1, nCats+1, nBins, 0, nBins );
        histoStore()->createTH2F( "h2_fineIndex"+suffixQCD,  nCats, 1, nCats+1, nIndex, 0, nIndex );
      }

      for (int iqcd(0); iqcd < 9; iqcd++) {
        TString suffixQCD = TString::Format("_QCD_2017_%s", qcdNames[iqcd].Data());
        histoStore()->createTH1F( "h_catSTXS"+suffixQCD,  nCats, 0.5, nCats+0.5 );
        histoStore()->createTH2F( "h2_catSTXS"+suffixQCD,  nCats, 1, nCats+1, nBins, 0, nBins );
        histoStore()->createTH2F( "h2_fineIndex"+suffixQCD, nCats, 1, nCats+1, nIndex, 0, nIndex );
      }
    }
  }

  for ( int icat(1); icat < nCats+1; icat++ ) {
    TString histName = TString::Format("h_myy_cat%d",icat);
    histoStore()->createTH1F( histName, 110, 105, 160, ";m_{#gamma#gamma} [GeV];Events / GeV");
  }

  return EL::StatusCode::SUCCESS;
}




EL::StatusCode CouplingAnalysis::execute()
{
  // Important to keep this, so that internal tools / event variables are filled properly.
  HgammaAnalysis::execute();

  // Blind the data
  //if (isData() && var::m_yy() >= 120*HG::GeV && var::m_yy() < 130*HG::GeV) return EL::StatusCode::SUCCESS;

  // event weights
  double wInit = (isData()) ? 1.0 : weightInitial() * lumiXsecWeight();
  double wMC   = (isData()) ? 1.0 : eventHandler()->mcWeight() * lumiXsecWeight();
  double w     = (isData()) ? 1.0 : weightCatCoup_Moriond2017BDT() * lumiXsecWeight();

  //if (isMC() && fabs(eventHandler()->mcWeight()) > 150) {
  //  wMC = lumiXsecWeight() * 150 * wMC / fabs(wMC);
  //}

  // Save Histogram for Truth Acceptance
  int stage1(0), errorCode(0);
  if (isMC() && eventInfo()->isAvailable<int>("HTXS_Stage1_Category_pTjet30")) {
    stage1   = eventInfo()->auxdata<int>("HTXS_Stage1_Category_pTjet30");
    errorCode = eventInfo()->auxdata<int>("HTXS_errorCode");
    if (errorCode != 0) stage1 = -1;
  }
  
  int fineIndex(0);
  if (isMC() && eventInfo()->isAvailable<int>("HTXS_Stage1_FineIndex_pTjet30")) {
    fineIndex = eventInfo()->auxdata<int>("HTXS_Stage1_FineIndex_pTjet30");
    if (stage1 / 100 == 8) {
      fineIndex = 49 + (stage1%100);
      if (m_isTWH) fineIndex += 2;
    }
    if ((stage1/100 == 1) && (eventInfo()->auxdata<int>("HTXS_prodMode")==5)) {
      fineIndex += 52;  // split off ggZH_gg2H bins
    }
  }

  // Create Histograms for Truth Acceptances ( requires unskimmed samples )
  int STXSbin = STXS::stage1_to_index( stage1 );
  if (m_isTWH && STXSbin > 0) STXSbin += 2;

  histoStore()->fillTH1F( "h_truthAcc_fineIndex_weightMC", fineIndex, wMC );
  histoStore()->fillTH1F( "h_truthAcc_fineIndex_weight", fineIndex, wInit );

  histoStore()->fillTH1F( "h_truthAcc_weightMC", STXSbin, wMC );
  histoStore()->fillTH1F( "h_truthAcc_weight", STXSbin, wInit );

  // Create histos for weighting
  if ( isMC() ) {
    histoStore()->fillTH1F( "h_initialEvents_weighted_pTRW", 1.0, weightInitial() );
    histoStore()->fillTH1F( "h_initialEvents_weighted",      1.0, weightInitial() );
  }

  if ( isMC() && not var::isDalitzEvent() ) {
    histoStore()->fillTH1F( "h_initialEvents_noDalitz_weighted_pTRW", 1.0, weightInitial() );
    histoStore()->fillTH1F( "h_initialEvents_noDalitz_weighted",      1.0, weightInitial() );
  }

  // PDF, alpha_S, and ggH QCD uncertainties
  if (isMC() && m_usePDFUncerts && !var::isDalitzEvent()) {
    xAOD::HiggsWeights higgsWeights = eventHandler()->higgsWeights();
    m_category = -999;

    double wASHI = wInit * higgsWeights.alphaS_up / higgsWeights.nominal;
    double wASLO = wInit * higgsWeights.alphaS_dn / higgsWeights.nominal;

    if (!std::isfinite(wASHI)) wASHI = wInit;
    if (!std::isfinite(wASLO)) wASLO = wInit;
    
    histoStore()->fillTH1F(  "h_catSTXS",   m_category, wInit );
    histoStore()->fillTH2F( "h2_catSTXS",   m_category, STXSbin,   wInit );
    histoStore()->fillTH2F( "h2_fineIndex", m_category, fineIndex, wInit );

    histoStore()->fillTH1F(  "h_catSTXS_alphaS_up",   m_category, wASHI );
    histoStore()->fillTH1F(  "h_catSTXS_alphaS_dn",   m_category, wASLO );

    histoStore()->fillTH2F( "h2_catSTXS_alphaS_up",   m_category, STXSbin, wASHI );
    histoStore()->fillTH2F( "h2_catSTXS_alphaS_dn",   m_category, STXSbin, wASLO );

    histoStore()->fillTH2F( "h2_fineIndex_alphaS_up", m_category, fineIndex, wASHI );
    histoStore()->fillTH2F( "h2_fineIndex_alphaS_dn", m_category, fineIndex, wASLO );

    for (int ipdf(0); ipdf < 30; ipdf++)  {
      double wPDF = wInit * higgsWeights.pdf4lhc_unc[ipdf] / higgsWeights.nominal;
      if (!std::isfinite(wPDF)) wPDF = wInit;
      //double wPDF = w * higgsWeights.pdf4lhc_unc[ipdf] / higgsWeights.nominal;
      //if (!std::isfinite(wPDF)) wPDF = w;
      TString suffixPDF = TString::Format("_PDF%d",ipdf);
      histoStore()->fillTH1F(  "h_catSTXS"+suffixPDF,   m_category, wPDF );
      histoStore()->fillTH2F( "h2_catSTXS"+suffixPDF,   m_category, STXSbin, wPDF );
      histoStore()->fillTH2F( "h2_fineIndex"+suffixPDF, m_category, fineIndex, wPDF );
    }

    for (int iqcd(0); iqcd < m_NQCD; iqcd++)  {
      double wQCD = wInit * higgsWeights.qcd[iqcd] / higgsWeights.nominal;
      if (!std::isfinite(wQCD)) wQCD = wInit;
      TString suffixQCD = TString::Format("_QCD%d",iqcd);
      histoStore()->fillTH1F(  "h_catSTXS"+suffixQCD,   m_category, wQCD );
      histoStore()->fillTH2F( "h2_catSTXS"+suffixQCD,   m_category, STXSbin, wQCD );
      histoStore()->fillTH2F( "h2_fineIndex"+suffixQCD, m_category, fineIndex, wQCD );
    }

    if (m_isGGH) { 
      for (int iqcd(0); iqcd < 27; iqcd++)  {
        double wQCD = wInit * higgsWeights.qcd_nnlops[iqcd] / higgsWeights.nominal;
        TString suffixQCD = TString::Format("_NNLOPS_QCD%d",iqcd);
        histoStore()->fillTH1F(  "h_catSTXS"+suffixQCD,   m_category, wQCD );
        histoStore()->fillTH2F( "h2_catSTXS"+suffixQCD,   m_category, STXSbin, wQCD );
        histoStore()->fillTH2F( "h2_fineIndex"+suffixQCD, m_category, fineIndex, wQCD );
      }

      for (int iqcd(0); iqcd < 9; iqcd++) {
        double wQCD = wInit * higgsWeights.qcd_2017[iqcd] / higgsWeights.nominal;
        TString suffixQCD = TString::Format("_QCD_2017_%s", qcdNames[iqcd].Data());
        histoStore()->fillTH1F(  "h_catSTXS"+suffixQCD,   m_category, wQCD );
        histoStore()->fillTH2F( "h2_catSTXS"+suffixQCD,   m_category, STXSbin,   wQCD );
        histoStore()->fillTH2F( "h2_fineIndex"+suffixQCD, m_category, fineIndex, wQCD );
      }
    }
  }

  // Loop over systematic variations
  TString suffix = "";
  for (auto sys: sysList) {

    bool nominal = (sys.name() == "");
    if (not nominal) {
      if (not m_useSystematics) break;
      if (isData()) break;

      TString sysName = sys.name();

      suffix = "_" + sys.name();
      suffix.ReplaceAll(" ","_");
      applySystematicVariation(sys);
    }

    if (not var::isPassed()) return EL::StatusCode::SUCCESS;

    w = (isData()) ? 1.0 : weightCatCoup_Moriond2017BDT() * lumiXsecWeight();
    if (w == 0.) return EL::StatusCode::SUCCESS;
    
    //m_category = var::catCoup_Moriond2017();
    m_category = var::catCoup_Moriond2017BDT();
    if      (m_category > 23) m_category -= 2;
    else if (m_category > 19) m_category -= 1;

    histoStore()->fillTH1F(  "h_catSTXS"+suffix,   m_category, w );
    histoStore()->fillTH2F( "h2_catSTXS"+suffix,   m_category, STXSbin, w );
    histoStore()->fillTH2F( "h2_fineIndex"+suffix, m_category, fineIndex, w );

    if (nominal) {
      TString histName = TString::Format("h_myy_cat%d", m_category);
      histoStore()->fillTH1F( histName, var::m_yy()*HG::invGeV, w );

      m_myy = var::m_yy()*HG::invGeV;
      if (isData()) m_tree->Fill();

      // PDF, alpha_S, and ggH QCD uncertainties
      if (isMC() && m_usePDFUncerts) {
        xAOD::HiggsWeights higgsWeights = eventHandler()->higgsWeights();

        double wASHI = w * higgsWeights.alphaS_up / higgsWeights.nominal;
        double wASLO = w * higgsWeights.alphaS_dn / higgsWeights.nominal;

        if (!std::isfinite(wASHI)) wASHI = w;
        if (!std::isfinite(wASLO)) wASLO = w;

        histoStore()->fillTH1F(  "h_catSTXS_alphaS_up",   m_category, wASHI );
        histoStore()->fillTH1F(  "h_catSTXS_alphaS_dn",   m_category, wASLO );

        histoStore()->fillTH2F( "h2_catSTXS_alphaS_up",   m_category, STXSbin, wASHI );
        histoStore()->fillTH2F( "h2_catSTXS_alphaS_dn",   m_category, STXSbin, wASLO );

        histoStore()->fillTH2F( "h2_fineIndex_alphaS_up", m_category, fineIndex, wASHI );
        histoStore()->fillTH2F( "h2_fineIndex_alphaS_dn", m_category, fineIndex, wASLO );

        for (int ipdf(0); ipdf < 30; ipdf++)  {
          double wPDF = w * higgsWeights.pdf4lhc_unc[ipdf] / higgsWeights.nominal;
          if (!std::isfinite(wPDF)) wPDF = w;
          TString suffixPDF = TString::Format("_PDF%d",ipdf);
          histoStore()->fillTH1F(  "h_catSTXS"+suffixPDF,   m_category, wPDF );
          histoStore()->fillTH2F( "h2_catSTXS"+suffixPDF,   m_category, STXSbin, wPDF );
          histoStore()->fillTH2F( "h2_fineIndex"+suffixPDF, m_category, fineIndex, wPDF );
        }

        for (int iqcd(0); iqcd < m_NQCD; iqcd++)  {
          double wQCD = w * higgsWeights.qcd[iqcd] / higgsWeights.nominal;
          if (!std::isfinite(wQCD)) wQCD = w;
          TString suffixQCD = TString::Format("_QCD%d",iqcd);
          histoStore()->fillTH1F(  "h_catSTXS"+suffixQCD,   m_category, wQCD );
          histoStore()->fillTH2F( "h2_catSTXS"+suffixQCD,   m_category, STXSbin, wQCD );
          histoStore()->fillTH2F( "h2_fineIndex"+suffixQCD, m_category, fineIndex, wQCD );
        }
        
        if (m_isGGH) { 
          for (int iqcd(0); iqcd < 27; iqcd++)  {
            double wQCD = w * higgsWeights.qcd_nnlops[iqcd] / higgsWeights.nominal;
            TString suffixQCD = TString::Format("_NNLOPS_QCD%d",iqcd);
            histoStore()->fillTH1F(  "h_catSTXS"+suffixQCD,   m_category, wQCD );
            histoStore()->fillTH2F( "h2_catSTXS"+suffixQCD,   m_category, STXSbin, wQCD );
            histoStore()->fillTH2F( "h2_fineIndex"+suffixQCD, m_category, fineIndex, wQCD );
          }

          for (int iqcd(0); iqcd < 9; iqcd++) {
            double wQCD = w * higgsWeights.qcd_2017[iqcd] / higgsWeights.nominal;
            TString suffixQCD = TString::Format("_QCD_2017_%s", qcdNames[iqcd].Data());
            histoStore()->fillTH1F(  "h_catSTXS"+suffixQCD,   m_category, wQCD );
            histoStore()->fillTH2F( "h2_catSTXS"+suffixQCD,   m_category, STXSbin,   wQCD );
            histoStore()->fillTH2F( "h2_fineIndex"+suffixQCD, m_category, fineIndex, wQCD );
          }
        }
    
      }

    }
    
  }

  return EL::StatusCode::SUCCESS;
}

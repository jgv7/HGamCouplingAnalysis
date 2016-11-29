from ROOT import *
import prettyplots
import HGamMoriondCats as HG

gStyle.SetOptStat(0)
prettyplots.setPalette("rainbow")

Stage1_Labels = ["UNKNOWN","GG2H_FWDH","GG2H_VBFTOPO_JET3VETO","GG2H_VBFTOPO_JET3","GG2H_0J","GG2H_1J_PTH_0_60","GG2H_1J_PTH_60_120","GG2H_1J_PTH_120_200","GG2H_1J_PTH_GT200","GG2H_GE2J_PTH_0_60","GG2H_GE2J_PTH_60_120","GG2H_GE2J_PTH_120_200","GG2H_GE2J_PTH_GT200","VBF_QQ2HQQ_FWDH","QQ2HQQ_VBFTOPO_JET3VETO","QQ2HQQ_VBFTOPO_JET3","QQ2HQQ_VH2JET","QQ2HQQ_REST","QQ2HQQ_PTJET1_GT200","QQ2HLNU_FWDH","QQ2HLNU_PTV_0_150","QQ2HLNU_PTV_150_250_0J","QQ2HLNU_PTV_150_250_GE1J","QQ2HLNU_PTV_GT250","QQ2HLL_FWDH","QQ2HLL_PTV_0_150","QQ2HLL_PTV_150_250_0J","QQ2HLL_PTV_150_250_GE1J","QQ2HLL_PTV_GT250","GG2HLL_FWDH","GG2HLL_PTV_0_150","GG2HLL_PTV_GT150_0J","GG2HLL_PTV_GT150_GE1J","TTH_FWDH","TTH","BBH_FWDH","BBH","TH_FWDH","TH"]

HCombStage1_Labels = ["UNKNOWN","GG2H_FWDH","GG2H_VBFTOPO_JET3VETO","GG2H_VBFTOPO_JET3","GG2H_0J","GG2H_1J_PTH_0_60","GG2H_1J_PTH_60_120","GG2H_1J_PTH_120_200","GG2H_1J_PTH_GT200","GG2H_GE2J_PTH_0_60","GG2H_GE2J_PTH_60_120","GG2H_GE2J_PTH_120_200","GG2H_GE2J_PTH_GT200","VBF_QQ2HQQ_FWDH","VBF_QQ2HQQ_VBFTOPO_JET3VETO","VBF_QQ2HQQ_VBFTOPO_JET3","VBF_QQ2HQQ_VH2JET","VBF_QQ2HQQ_REST","VBF_QQ2HQQ_PTJET1_GT200","WH_QQ2HQQ_FWDH","WH_QQ2HQQ_VBFTOPO_JET3VETO","WH_QQ2HQQ_VBFTOPO_JET3","WH_QQ2HQQ_VH2JET","WH_QQ2HQQ_REST","WH_QQ2HQQ_PTJET1_GT200","ZH_QQ2HQQ_FWDH","ZH_QQ2HQQ_VBFTOPO_JET3VETO","ZH_QQ2HQQ_VBFTOPO_JET3","ZH_QQ2HQQ_VH2JET","ZH_QQ2HQQ_REST","ZH_QQ2HQQ_PTJET1_GT200","QQ2HLNU_FWDH","QQ2HLNU_PTV_0_150","QQ2HLNU_PTV_150_250_0J","QQ2HLNU_PTV_150_250_GE1J","QQ2HLNU_PTV_GT250","QQ2HLL_FWDH","QQ2HLL_PTV_0_150","QQ2HLL_PTV_150_250_0J","QQ2HLL_PTV_150_250_GE1J","QQ2HLL_PTV_GT250","GG2HLL_FWDH","GG2HLL_PTV_0_150","GG2HLL_PTV_GT150_0J","GG2HLL_PTV_GT150_GE1J","TTH_FWDH","TTH","BBH_FWDH","BBH","THQB_FWDH","THQB","TWH_FWDH","TWH"]

CatLabels = HG.CatLabels


def decorateHist( hist, ztitle=None ):
  for ibin, catName in enumerate(CatLabels, 1):      hist.GetXaxis().SetBinLabel( ibin, catName )
  for jbin, binName in enumerate(Stage1_Labels, 1):  hist.GetYaxis().SetBinLabel( jbin, binName )
  hist.GetXaxis().LabelsOption("v")
  hist.GetXaxis().SetTitle("Reco Category")
  hist.GetYaxis().SetTitle("STXS Truth Bin")
  if (ztitle): hist.GetZaxis().SetTitle(ztitle)
  hist.GetXaxis().SetTitleOffset(4.0)
  hist.GetYaxis().SetTitleOffset(4.5)
  hist.SetMinimum(0.0)
  hist.GetXaxis().SetRangeUser( 1, len(CatLabels)+1 )
  hist.GetYaxis().SetRangeUser( 0., len(Stage1_Labels) )
  #hist.GetXaxis().SetRangeUser(  0.5, len(CatLabels)+0.5     )
  #hist.GetYaxis().SetRangeUser( -0.5, len(Stage1_Labels)-0.5 )
  return hist

def sumHist( histName, tfs ):
  hsum = tfs[0].Get( histName )
  for i in xrange(1,len(tfs)):
    hsum.Add( tfs[i].Get( histName ) )
  return hsum

def purityHistX( hist ):
  hPurity = hist.Clone()
  for ibin in xrange( 1, hist.GetNbinsX() ):
    NX = 0.0
    for jbin in xrange( 1, hist.GetNbinsY() ):
      NX += hPurity.GetBinContent( ibin, jbin )
    if (NX == 0.): continue
    for jbin in xrange( 1, hist.GetNbinsY() ):
      hPurity.SetBinContent( ibin, jbin, hPurity.GetBinContent( ibin, jbin )/float(NX) )
  return decorateHist( hPurity, "Truth Purity / Reco Category" )


histName = "h2_catSTXS"
procs = ["ggH","VBF","WH","ZH","ttH","bbH","tWH","tHjb"]
tfs = [ TFile("output/Coupling_%s/hist-%s.root" % (p,p)) for p in procs ]
hsum = sumHist( histName, tfs )
hpur = purityHistX( hsum )

# Filter down for a reduced binning
filterList = ['UNKNOWN','_FWDH','GG2HLL','BBH']
def removeBin( binName ):
  for key in filterList:
    if (key in binName): return False
  return True

binsKeep = filter( removeBin, Stage1_Labels ) + ['BBH']
binsMap = { x : i for i, x in enumerate(Stage1_Labels,1) }
binsKeep.insert( 10, binsKeep.pop(0) )
binsKeep.insert( 10, binsKeep.pop(0) )
#for x in binsKeep: print x
#import sys; sys.exit()

def rebinHist( origHist, useBins, binsMap ):
  # Reset histogram
  hist = origHist.Clone("purity2D")
  for jbin in xrange(1, hist.GetNbinsY()+1 ):
    hist.GetYaxis().SetBinLabel( jbin, '' )
    for ibin in xrange(1, hist.GetNbinsX()+1 ):
      pass
      #hist.SetBinContent( ibin, jbin, 0.0 )
  hist.GetYaxis().SetRangeUser( 0, len(useBins) )

  # Fill Purities
  for jbin, binName in enumerate(useBins,1):
    origBin = binsMap[binName]
    hist.GetYaxis().SetBinLabel( jbin, binName )
    print binName
    for ibin in xrange( 1, hist.GetNbinsX()+1 ):
      purity = origHist.GetBinContent( ibin, origBin )
      hist.SetBinContent( ibin, jbin, purity )

  return hist

hpur = rebinHist( hpur, binsKeep, binsMap )

can = TCanvas(); can.cd()
can.SetTopMargin(0.02)
can.SetRightMargin(0.13)
can.SetLeftMargin(0.28)
can.SetBottomMargin(0.25)
can.SetGrid()
gStyle.SetGridColor(kGray+1)
hpur.Draw("colz")
hpur.Draw("AXIS SAME")

can.SaveAs("plots/purity.pdf")
can.SaveAs("plots/purity.png")

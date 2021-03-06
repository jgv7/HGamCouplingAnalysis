#!/usr/bin/env python

from ROOT import *
import prettyplots

import sys; sys.dont_write_bytecode = True
import HGamMoriondCatsBDT2 as HG

gROOT.SetBatch(1)
gStyle.SetOptStat(0)
prettyplots.setPalette('rainbow')
#prettyplots.setPalette('sequentialGB')

Stage1_Labels = ['UNKNOWN','GG2H_FWDH','GG2H_VBFTOPO_JET3VETO','GG2H_VBFTOPO_JET3','GG2H_0J','GG2H_1J_PTH_0_60','GG2H_1J_PTH_60_120','GG2H_1J_PTH_120_200','GG2H_1J_PTH_GT200','GG2H_GE2J_PTH_0_60','GG2H_GE2J_PTH_60_120','GG2H_GE2J_PTH_120_200','GG2H_GE2J_PTH_GT200','VBF_QQ2HQQ_FWDH','QQ2HQQ_VBFTOPO_JET3VETO','QQ2HQQ_VBFTOPO_JET3','QQ2HQQ_VH2JET','QQ2HQQ_REST','QQ2HQQ_PTJET1_GT200','QQ2HLNU_FWDH','QQ2HLNU_PTV_0_150','QQ2HLNU_PTV_150_250_0J','QQ2HLNU_PTV_150_250_GE1J','QQ2HLNU_PTV_GT250','QQ2HLL_FWDH','QQ2HLL_PTV_0_150','QQ2HLL_PTV_150_250_0J','QQ2HLL_PTV_150_250_GE1J','QQ2HLL_PTV_GT250','GG2HLL_FWDH','GG2HLL_PTV_0_150','GG2HLL_PTV_GT150_0J','GG2HLL_PTV_GT150_GE1J','TTH_FWDH','TTH','BBH_FWDH','BBH','THJB_FWDH','THJB','TWH_FWDH','TWH']

Long_Labels = [
  #'UNKNOWN',
  #'GG2H_FWDH',
  'ggH (0-jet)',
  'ggH (1-jet, p_{T}^{H} < 60 GeV)', 'ggH (1-jet, 60 #leq p_{T}^{H} < 120 GeV)',
  'ggH (1-jet, 120 #leq p_{T}^{H} < 200 GeV)',
  'ggH (1-jet, p_{T}^{H} #geq 200 GeV)',
  'ggH (#geq 2-jet, p_{T}^{H} < 60 GeV)',
  'ggH (#geq 2-jet, 60 #leq p_{T}^{H} < 120 GeV)',
  'ggH (#geq 2-jet, 120 #leq p_{T}^{H} < 200 GeV)',
  'ggH (#geq 2-jet, p_{T}^{H} #geq 200 GeV)',
  'ggH (VBF-like, 3-jet veto)',
  'ggH (VBF-like, 3-jet)',
  #'VBF_QQ2HQQ_FWDH',
  'qq #rightarrow Hqq (VBF-like, 3-jet veto)',
  'qq #rightarrow Hqq (VBF-like, 3-jet)',
  'qq #rightarrow Hqq (VH)',
  'qq #rightarrow Hqq (rest)',
  'qq #rightarrow Hqq (p_{T}^{j} #geq 200 GeV)',
  #'QQ2HLNU_FWDH',
  'qq #rightarrow Hl#nu (p_{T}^{V} < 150 GeV)',
  'qq #rightarrow Hl#nu (0-jet, 150 #leq p_{T}^{V} < 250 GeV)',
  'qq #rightarrow Hl#nu (#geq 1-jet, 150 #leq p_{T}^{V} < 250 GeV)',
  'qq #rightarrow Hl#nu (p_{T}^{V} #geq 250 GeV)',
  #'QQ2HLL_FWDH',
  'qq #rightarrow Hll (p_{T}^{V} < 150 GeV)',
  'qq #rightarrow Hll (0-jet, 150 #leq p_{T}^{V} < 250 GeV)',
  'qq #rightarrow Hll (#geq 1-jet, 150 #leq p_{T}^{V} < 250 GeV)',
  'qq #rightarrow Hll (p_{T}^{V} #geq 250 GeV)',
  #'GG2HLL_FWDH',
  'gg #rightarrow Hll (p_{T}^{V} < 150 GeV)',
  'gg #rightarrow Hll (0-jet, p_{T}^{V} #geq 150 GeV)',
  'gg #rightarrow Hll (#geq 1-jet, p_{T}^{V} #geq 150 GeV)',
  #'TTH_FWDH',
  'ttH',
  #'THJB_FWDH',
  'tHqb',
  #'TWH_FWDH',
  'tHW',
  #'BBH_FWDH',
  'bbH',
]

CatLabels = HG.CatLabels


# ----------------------------------------------------------------
def decorateHist( hist, ztitle=None ):
  for ibin, catName in enumerate(CatLabels, 1):    hist.GetXaxis().SetBinLabel( ibin, catName )
  for jbin, binName in enumerate(Long_Labels, 1):  hist.GetYaxis().SetBinLabel( jbin, binName )
  hist.GetXaxis().LabelsOption('v')
  hist.GetXaxis().SetTitle('Reco Category')
  hist.GetYaxis().SetTitle('STXS Truth Bin')
  if (ztitle): hist.GetZaxis().SetTitle(ztitle)
  #hist.GetXaxis().SetTitleOffset(2.5)
  #hist.GetYaxis().SetTitleOffset(4.2)
  hist.GetXaxis().SetTitleOffset(3.0)
  hist.GetYaxis().SetTitleOffset(5.3)
  hist.GetZaxis().SetTitleOffset(1.2)
  hist.SetMinimum(0.0)
  hist.GetXaxis().SetRangeUser( 1, len(CatLabels)+1 )
  hist.GetYaxis().SetRangeUser( 0., len(Stage1_Labels))
  return hist


# ----------------------------------------------------------------
def sumHist( histName, tfs ):
  hsum = tfs[0].Get( histName )
  for i in xrange(1,len(tfs)):
    hsum.Add( tfs[i].Get( histName ) )
  return hsum


# ----------------------------------------------------------------
def purityHistX( hist ):
  hPurity = hist.Clone()
  for ibin in xrange( 1, hist.GetNbinsX()+1 ):
    NX = 0.0
    for jbin in xrange( 1, hist.GetNbinsY()+1 ):
      NX += hPurity.GetBinContent( ibin, jbin )
    if (NX == 0.): continue
    for jbin in xrange( 1, hist.GetNbinsY()+1 ):
      hPurity.SetBinContent( ibin, jbin, hPurity.GetBinContent( ibin, jbin )/float(NX) )
  return decorateHist( hPurity, 'Truth Purity / Reco Category' )


# ----------------------------------------------------------------
histName = 'h2_catSTXS'
procs = ['ggH_NNLOPS','VBF_NNPDF','WH_NLO','ZH_NLO','ttH','bbH','tHjb','tHW','ggZH']
tfs = [ TFile('output/HGamCoupling_%s/hist-%s.root' % (p,p)) for p in procs ]
hsum = sumHist( histName, tfs )
hpur = purityHistX( hsum )

# Filter down for a reduced binning
#filterList = ['UNKNOWN','_FWDH','GG2HLL','BBH']
filterList = ['UNKNOWN','_FWDH','BBH']
def removeBin( binName ):
  for key in filterList:
    if (key in binName): return False
  return True

binsKeep = filter( removeBin, Stage1_Labels ) + ['BBH']
binsMap = { x : i for i, x in enumerate(Stage1_Labels,1) }
binsKeep.insert( 10, binsKeep.pop(0) )
binsKeep.insert( 10, binsKeep.pop(0) )


# ----------------------------------------------------------------
def rebinHist( origHist, useBins, binsMap ):
  # Reset histogram
  hist = origHist.Clone('purity2D')
  for jbin in xrange(1, hist.GetNbinsY()+1 ):
    hist.GetYaxis().SetBinLabel( jbin, '' )
    for ibin in xrange(1, hist.GetNbinsX()+1 ):
      hist.SetBinContent( ibin, jbin, 0.0 )
      #pass
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


# ----------------------------------------------------------------

hpur = rebinHist( hpur, binsKeep, binsMap )

print ''
for jbin, binName in enumerate(Long_Labels):
  print hpur.GetYaxis().GetBinLabel(jbin+1), '    ', binName
  hpur.GetYaxis().SetBinLabel(jbin+1, binName)

can = TCanvas('can','', 1200, 900)
#can.SetMargin(0.26, 0.14, 0.17, 0.08)
can.SetMargin(0.32, 0.14, 0.20, 0.08)
can.SetGrid()
can.cd()

gridColor = kGray+1
gStyle.SetGridColor(gridColor)
hpur.GetXaxis().SetAxisColor(gridColor)
hpur.GetYaxis().SetAxisColor(gridColor)
hpur.GetXaxis().SetLabelSize(0.035)
hpur.GetYaxis().SetLabelSize(0.035)
hpur.Draw('colz')
hpur.Draw('AXIS SAME')

paves = {}
paves[ 'ggH'] = TPave(  1,  0, 11,  9)
paves[ 'qqH'] = TPave( 11,  9, 18, 16)
paves[  'VH'] = TPave( 18, 16, 23, 27)
paves['ggZH'] = TPave( 18, 20, 23, 24)
paves['ggZH'].SetLineStyle(2)
paves[ 'ttH'] = TPave( 23, 27, 32, 30)
for group in paves:
  pave = paves[group]
  pave.SetBorderSize(1)
  pave.SetFillStyle(0)
  pave.SetLineWidth(3)
  pave.Draw()

tl = TLatex()
tl.SetNDC()
tl.DrawLatex(0.3, 0.94, '#bf{#it{#bf{ATLAS}} Internal}')
tl.SetTextSize(0.035)
tl.DrawLatex(0.55, 0.94, '#bf{#it{H #rightarrow #gamma#gamma, m_{H} = 125.09} GeV}')

can.SaveAs('plots/purity_2D.pdf')
can.SaveAs('plots/purity_2D.png')


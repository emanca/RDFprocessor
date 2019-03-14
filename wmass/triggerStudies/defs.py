import os
import copy
import math
import argparse
import sys
sys.path.append('../../framework')
import ROOT
from RDFtreeV2 import RDFtree

from triggerStudies import *

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

parser = argparse.ArgumentParser("")
parser.add_argument('-tag', '--tag', type=str, default="TEST",      help="")
parser.add_argument('-dataYear', '--dataYear',type=int, default=2016, help="")
args = parser.parse_args()
tag = args.tag
dataYear = args.dataYear
print "tag =", bcolors.OKGREEN, tag, bcolors.ENDC, \
    ", dataYear =", bcolors.OKGREEN, str(dataYear), bcolors.ENDC

ROOT.ROOT.EnableImplicitMT()
ROOT.gInterpreter.ProcessLine('''
  gErrorIgnoreLevel = kWarning;
  ''')


def filterVariables(variables={}, selection='Signal', verbose=False):
    if verbose: print '>>', variables
    new_variables = copy.deepcopy(variables)
    delete_vars = []
    for ivar,var in new_variables.iteritems():
        match = False
        appliesTo = var['appliesTo']
        for applyTo in appliesTo:
            if applyTo[-1]=="*":
                if applyTo[0:-1] in selection: 
                    match = True
            else:
                if applyTo==selection:  match = True                
        if not match: 
            delete_vars.append(ivar)
    for ivar in delete_vars: 
        del new_variables[ivar]  
    if verbose: print '<<', new_variables
    return new_variables

selections = {
    'SignalAll' : {
        'MC' : {
            'cut': \
                'Vtype==0 && ' + \
                'MET_filters==1 && ' + \
                'nVetoElectrons==0 && ' + \
                '1',            
            'weight' : \
                'puWeight'
            },
        },
    'SignalPass' : {
        'MC' : {
            'cut': \
                'Vtype==0 && ' + \
                'HLT_SingleMu24 && '+ \
                'MET_filters==1 && ' + \
                'nVetoElectrons==0 && ' + \
                '1',            
            'weight' : \
                'puWeight'
            },
        }
}        

myselections = {}
for cut in ['SignalAll','SignalPass']:
    myselections['%sPlus' % cut]  = copy.deepcopy(selections['%s' % cut])
    myselections['%sMinus' % cut] = copy.deepcopy(selections['%s' % cut])
    for d in ['MC']:
        myselections['%sPlus' % cut][d]['cut']    += ' && Muon_charge[Idx_mu1]>0'
        myselections['%sMinus' % cut][d]['cut']   += ' && Muon_charge[Idx_mu1]<0'

# respect nanoAOD structure: Collection_modifiers_variable
variables =  {        
    'Muon1': { 
        'appliesTo' : ['Signal*'],
        'inputCollection' : 'Muon',
        'newCollection': 'SelectedMuon', 
        'modifiers': '', 
        'index': 'Idx_mu1',
        'variables': { 
            'pt:eta': ('muon pt',  80, 20, 100, 'muon eta', 50, -2.5, 2.5),
            },
    },
}

inputDir = '/gpfs/ddn/srm/cms/store/user/bianchi//NanoAOD%s-%s/' % (str(dataYear), tag)
outDir =  'NanoAOD%s-%s/' % (str(dataYear), tag)

if not os.path.isdir(outDir): os.system('mkdir '+outDir)

# available filed
samples = os.listdir(inputDir)

samples_dict = {
    'WJets' : {
        'dir' : ['WJetsToLNu_TuneCUETP8M1_13TeV-madgraphMLM-pythia8/RunIISummer16NanoAODv4-PUMoriond17_Nano14Dec2018_102X_mcRun2_asymptotic_v6_ext2-v1/190314_093834/0000/'],
        'xsec' : 61526.7,
        'subsel' : {'MCmatched' : " && Muon_genPartIdx[Idx_mu1]>=0"}
        }
    }

outputFiles = []
for sample_key, sample in samples_dict.iteritems():
    dataType = 'MC' if 'Run' not in sample_key else 'DATA'

    print 'Analysing sample', bcolors.OKBLUE, sample_key, bcolors.ENDC
    print '\tdirectories =', bcolors.OKBLUE, sample['dir'] , bcolors.ENDC
    print '\txsec = '+'{:0.3f}'.format(sample['xsec'])+' pb', \
        ', (data type is', bcolors.OKBLUE, dataType, bcolors.ENDC, ')'
    print '\tsubselections =', bcolors.OKBLUE, sample['subsel'] , bcolors.ENDC

    inputFile = ROOT.std.vector("std::string")()
    for x in sample['dir']: inputFile.push_back(inputDir+x+"/tree_11.root")
    p = RDFtree(outputDir=outDir, inputFile=inputFile  )
    
    # create branches
    for subsel_key, subsel in sample['subsel'].iteritems(): 
        outputFiles.append("%s%s" % (sample_key, ('_'+subsel_key if subsel_key!='none' else '')) )
        for sel_key, sel in myselections.iteritems():
            if len(sample['subsel'])>1 and subsel_key=='none': continue
            myvariables = filterVariables(variables, sel_key)
            print '\tBranching: subselection', bcolors.OKBLUE, subsel_key, bcolors.ENDC, 'with selection' , bcolors.OKBLUE, sel_key, bcolors.ENDC
            print '\tAdding variables for collections', bcolors.OKBLUE, myvariables.keys(), bcolors.ENDC
            outputFile = "%s%s_%s.root" % (sample_key, ('_'+subsel_key if subsel_key!='none' else ''), sel_key) 
            myselection = copy.deepcopy(sel)
            myselection[dataType]['cut'] += subsel if subsel_key!='none' else ''
            p.branch(nodeToStart='input',
                     nodeToEnd='triggerStudies'+sel_key,
                     outputFile=outputFile,
                     modules = [triggerStudies(selections=myselection, variables=myvariables,  dataType=dataType, xsec=sample['xsec'], inputFile=inputFile)])

    p.getOutput()

















































import math 
import ROOT

import sys
sys.path.append('../../framework')
from module import *

class triggerStudies(module):
   
    def __init__(self, selections, variables, dataType, xsec, inputFile):
        
        # TH lists
        self.myTH1 = []
        self.myTH2 = []
        self.myTH3 = []

        # MC or DATA
        self.selections = selections
        self.variables = variables 
        self.dataType = dataType
        self.xsec = xsec

        self.inputFile = inputFile

    def run(self,d):

        RDF = ROOT.ROOT.RDataFrame
        runs = RDF('Runs', self.inputFile)

        genEventSumw = runs.Sum("genEventSumw").GetValue()
        print 'genEventSumw : '+'{:1.1f}'.format(genEventSumw)+' weighted events'
        print 'xsec         : '+'{:1.1f}'.format(self.xsec)+' pb'
        print 'lumiweight   : '+'{:1.8f}'.format((1.*self.xsec)/genEventSumw)+' (|Generator_weight| not accounted for)'

        self.d = d.Filter(self.selections[self.dataType]['cut'])


        self.d = self.d.Define('lumiweight', '({L}*{xsec})/({genEventSumw})'.format(L=1.0, genEventSumw = genEventSumw, xsec = self.xsec)) \
            .Define('totweight', 'lumiweight*genWeight*{}'.format(self.selections[self.dataType]['weight']))


        # loop over variables
        for Collection,dic in self.variables.iteritems():
            collectionName = ''
            if dic.has_key('newCollection') and dic['newCollection'] != '':
                if 'index' not in dic:  continue
                self.d = self.defineSubcollectionFromIndex(dic['inputCollection'], dic['newCollection'], dic['index'], self.d)                 
                collectionName = dic['newCollection']
            else:
                collectionName = dic['inputCollection']
            for var,tools in dic['variables'].iteritems():
                columns = list(self.d.GetDefinedColumnNames())
                var_split = var.split(':')
                h2 = self.d.Histo2D((Collection+'_'+var.replace(':','_'), " ; %s; %s" % (tools[4], tools[0]), tools[5],tools[6], tools[7],  tools[1],tools[2], tools[3]), \
                                        collectionName+'_'+var_split[1], collectionName+'_'+var_split[0], \
                                        'totweight')  
                self.myTH2.append(h2)

        return self.d


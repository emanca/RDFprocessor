from header import *
import os
import copy
import h5py
import numpy as np
from array import array
import ROOT
ROOT.gInterpreter.ProcessLine('#include "../RDFprocessor/framework/interface/DataFormat.h"')
ROOT.gInterpreter.ProcessLine('#include "../RDFprocessor/framework/interface/Utility.h"')

class RDFtree:
    def __init__(self, outputDir, outputFile, inputFile,treeName='Events', pretend=False):

        self.outputDir = outputDir # output directory
        self.outputFile = outputFile
        self.inputFile = inputFile
        
        self.treeName = treeName

        RDF = ROOT.ROOT.RDataFrame

        self.pretend = pretend
        if self.pretend:

            ROOT.ROOT.DisableImplicitMT()
            self.d = RDF(self.treeName, self.inputFile)
            self.d=self.d.Range(10)
        else:

            self.d = RDF(self.treeName, self.inputFile)
        
        self.entries = self.d.Count() #stores lazily the number of events
        
        self.modules = []
        
        self.variationsRules = ROOT.map("std::string", "std::vector<std::string>")() #systematic variations

        self.objs = {} # objects to be received from modules
        
        self.node = {} # dictionary branchName - RDF
        self.node['input'] = self.d # assign input RDF to a branch called 'input'

        self.graph = {} # save the graph to write it in the end 

        if not os.path.exists(self.outputDir):
            os.system("mkdir -p " + self.outputDir)
   
        os.chdir(self.outputDir)

        self.fout = ROOT.TFile(self.outputFile, "recreate")
        self.fout.Close()

        os.chdir("..")
        
    def branch(self,nodeToStart, nodeToEnd, modules=[]):

        self.branchDir = nodeToEnd
        if not self.branchDir in self.objs:
            self.objs[self.branchDir] = []
   
        if nodeToStart in self.graph:
            self.graph[nodeToStart].append(nodeToEnd)
        else: 
            self.graph[nodeToStart]=[nodeToEnd]

        branchRDF = self.node[nodeToStart]

        lenght = len(self.modules)

        self.modules.extend(modules)
        
        # modify RDF according to modules

        for m in self.modules[lenght:]: 
            
            m.setVariationRules(self.variationsRules)
            branchRDF = m.run(ROOT.RDF.AsRNode(branchRDF))
            self.variationsRules = m.getVariationRules()

            tmp_th1 = m.getTH1()
            tmp_th2 = m.getTH2()
            tmp_th3 = m.getTH3()

            tmp_th1G = m.getGroupTH1()
            tmp_th2G = m.getGroupTH2()
            tmp_th3G = m.getGroupTH3()
            tmp_thNG = m.getGroupTHN()    

            for obj in tmp_th1:
                    
                value_type = getValueType(obj)

                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))
                
            for obj in tmp_th2:
                    
                value_type = getValueType(obj)

                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))

            for obj in tmp_th3:
                    
                value_type = getValueType(obj)

                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))

            for obj in tmp_th1G:
                    
                value_type = getValueType(obj)

                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))

            for obj in tmp_th2G:
                    
                value_type = getValueType(obj)

                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))

            for obj in tmp_th3G:
                    
                value_type = getValueType(obj)
                
                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))

            for obj in tmp_thNG:

                value_type = getValueType(obj)

                self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(obj))

            m.reset()

        self.node[nodeToEnd] = branchRDF

    def Histogram(self, columns, types, node, histoname, bins, variations):

        d = self.node[node]
        rules = self.variationsRules
        self.branchDir = node
        
        if not len(columns)==len(types): raise Exception('number of columns and types must match')
        nweights = len(columns) - len(bins)

        variations_vec = ROOT.vector(ROOT.vector('string'))()
        # reorder variations to follow column order
        
        for col in columns:
            if col in rules:
                variations_vec.push_back(copy.deepcopy(rules.at(col))) #deepcopy otherwise it gets deleted
                columns.append(variations[col]) # append column containing variations
                types.append('float')
                variations_vec.push_back(ROOT.vector('string')({""}))
            else:
                variations_vec.push_back(ROOT.vector('string')({""}))
        #############################################################
        print("this is how I will make variations for this histogram")
        for icol, col in enumerate(columns):
            print(col, variations_vec[icol])
        #############################################################
        # print("number of weights columns:",nweights)
        ROOT.gSystem.SetIncludePath("-I$ROOTSYS/include -I/scratchnvme/emanca/wproperties-analysis/templateMaker/interface -I/opt/boost/include")
        with open("helperbooker.cpp", "w") as f:                                                                            
            code = bookingCode.format(template_args="{},{},{}".format(len(bins),nweights,', '.join(types)))                                                        
            f.write(code)                                                                                                   
        ROOT.gSystem.CompileMacro("helperbooker.cpp", "gfO")                                                            
                                                                                                                                                                                                    
        histo = ROOT.BookIt(d, histoname, bins, columns,variations_vec) 

        value_type = getValueType(histo)
        self.objs[self.branchDir].append(ROOT.RDF.RResultPtr(value_type)(histo))
        
    
    def Snapshot(self, node, blist=[]):

        opts = ROOT.ROOT.RDF.RSnapshotOptions()
        opts.fLazy = True

        branchList = ROOT.vector('string')()

        for l in blist:
            branchList.push_back(l)

        if not len(blist)==0:
            out = self.node[node].Snapshot(self.treeName,self.outputFile, branchList, opts)
        else:
            out = self.node[node].Snapshot(self.treeName,self.outputFile, "", opts)

        self.objs[self.branchDir].append(out)

    def getROOTOutput(self):

        #start analysis
        self.start = time.time()

        # now write all the outputs together

        print("Writing output files in "+ self.outputDir)

        os.chdir(self.outputDir)
        self.fout = ROOT.TFile(self.outputFile, "update")
        self.fout.cd()

        obj_number = 0

        for branchDir, objs in self.objs.items():

            if objs==[]: continue #get rid of empty nodes
            if not self.fout.GetDirectory(branchDir): self.fout.mkdir(branchDir)

            self.fout.cd(branchDir)
            
            for obj in objs:
                
                if not 'TH' in type(obj).__cpp_name__:
                    continue
                elif 'vector' in type(obj).__cpp_name__:
                    
                    for h in obj:
                        obj_number =  obj_number+1
                        h.Write()
                else:
                    obj_number =  obj_number+1
                    obj.Write()

        
        #self.objs = {} # re-initialise object list
        self.fout.Close()
        os.chdir("..")

        print(self.entries.GetValue(), "events processed in "+"{:0.1f}".format(time.time()-self.start), "s", "rate", self.entries.GetValue()/(time.time()-self.start), "histograms written: ", obj_number)

    def gethdf5Output(self,branchDirs=None):

        #start analysis
        self.start = time.time()

        if branchDirs is None:
            branchDirs = list(self.objs.keys())
        os.chdir(self.outputDir)
        with h5py.File(self.outputFile.replace('root','hdf5'), mode="w") as f:
            dtype = 'float64'
            for branchDir, objs in self.objs.items():
                if objs == []: continue
                if not branchDir in branchDirs: continue #only write selected folders
                for obj in objs:
                    if not 'TH' in type(obj).__cpp_name__: #writing boost histograms
                        map = obj.GetValue()
                        for name,h in map:
                            print(name)
                            arr = ROOT.convert(h)
                            counts = np.asarray(arr[0])
                            sumw = np.asarray(arr[1])
                            dset = f.create_dataset('{}'.format(name), [counts.shape[0]], dtype=dtype)
                            dset[...] = counts
                            dset2 = f.create_dataset('{}_sumw2'.format(name), [counts.shape[0]], dtype=dtype)
                            dset2[...] = sumw
                    elif 'vector' in type(obj).__cpp_name__:
                        for h in obj:
                            nbins = h.GetNbinsX()*h.GetNbinsY() * h.GetNbinsZ()
                            dset = f.create_dataset('{}'.format(h.GetName()), [nbins], dtype=dtype)
                            harr = np.array(h)[1:-1].ravel().astype(dtype) #no under/overflow bins
                            dset[...] = harr
                            #save sumw2
                            if not h.GetSumw2().GetSize()>0: continue 
                            sumw2_hist = h.Clone()
                            dset2 = f.create_dataset('{}_sumw2'.format(h.GetName()), [nbins], dtype=dtype)
                            sumw2f=[sumw2_hist.GetSumw2()[i] for i in range(sumw2_hist.GetSumw2().GetSize())]
                            sumw2f = np.array(sumw2f,dtype='float64').ravel().astype(dtype)
                            dset2[...] = sumw2f
                    else:
                        nbins = obj.GetNbinsX()*obj.GetNbinsY() * obj.GetNbinsZ()
                        dset = f.create_dataset('{}'.format(obj.GetName()), [nbins], dtype=dtype)
                        harr = np.array(h)[1:-1].ravel().astype(dtype) #no under/overflow bins
                        dset[...] = harr
                        #save sumw2
                        if not obj.GetSumw2().GetSize()>0: continue 
                        sumw2_hist = obj.Clone()
                        dset2 = f.create_dataset('{}_sumw2'.format(obj.GetName()), [nbins], dtype=dtype)
                        sumw2f=[sumw2_hist.GetSumw2()[i] for i in range(sumw2_hist.GetSumw2().GetSize())]
                        sumw2f = np.array(sumw2f,dtype='float64').ravel().astype(dtype)
                        dset2[...] = sumw2f
        
        print(self.entries.GetValue(), "events processed in "+"{:0.1f}".format(time.time()-self.start), "s", "rate", self.entries.GetValue()/(time.time()-self.start))
        os.chdir("..")

    def getObjects(self):

        return self.objs

    def saveGraph(self):

        print(self.graph)

        from graphviz import Digraph

        dot = Digraph(name='my analysis', filename = 'graph.pdf')

        for node, nodelist in self.graph.items():

            dot.node(node, node)
            for n in nodelist:
                dot.node(n, n)
                dot.edge(node,n)

        dot.render()  

    def EventFilter(self,nodeToStart, nodeToEnd, evfilter, filtername):
        if not nodeToEnd in self.objs:
            self.objs[nodeToEnd] = []
   
        if nodeToStart in self.graph:
            self.graph[nodeToStart].append(nodeToEnd)
        else: 
            self.graph[nodeToStart]=[nodeToEnd]

        branchRDF = self.node[nodeToStart]
        branchRDF = ROOT.RDF.AsRNode(ROOT.RDF.AsRNode(branchRDF).Filter(evfilter, filtername))
        self.node[nodeToEnd] = branchRDF


    def getCutFlowReport(self):
        return self.d.Report()

    def displayColumn(self, node, columname, nrows=10):
        if node not in self.graph : 
            print("Node {} does not exist! Skipping display!".format(node))
            return -1
        self.node[node].Display(columname, nrows).Print()

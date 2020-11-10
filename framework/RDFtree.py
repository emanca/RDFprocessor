from header import *
import copy
import h5py
import numpy as np
from root_numpy import hist2array

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
            self.d=self.d.Range(100)
        else:

            self.d = RDF(self.treeName, self.inputFile)
        
        self.entries = self.d.Count() #stores lazily the number of events
        
        self.modules = []

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

        for i, m in enumerate(self.modules[lenght:]): 
            
            branchRDF = m.run(ROOT.RDF.AsRNode(branchRDF))

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


    def takeSnapshot(self, node, blist=[]):

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
                    

    def getOutput(self):

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
                    obj.GetValue()
    
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

    def gethdf5Output(self):

        with h5py.File(self.outputFile.replace('root','hdf5'), "w") as f:
            dtype = 'float64'
            for branchDir, objs in self.objs.items():
                if objs == []: continue
                for obj in objs:
                    print(type(obj).__cpp_name__)
                    if not 'TH' in type(obj).__cpp_name__: continue #skipping eventual TTree writing
                    elif 'vector' in type(obj).__cpp_name__:
                        for h in obj:
                            nbins = h.GetNbinsX()*h.GetNbinsY() * h.GetNbinsZ()
                            dset = f.create_dataset('{}/{}'.format(branchDir,h.GetName()), [nbins], dtype=dtype)
                            harr = hist2array(h, include_overflow=False).ravel().astype(dtype)
                            dset[...] = harr
                    else:
                        nbins = obj.GetNbinsX()*obj.GetNbinsY() * obj.GetNbinsZ()
                        dset = f.create_dataset('{}/{}'.format(branchDir,obj.GetName()), [nbins], dtype=dtype)
                        harr = hist2array(obj, include_overflow=False).ravel().astype(dtype)
                        dset[...] = harr

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


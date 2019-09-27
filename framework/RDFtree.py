from header import *
import copy

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

        #start analysis
        self.start = time.time()
        
    def branch(self,nodeToStart, nodeToEnd, modules=[]):

        self.branchDir = nodeToEnd
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

            branchRDF = m.run(CastToRNode(branchRDF))

                
            tmp_th1 = m.getTH1()
            tmp_th2 = m.getTH2()
            tmp_th3 = m.getTH3()

            tmp_th1G = m.getGroupTH1()
            tmp_th2G = m.getGroupTH2()
            tmp_th3G = m.getGroupTH3()
                

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


            m.reset()

        self.node[nodeToEnd] = branchRDF


    def takeSnapshot(self, blist=[]):

        opts = ROOT.ROOT.RDF.RSnapshotOptions()
        opts.fLazy = True

        branchList = ROOT.vector('string')()

        for l in blist:
            branchList.push_back(l)

        print time.time()-self.start, "before snapshot"

        if not len(blist)==0:
            out = self.d.Snapshot(self.treeName,self.outputFile[i], branchList, opts)
        else:
            out = self.d.Snapshot(self.treeName,self.outputFile[i], "", opts)
        # dummy histogram to trigger snapshot

        h = self.d.Define("foo", "1").Histo1D("foo")    
        self.objs.append(ROOT.RDF.RResultPtr('TH1D')(h))
                    

    def getOutput(self):

        # now write all the outputs together

        print "Writing output files in "+ self.outputDir

        os.chdir(self.outputDir)
        self.fout = ROOT.TFile(self.outputFile, "update")
        self.fout.cd()
    
        obj_number = 0

        for branchDir, objs in self.objs.iteritems():

            if not self.fout.GetDirectory(branchDir): self.fout.mkdir(branchDir)
            
            self.fout.cd(branchDir)
            for obj in objs:
                
                if 'vector' in obj.__cppname__:
                    
                    print "writing group of histos "
                    
                    for h in obj:
                        obj_number =  obj_number+1
                        print h.GetName()
                        h.Write()
                else:
                    obj_number =  obj_number+1
                    obj.Write()

        
        self.objs = {} # re-initialise object list
        self.fout.Close()
        os.chdir("..")

        print self.entries.GetValue(), "events processed in "+"{:0.1f}".format(time.time()-self.start), "s", "rate", self.entries.GetValue()/(time.time()-self.start), "histograms written: ", obj_number

    def saveGraph(self):

        from graphviz import Digraph

        dot = Digraph(name='my analysis', filename = 'graph.pdf')

        for node, nodelist in self.graph.iteritems():
            for n in nodelist:
                
                dot.node(node, n)


        print(dot.source)  

        #dot.render(view=True)  


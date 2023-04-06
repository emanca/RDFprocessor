from header import *
import os
import copy
import pickle
import gzip
import narf
from narf.lumitools import make_lumihelper, make_jsonhelper
import hist
import lz4.frame
import numpy as np
from array import array
import ROOT
ROOT.gInterpreter.ProcessLine(".O3")

class RDFtree:
    def __init__(self,d):
        
        self.modules = []
        
        self.objs = [] # objects to be received from modules
        self.d=d

        self.node = {} # dictionary branchName - RDF
        self.node['input'] = self.d # assign input RDF to a branch called 'input'

        self.graph = {} # save the graph to write it in the end 

    def branch(self,nodeToStart, nodeToEnd, modules=[]):
   
        if nodeToStart in self.graph:
            self.graph[nodeToStart].append(nodeToEnd)
        else: 
            self.graph[nodeToStart]=[nodeToEnd]

        branchRDF = self.node[nodeToStart]

        lenght = len(self.modules)

        self.modules.extend(modules)
        
        # modify RDF according to modules

        for m in self.modules[lenght:]: 
            print(type(branchRDF).__cpp_name__)
            branchRDF = m.run(ROOT.RDF.AsRNode(branchRDF))

        self.node[nodeToEnd] = branchRDF

    def Histogram(self, node, name, cols, axes, tensor_axes=''):

        d = self.node[node]
        self.branchDir = node

        if tensor_axes=='':
            self.objs.append(d.HistoBoost(name, axes, cols))
        else:
            self.objs.append(d.HistoBoost(name, axes, cols, tensor_axes=tensor_axes))

    def EventCount(self,node, column):

        d = self.node[node]
        return d.SumAndCount(column)

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

        self.objs.append(out)

    def getObjects(self):
        return self.objs

    def saveGraph(self):

        ROOT.RDF.SaveGraph(self.node['input'],"graph.pdf")
        # print(self.graph)

        # from graphviz import Digraph

        # dot = Digraph(name='my analysis', filename = 'graph.pdf')

        # for node, nodelist in self.graph.items():

        #     dot.node(node, node)
        #     for n in nodelist:
        #         dot.node(n, n)
        #         dot.edge(node,n)

        # dot.render()  

    def EventFilter(self,nodeToStart, nodeToEnd, evfilter, filtername):
        
        if nodeToStart in self.graph:
            self.graph[nodeToStart].append(nodeToEnd)
        else: 
            self.graph[nodeToStart]=[nodeToEnd]

        branchRDF = self.node[nodeToStart]
        branchRDF = ROOT.RDF.AsRNode(ROOT.RDF.AsRNode(branchRDF).Filter(evfilter, filtername))
        self.node[nodeToEnd] = branchRDF

    def getCutFlowReport(self, node):
        return self.node[node].Report()

    def displayColumn(self, node, columnList=[], nrows=1000000):
        print("careful: this is triggering the event loop!")
        if node not in self.node:
            print("Node {} does not exist! Skipping display!".format(node))
            return -1
        columnVec = ROOT.vector('string')(columnList)
        self.node[node].Display(columnVec, nrows).Print()

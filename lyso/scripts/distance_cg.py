#!/usr/bin/env python3

import argparse
import multiprocessing as mp
import sys
from argparse import ArgumentTypeError as ArgTypeErr
from pathlib import Path
import networkx as nx
import pydot

DOT_DIR_NAME = "dot-files"


def find_nodes (name, graph):

    return [n[0] for n in graph.nodes(data=True) if name == n[0]]


def node_name (name):
    return "\"{%s}\"" % name

def find_nodes_label(name, graph):
  n_name = node_name (name)
  return [n for n, d in graph.nodes(data=True) if n_name in d.get('label', '')]


def cg_weight_distance(src_fun, graph):
    
    for t in ftargets:
        try:
            shortest = nx.dijkstra_path_length(graph, src_fun, t, weight='weight')
            if (shortest != None):
                cg_weight.append([[int(src_fun), int(t)], float(shortest)])
                cg_weight_rev.append([[int(t), int(src_fun)], float(shortest)])  


        #if the node is not found, it means the node has no outgoing edges at all 
        except nx.NodeNotFound:
            pass

        except nx.NetworkXNoPath: 
            pass


def bb_distance(entry, callsite, graph):
    
   try:
       return  nx.shortest_path_length(graph, entry, callsite)
   except  nx.NetworkXNoPath:
       pass

def is_path_to_dir(path):
    """Returns Path object when path is an existing directory"""
    p = Path(path)
    if not p.exists():
        raise ArgTypeErr("path doesn't exist")
    if not p.is_dir():
        raise ArgTypeErr("not a directory")
    return p


##################################
# Main function
##################################
if __name__ == '__main__':
  parser = argparse.ArgumentParser ()
  parser.add_argument ('-d', '--dot', type=str, required=True, help="Path to dot-file representing the graph.")
  parser.add_argument ('-f', '--ftargets', type=str, required=True, help="Path to file specifying Target nodes.")
  parser.add_argument ('-b', '--btargets', type=str, required=True, help="Path to file specifying Target nodes.")
  parser.add_argument ('-w', '--weight_out', type=str, required=True, help="Path to output file containing weighed callgraph distance.")
  parser.add_argument ('-r', '--rev_weight_out', type=str, required=True, help="Path to output file containing weighed callgraph distance.")

  parser.add_argument ('-o', '--distance_out', type=str, required=True, help="Path to output file containing distance between funtion target to its bb targets.")
  parser.add_argument ('-n', '--fnames', type=str, required=True, help="Path to file containing name for each node.")
  parser.add_argument ('-s', '--cg_callsites', type=str, help="Path to file containing mapping between basic blocks and called functions.")
  parser.add_argument("temporary_directory", metavar="temporary-directory",
                        type=is_path_to_dir,
                        help="Directory where dot files and target files are "
                             "located")


  args = parser.parse_args ()
  dot_files = args.temporary_directory / DOT_DIR_NAME


  

  print ("\nParsing %s .." % args.dot)
  CG = nx.DiGraph(nx.drawing.nx_pydot.read_dot(args.dot))
  
  #Draw the call graph
  '''
  callgraph_pdot = nx.drawing.nx_pydot.to_pydot(CG)
  graph_name = "call_graph.png"
  callgraph_pdot.write_png(graph_name)
  '''

  #Callgraph weight, [source_fun_id, dest_fun_id]->[weight]
  cg_fun_pair = []
  cg_fun_distance = []
  #{"fun_name": fun_id}
  fun2index = {}
  #[bb_id, "fun_name"]
  bb2fun = []
  
  bb_targets = []
  #[target_fun_id, bb_id]
  targetfun2bb_pair = []
  targetfun2bb_distance = []

  print("Loading BB targetis..")
  with open(args.btargets, 'r') as f:
      for l in f.readlines():
          s = l.strip()
          bb_targets.append(s)

  #Populate fun2index
  with open(args.fnames, 'r') as f:
      for l in f.readlines():
          s = l.strip().split(':')[1].split(',')
          fun_name = s[0]
          fun_id = s[1]
          fun2index[fun_name] = fun_id

  #Populate bb2fun 
  with open(args.cg_callsites, 'r') as f:
      for l in f.readlines():
          s = l.strip().split(",")
          bb_id = s[0]
          fun_name = s[1]
          pair = [bb_id, fun_name]
          bb2fun.append(pair)
   
  print ("Loading Fun targets..")
  with open(args.ftargets, "r") as f:
      ftargets = []
      for line in f.readlines():
          line = line.strip()
          for target in find_nodes(line, CG):
              ftargets.append(target)
  if (not ftargets):
      print("No targets available")
      exit(0)

  #We check if all the target are reachable from main function
  unreachable_targets = []
  for target in ftargets:
     try:
       main = find_nodes_label('main', CG)
       for t in main:
          nx.shortest_path_length(CG, t, target)

     except  nx.NetworkXNoPath:
        unreachable_targets.append(target)

  #if len(unreachable_targets) == len(ftargets):
  #   print("These targets are unable to reach from main, please change your targets")
  #   print(unreachable_targets)
  #   exit(0)
 

  print("Computing fun edge weight for the call graph (this might take a while)")
  for cfg in dot_files.glob("cfg.*.dot"):
      if cfg.stat().st_size == 0: continue 
      if cfg.name.split('.')[2] != 'dot':
         caller = cfg.name.split('.')[1] + '.' + cfg.name.split('.')[2]
      else:
         caller = cfg.name.split('.')[1]
      
      
      if not find_nodes_label(caller, CG): continue
      
      
      print("Loading cfg for function '%s'.." %caller)
      
      
      CFGraph = nx.DiGraph(nx.drawing.nx_pydot.read_dot(cfg))

      #get the entry node from the graph
      entry_node = list(CFGraph.nodes)[0]
      #for node in CFGraph.nodes:
      #check if the node is in BBcalls.txt
      for bb2fun_pair in bb2fun:
          if find_nodes(bb2fun_pair[0], CFGraph):
                #Calculate the distance between entry node and the callsite
                distance = bb_distance(entry_node, bb2fun_pair[0], CFGraph)
                if (distance != None):
                    if caller in fun2index and bb2fun_pair[1] in fun2index:
                       source_fun_id = fun2index[caller]
                       dest_fun_id = fun2index[bb2fun_pair[1]]
                       pair = [source_fun_id, dest_fun_id]
                       if pair not in cg_fun_pair:
                          cg_fun_pair.append(pair)
                          cg_fun_distance.append(float(distance))
                       else:
                           #There could be some duplicate in fun_weight_pair
                           #E.g., [[b,e],2] and [[b,e],4], we use the minimal distance
                           index = cg_fun_pair.index(pair)
                           if cg_fun_distance[index] > distance:
                              cg_fun_distance[index] = distance
         
      #calculate the distance between bb target and entry node
      for bbtarget in bb_targets:
          if find_nodes(bbtarget, CFGraph):
             distance = bb_distance(entry_node, bbtarget, CFGraph) 
             if (distance != None):
                if caller in fun2index:
                   target_fun_id = fun2index[caller]
                   pair = [target_fun_id, bbtarget]
                   targetfun2bb_pair.append(pair)
                   targetfun2bb_distance.append(float(distance))   
  
  with open(args.distance_out, "w") as distance_out:
       for i in range(len(targetfun2bb_pair)):
           distance_out.write(targetfun2bb_pair[i][0] + "," + targetfun2bb_pair[i][1] + "," + str(targetfun2bb_distance[i]) + "\n")


  if len(cg_fun_pair) != len(cg_fun_distance):
     print ("The length of fun pair and distance are not equal")
     exit(0)


           

  print("Build the function weight graph")
  
  Weight_G = nx.DiGraph()
  for i in range(len(cg_fun_pair)):
    Weight_G.add_edge(cg_fun_pair[i][0], cg_fun_pair[i][1], weight=cg_fun_distance[i], label=cg_fun_distance[i])
  #print(Weight_G.edges.data())


  #output the weight graph dot      
  
  '''
  nx.drawing.nx_pydot.write_dot(Weight_G, "weight_callgraph.dot")
  pdot = nx.drawing.nx_pydot.to_pydot(Weight_G)  
  graph_name = "weight_graph.png"
  pdot.write_png(graph_name)
  '''
  

  cg_weight = []
  cg_weight_rev = [] 
  print ("Calculating cg distance..") 
  with open(args.fnames, "r") as f:   
     for line in f.readlines():
         fun_id = line.strip().split(",")[1]
         cg_weight_distance(fun_id, Weight_G)
  
  with open(args.weight_out, "w") as weight_out, open(args.rev_weight_out, "w") as rev_weight_out:
       cg_weight.sort()
       cg_weight_rev.sort()

       for item in cg_weight:
           weight_out.write(str(item[0][0]) + "," + str(item[0][1]) + "," + str(item[1]) + "\n")
       
       for item in cg_weight_rev:
           rev_weight_out.write(str(item[0][0]) + "," + str(item[0][1]) + "," + str(item[1]) + "\n")

  

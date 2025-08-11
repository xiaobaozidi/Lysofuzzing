#!/usr/bin/env python3

import argparse
import collections
import functools
import networkx as nx
import logging


def find_nodes (name, graph):

    return [n[0] for n in graph.nodes(data=True) if name == n[0]]


def find_bb(bb_id, pair):
     for n in pair: 
        if bb_id == n[0]:
           return True

##################################
# Calculate Distance
##################################
def cfg_distance (bb_id, graph, edge_pair, edge_distance, tmp_edge_pair, tmp_edge_distance):
    if find_nodes(bb_id, graph):
              if not find_bb(bb_id, edge_pair): 
                 for i in range(len(edge_pair)):
                     try:
                        #calculate the distance between BBs in the graph and joint BB
                        shortest = nx.dijkstra_path_length(graph, bb_id, edge_pair[i][0])
                        if shortest != None:
                           distance = shortest + edge_distance[i]
                           pair = [bb_id, edge_pair[i][1]]
                           if pair not in tmp_edge_pair:                              
                              tmp_edge_pair.append(pair)
                              tmp_edge_distance.append(distance)
                           else:
                              #There could be some duplicate in tmp_bb_fun_pair
                              #E.g., [[b.2,e],2] and [[b.2,e],4], we use the minimal distance
                              index = tmp_edge_pair.index(pair)
                              if tmp_edge_distance[index] > distance:
                                 tmp_edge_distance[index] = distance
 
                     except nx.NetworkXNoPath:
                        pass
		 

def intra_distance(bb_id, target_id, graph):               
    try:
       return nx.shortest_path_length(graph, bb_id, target_id)
    except nx.NetworkXNoPath:
       pass
 


##################################
# Main function
##################################
if __name__ == '__main__':

  logging.basicConfig(level=logging.DEBUG, filename='output.log')
  logger = logging.getLogger(__name__)

  parser = argparse.ArgumentParser ()
  parser.add_argument ('-d', '--dot', type=str, required=True, help="Path to dot-file representing the graph.")
  parser.add_argument ('-o', '--out', type=str, required=True, help="Path to output file containing distance for each node.")
  parser.add_argument ('-b', '--bbnames', type=str, required=True, help="Path to file containing BB name for each node.")
  parser.add_argument ('-f', '--fnames', type=str, required=True, help="Path to file containing Fun name for each node.")
  parser.add_argument ('-c', '--cg_distance', type=str, help="Path to file containing call graph distance.")
  parser.add_argument ('-s', '--cg_callsites', type=str, help="Path to file containing mapping between basic blocks and called functions.")
  parser.add_argument ('-t', '--fun2bb_distance', type=str, help="Path to output file containing distance between funtion target to its bb targets.")


  args = parser.parse_args ()
  print ("\nParsing %s .." % args.dot)
  CFG = nx.DiGraph(nx.drawing.nx_pydot.read_dot(args.dot))

  # Process as ControlFlowGraph
  caller = ""
  #{"fun_name": fun_id}
  fun2index = {}
  
  cg_fun_pair = []
  cg_fun_distance = []

  cg_fun_rev= []
 
  bb2targetfun_pair = []
  bb2targetfun_distance = []
  
  #[target_fun_id, bb_id]
  targetfun2targetbb_pair = []
  targetfun2targetbb_distance = []  
 
  
  if args.cg_distance is None:
      print ("Specify file containing CG-level distance (-c).")
      exit(1)

  if args.cg_callsites is None:
      print ("Specify file containing mapping between basic blocks and called functions (-s).")
      exit(1)
   
  if args.fun2bb_distance is None:
      print ("Specify file containing funtarget to its bbtarget distance (-g).")
      exit(0)
  
  #Populate fun2index
  with open(args.fnames, 'r') as f:
      for l in f.readlines():
          s = l.strip().split(':')[1].split(',')
          fun_name = s[0]
          fun_id = s[1]
          fun2index[fun_name] = fun_id
  
  #Populate targetfun2targetbb
  with open(args.fun2bb_distance, 'r') as f:
      for l in f.readlines():
          s = l.strip().split(',')
          target_fun_id = s[0]
          target_bb_id = s[1]
          distance = s[2]
          targetfun2targetbb_pair.append([target_fun_id, target_bb_id]) 
          targetfun2targetbb_distance.append(float(distance)) 
      
  caller = args.dot.split(".")
  caller = caller[len(caller)-2]
  
  print ("Loading cg_distance for function '%s'.." % caller)      
  with open(args.cg_distance, 'r') as f:
       for l in f.readlines():
           s = l.strip().split(',')
           source_fun_id = s[0];
           target_fun_id = s[1];
           fun_distance = s[2];
           cg_fun_rev.append([[target_fun_id, source_fun_id],fun_distance])
           cg_fun_pair.append([source_fun_id, target_fun_id])
           cg_fun_distance.append(float(fun_distance))

  if not cg_fun_pair or not cg_fun_distance:
     print ("Call graph distance file is empty.")
     exit(0)
     
  if len(cg_fun_pair) != len(cg_fun_distance):
       print ("The length of fun pair and distance are not equal")
       exit(0)
      
  
  print ("Building bb to target function pair and distance...")
  with open(args.cg_callsites, 'r') as f:
       for l in f.readlines():
          s = l.strip().split(",")
          bb_id = s[0]
          callee_fun = s[1] 
          if find_nodes(bb_id, CFG):
             for i in range(len(cg_fun_pair)):
                 source_fun_id = cg_fun_pair[i][0]
                 target_fun_id = cg_fun_pair[i][1]
                 fun_distance = cg_fun_distance[i]
                 if callee_fun in fun2index:
                    if fun2index[callee_fun] == source_fun_id:
                       pair = [bb_id, target_fun_id]
                       if pair not in bb2targetfun_pair:
                          bb2targetfun_pair.append(pair)
                          bb2targetfun_distance.append(fun_distance)
                       else:
                          index = bb2targetfun_pair.index(pair)
                          if bb2targetfun_distance[index] > fun_distance: 
                             bb2targetfun_distance[index] = fun_distance
  
  
  tmp_bb2targetfun_pair = []
  tmp_bb2targetfun_distance = []
  print ("Calculating cfg distance..")
  with open(args.bbnames, "r") as f:
    for line in f.readlines():
       cfg_distance(line.strip().split(":")[0], CFG, bb2targetfun_pair, 
                    bb2targetfun_distance, tmp_bb2targetfun_pair, tmp_bb2targetfun_distance)
 
 
  #Merge tmp_bb_fun to bb_fun
  for i in range(len(tmp_bb2targetfun_pair)):
      bb2targetfun_pair.append([tmp_bb2targetfun_pair[i][0], tmp_bb2targetfun_pair[i][1]])
      bb2targetfun_distance.append(tmp_bb2targetfun_distance[i])
  
 
  bb2targetbb_pair = []
  bb2targetbb_distance = []
 
  #Calculate the distance between bb and target bb that is in this CFG
  for pair in targetfun2targetbb_pair:
      if find_nodes(pair[1], CFG):
         for node in CFG.nodes:
             distance = intra_distance(node, pair[1], CFG)  
             if (distance != None):
                bb2targetbb_pair.append([node, pair[1]])
                bb2targetbb_distance.append(float(distance)) 
                  

  #Concatenate the  bb2targetfun distance to targetfun2targetbb distance that is not in this CFG
  for i in range(len(targetfun2targetbb_pair)):
      for j in range(len(bb2targetfun_pair)):
          if targetfun2targetbb_pair[i][0] == bb2targetfun_pair[j][1]:
             pair = [bb2targetfun_pair[j][0], targetfun2targetbb_pair[i][1]]
             distance = bb2targetfun_distance[j] + targetfun2targetbb_distance[i]
             if pair not in bb2targetbb_pair:
                bb2targetbb_pair.append([bb2targetfun_pair[j][0], targetfun2targetbb_pair[i][1]])
                bb2targetbb_distance.append(distance)
             else: 
                index = bb2targetbb_pair.index(pair)
                if bb2targetbb_distance[index] > distance:
                   bb2targetbb_distance[index] = distance   
          
  if len(bb2targetbb_pair) != len(bb2targetbb_distance):
       print ("The length of fun pair and distance are not equal")
       exit(0)

  with open(args.out, "w") as out:
       for i in range(len(bb2targetbb_pair)):
           out.write(bb2targetbb_pair[i][0] + "," + bb2targetbb_pair[i][1] + "," + str(bb2targetbb_distance[i]) + "\n")
          

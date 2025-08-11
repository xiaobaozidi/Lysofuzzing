#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>
#include "types.h"
#include "distance_calculate.h"

#define MAX_DISTANCE 100

using namespace std;
using namespace boost::algorithm;

typedef unordered_map<string, u32> edgeDistanceMap;
typedef unordered_map<u32, u32> bbMap;

edgeDistanceMap funcEdgeDist;
edgeDistanceMap bbEdgeDist;
bbMap bb2bb;


void init_func_distance(char *path){
    std::string line;
    std::ifstream edgeFile(path);
    while (std::getline(edgeFile, line)){
         trim(line);
         std::vector<std::string> splits;
         split(splits, line, is_any_of(","));
         u16 targetFunc = stoi(splits[0]);
         u16 srcFunc = stoi(splits[1]);
         u32 dist = stoi(splits[2]);
         string pair = to_string(targetFunc) + "&" + to_string(srcFunc);
         funcEdgeDist.insert(make_pair(pair, dist));
    }
}

void init_bb_distance(char *path){
    std::string line;
    std::ifstream edgeFile(path);
    while (std::getline(edgeFile, line)){
         trim(line);
         std::vector<std::string> splits;
         split(splits, line, is_any_of(","));
         u16 targetBB = stoi(splits[0]);
         u16 srcBB = stoi(splits[1]);
         u32 dist = stoi(splits[2]);
         string pair = to_string(targetBB) + "&" + to_string(srcBB);
         bbEdgeDist.insert(make_pair(pair, dist));
    }

}

u32 init_bb2bb_map(char *path){
   std::string line;
   std::ifstream bb2bbFile(path);
   u32 total_target = 0;
   while (std::getline(bb2bbFile, line)){
        total_target++;
        trim(line);
        std::vector<std::string> splits;
        split(splits, line, is_any_of(","));
        u16 originBB = stoi(splits[0]);
        u16 newBB = stoi(splits[1]);
        bb2bb.insert(make_pair(newBB, originBB));

   }
   return total_target;
}



u32 get_shortest_fun_distance(u8* bitmap, u32 len, u32 target){
      u32 shortest_distance = MAX_DISTANCE;
      u8* temp = bitmap;
      for (int i=0; i<len; i++){
          if (temp[i] != 0){
             string pair = to_string(target) + "&" + to_string(i);
             if (funcEdgeDist.find(pair)!= funcEdgeDist.end()){
                u32 distance = funcEdgeDist[pair];
                if (shortest_distance > distance)
                   shortest_distance = distance;
             }

          }
      }
      return shortest_distance;
}

u32 get_total_bb_distance(u8* bitmap, u32 len, u32 target){
      
      u32 total_distance = 0;
      u8* temp = bitmap;
      for (int i = 0; i < len; i++){
          if (temp[i] != 0){
             string pair = to_string(target) + "&" + to_string(i);             
             if (bbEdgeDist.find(pair)!= bbEdgeDist.end()){
                total_distance += (bbEdgeDist[pair] * temp[i]) ;
             }
         }
      } 
      return total_distance;
}


u32 get_shortest_bb_distance(u8* bitmap, u32 len, u32 target){

     u32 shortest_distance = MAX_DISTANCE;
     u8* temp = bitmap;
     for (int i = 0; i < len; i++){
         if (temp[i] != 0){
             string pair = to_string(target) + "&" + to_string(bb2bb[i]);
             if (bbEdgeDist.find(pair)!= bbEdgeDist.end()){
                u32 distance = bbEdgeDist[pair];
                if (shortest_distance > distance)
                   shortest_distance = distance;  
             }             
         }
     }
     return shortest_distance;
}


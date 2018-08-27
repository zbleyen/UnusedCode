//
//  dijkstra.cpp
//  UnusedCodeAnalyzer
//
//  Created by 张博 on 2018/6/15.
//  Copyright © 2018年 张博. All rights reserved.
//

#include <stdio.h>
#include "json.hpp"

#define MAXDist 10000

using namespace std;
using namespace nlohmann;


static json wholeGraph(json callees,vector<string> whiteList) {
    json wholeGraph = json(callees);
    json loadCallees ;
    for(vector<string>::iterator it=whiteList.begin();it!=whiteList.end();++it){
        loadCallees[*it] = 1;
    }
    wholeGraph["+load()"] = loadCallees;
    return wholeGraph;
}

static json filterGraph(json graph) {
    json filteredG ;
    json nodes;
    for (json::iterator it = graph.begin(); it != graph.end(); ++it) {
        string source = it.key();
        nodes.push_back(source);
    }
    
    for (json::iterator it = graph.begin(); it != graph.end(); ++it) {
        json callee = it.value();
        json filteredCallee;
        for (json::iterator it = callee.begin(); it != callee.end(); ++it) {
            string dest = it.key();
            if (std::find(nodes.begin(), nodes.end(), dest) != nodes.end()) {
                filteredCallee[dest] = it.value();
            }
        }
        filteredG[it.key()] = filteredCallee;
    }
    return filteredG;
}

static json calcDistance(json graph) {
    json newGraph ;
    for (json::iterator it = graph.begin(); it != graph.end(); ++it) {
        json callee = it.value();
        json newCallee;
        for (json::iterator it = callee.begin(); it != callee.end(); ++it) {
            string dest = it.key();
            float distance = 1.0 / ((int)it.value());
            newCallee[dest] = distance;
        }
        newGraph[it.key()] = newCallee;
    }
    return newGraph;
}

static json minGraph(json graph, string origin) {
    json inSet;
    json distMin;
    json minGraph;      //key-dest,value-start
    int nodesN = 0;
    int inSetN = 0;
    //init
    for (json::iterator it = graph.begin(); it != graph.end(); ++it) {
        inSet[it.key()] = false;
        distMin[it.key()] = MAXDist;
        nodesN ++;
    }
    
    /*
     dijkstra
     */
    // add origin point
//    inSet[origin] = true;
//    inSetN = 1;
    distMin[origin] = 0;
//    json callee = graph[origin];
//    for (json::iterator it = callee.begin(); it != callee.end(); ++it) {
//        string dest = it.key();
//        float distance = it.value();
//        if ((float)distMin[dest] > (float)(distMin[origin]) + distance) {
//            distMin[dest] = (float)(distMin[origin]) + distance;
//        }
//    }
    
    
    while (inSetN < nodesN) {
        string minDistNode;
        float minDist = MAXDist;
        for (json::iterator it = distMin.begin(); it != distMin.end(); ++it) {
            string node = it.key();
            float dist = it.value();
            if ((bool)(inSet[node]) == false && minDist >= dist) {
                minDistNode = node;
                minDist = dist;
            }
        }
        inSet[minDistNode] = true;
        distMin[minDistNode] = minDist;
        inSetN ++;
        
        json callee = graph[minDistNode];
        for (json::iterator it = callee.begin(); it != callee.end(); ++it) {
            string dest = it.key();
            float distance = it.value();
            if ((float)distMin[dest] > minDist + distance) {
                distMin[dest] = minDist + distance;
                minGraph[dest] = minDistNode;
            }
        }
    }
    return minGraph;
}

/*
 for d3js visualization
 */
static json edgesForD3js(json callees,vector<string> whiteList) {
    json wholeG = wholeGraph(callees, whiteList);
    
    cout<<"wholeG:"<<wholeG<<endl;
    cout<<"-----------"<<endl;
    json filteredG = filterGraph(wholeG);
    json normalizedG = calcDistance(filteredG);
    cout<<"normalizedG:"<<normalizedG<<endl;
    cout<<"-----------"<<endl;
    json minG = minGraph(normalizedG, "main()");
    cout<<"minG:"<<minG<<endl;
    cout<<"-----------"<<endl;
    
    
    json origin;
    json links;
    for (json::iterator it = minG.begin(); it != minG.end(); ++it) {
        string dest = it.key();
        string start = it.value();
        json edge;
        edge["source"] = start;
        edge["dest"] = dest;
        links.push_back(edge);
    }
    origin["links"] = links;
    return origin;
}


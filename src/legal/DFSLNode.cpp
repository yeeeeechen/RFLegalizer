#include "DFSLNode.h"
#include "fp/rectilinear.h"
#include <iostream>

namespace DFSL{

DFSLNode::DFSLNode(){

}

DFSLNode::DFSLNode(std::string name, DFSLNodeType type, int id):
    nodeName(name), nodeType(type), index(id), area(0){ ; }

DFSLNode::~DFSLNode(){
    if (nodeType == DFSLNodeType::OVERLAP){
        delete mTileListPtr.overlapTiles;
    }
}

void DFSLNode::initOverlapNode(int overlapIndex1, int overlapIndex2){
    overlaps.insert(overlapIndex1);
    overlaps.insert(overlapIndex2);
    mTileListPtr.overlapTiles = new std::set<Tile*>;
}

void DFSLNode::initBlockNode(Rectilinear* recti){
    mTileListPtr.blockSet = &(recti->blockTiles);
    for (Tile* tile: getOverlapTileList()){
        this->area += tile->getArea(); 
    }
}


std::unordered_set<Tile *>& DFSLNode::getBlockTileList(){
    return *(mTileListPtr.blockSet);
}

std::set<Tile *>& DFSLNode::getOverlapTileList(){
    return *(mTileListPtr.overlapTiles);
}

void DFSLNode::addTile(Tile* newTile){
    switch (nodeType){
        case DFSLNodeType::OVERLAP:
            mTileListPtr.overlapTiles->insert(newTile);
            break;
        case DFSLNodeType::FIXED:
            mTileListPtr.blockSet->insert(newTile);
            break;
        case DFSLNodeType::SOFT:
            mTileListPtr.blockSet->insert(newTile);
            break;
        case DFSLNodeType::BLANK:
            mTileListPtr.blankTile = newTile;
            this->area = 0;
            break;
    }
    this->area += newTile->getArea();
}

}
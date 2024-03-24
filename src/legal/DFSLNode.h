#ifndef _DFSLNODE_H_
#define _DFSLNODE_H_

#include <vector>
#include <set>
#include <unordered_set>
#include <string>
#include "DFSLEdge.h"
#include "fp/tile.h"
#include "fp/rectilinear.h"

namespace DFSL {

enum class DFSLNodeType : unsigned char { OVERLAP, FIXED, SOFT, BLANK };

union tileListUnion;
class DFSLNode;

union tileListUnion
{
    std::unordered_set<Tile *>* blockSet;
    std::set<Tile *>* overlapTiles;
    Tile* blankTile;
};

class DFSLNode {
private:
    tileListUnion mTileListPtr;
public:
    DFSLNodeType nodeType;
    std::string nodeName;
    int area;
    int index;
    // std::vector<Tile*> tileList; 
    std::vector<DFSLEdge> edgeList;
    std::set<int> overlaps;

    DFSLNode();
    DFSLNode(std::string name, DFSLNodeType nodeType, int index);
    ~DFSLNode();

    std::unordered_set<Tile *>& getBlockTileList();
    std::set<Tile *>& getOverlapTileList();

    void addTile(Tile* newTile);
    // void removeTile(Tile* deleteTile);
    void initOverlapNode(int overlapIndex1, int overlapIndex2);
    void initBlockNode(Rectilinear* recti);
};

}
#endif
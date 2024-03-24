#ifndef _DFSLEGALIZER_H_
#define _DFSLEGALIZER_H_

#include <string>
#include <cstdarg>
#include <vector>
#include <map>
// #include "LFLegaliser.h"
#include "boost/polygon/polygon.hpp"
#include "fp/floorplan.h"
#include "DFSLConfig.hpp"
#include "DFSLNode.h"
#include "DFSLEdge.h"
#include "DFSLUnits.h"

#define PI 3.14159265358979323846
#define EPSILON 0.0001
#define UTIL_RULE 0.8
#define ASPECT_RATIO_RULE 2.0 

namespace DFSL {

using namespace boost::polygon::operators;

struct MigrationEdge;

enum class RESULT : unsigned char { SUCCESS, OVERLAP_NOT_RESOLVED, CONSTRAINT_FAIL };

class DFSLegalizer{
private:
    std::vector<DFSLNode> mAllNodes;
    double mBestCost;
    int mMigratingArea;
    int mResolvableArea;
    std::vector<MigrationEdge> mBestPath;
    std::vector<MigrationEdge> mCurrentPath;
    std::multimap<Tile*, int> mTilePtr2NodeIndex;
    Floorplan* mFP;
    int mFixedModuleNum;
    int mSoftModuleNum;
    int mOverlapNum;
    int mBlankNum;
    
    // initialize related functions
    void addOverlapInfo(Tile* tile);
    void addSingleOverlapInfo(Tile* tile, int overlapIdx1, int overlapIdx2);
    void getSoftNeighbors(int nodeId);
    void addBlockNode(Rectilinear* tess, bool isFixed);
    void findOBEdge(int fromIndex, int toIndex);
    std::string toOverlapName(int tessIndex1, int tessIndex2);

    // related to neighbor graph
    void updateGraph();

    // DFS path finding
    bool migrateOverlap(int overlapIndex);
    void dfs(DFSLEdge& edge, double currentCost);
    MigrationEdge getEdgeCost(DFSLEdge& edge);
    Rectangle getRectFromEdge(MigrationEdge& edge, bool findRemainder, Rectangle& remainderRect, bool useCeil);

    // functions that change physical layout
    bool placeInBlank(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    bool splitOverlap(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    bool splitOverlap2(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    bool splitSoftBlock(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    void removeIndexFromOverlap(int indexToRemove, Tile* overlapTile);
public:
    DFSLegalizer();
    ~DFSLegalizer();

    int getSoftBegin();
    int getSoftEnd();
    int getFixedBegin();
    int getFixedEnd();
    int getOverlapBegin();
    int getOverlapEnd();
    

    void setOutputLevel(int level);
    void initDFSLegalizer(Floorplan* floorplan);
    void constructGraph();
    RESULT legalize(int mode);
    void DFSLPrint(int level, const char* fmt, ...);
    void printFloorplanStats();
    ConfigList config;
};

Rectangle tile2Rectangle(Tile* tile);

void TileVec2PolySet(std::vector<Tile*>& tileVec, Polygon90Set& polySet);
Segment findTangentSegment(Tile* from, Tile* to, int direction);

struct MigrationEdge {
    int fromIndex;
    int toIndex;
    Segment segment;
    Rectangle migratedArea; 
    double edgeCost;
    MigrationEdge();
    MigrationEdge(int from, int to, Rectangle& area, Segment& seg, double cost);
};

}

#endif
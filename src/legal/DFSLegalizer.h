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

#define PI 3.14159265358979323846
#define EPSILON 0.0001
#define UTIL_RULE 0.8
#define ASPECT_RATIO_RULE 2.0 

namespace DFSL {

namespace gtl = boost::polygon;
typedef gtl::rectangle_data<int> Rectangle;
typedef gtl::polygon_90_set_data<int> Polygon90Set;
typedef gtl::polygon_90_with_holes_data<int> Polygon90WithHoles;
typedef gtl::point_data<int> Point;
using namespace boost::polygon::operators;


struct DFSLNode;
struct DFSLEdge;
struct Segment;
struct LegalInfo;
struct OverlapArea;
struct MigrationEdge;

enum class DFSLTessType : unsigned char { OVERLAP, FIXED, SOFT, BLANK };

enum class DIRECTION : unsigned char { TOP, RIGHT, DOWN, LEFT, NONE };

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
    int mFixedTessNum;
    int mSoftTessNum;
    int mOverlapNum;
    int mBlankNum;
    
    // initialize related functions
    void addOverlapInfo(Tile* tile);
    void addSingleOverlapInfo(Tile* tile, int overlapIdx1, int overlapIdx2);
    void getTessNeighbors(int nodeId, std::set<int>& allNeighbors);
    void addBlockNode(Tessera* tess, bool isFixed);
    std::string toOverlapName(int tessIndex1, int tessIndex2);

    // DFS path finding
    bool migrateOverlap(int overlapIndex);
    void dfs(DFSLEdge& edge, double currentCost);
    MigrationEdge getEdgeCost(DFSLEdge& edge);
    void DFSLTraverseBlank(Tile* tile, std::vector <Cord> &record);
    void findEdge(int fromIndex, int toIndex);
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
    void setOutputLevel(DFSL_PRINTLEVEL level);
    void initDFSLegalizer(Floorplan* floorplan);
    void constructGraph();
    RESULT legalize(int mode);
    void DFSLPrint(int level, const char* fmt, ...);
    void printFloorplanStats();
    DFSLC::ConfigList config;
};

bool removeFromVector(int a, std::vector<int>& vec);
bool removeFromVector(Tile* a, std::vector<Tile*>& vec);

static bool compareXSegment(Segment a, Segment b);
static bool compareYSegment(Segment a, Segment b);

LegalInfo getLegalInfo(std::vector<Tile*>& tiles); 
LegalInfo getLegalInfo(std::set<Tile*>& tiles); 
LegalInfo getLegalInfo(Polygon90Set& tiles); 

Rectangle tile2Rectangle(Tile* tile);

void TileVec2PolySet(std::vector<Tile*>& tileVec, Polygon90Set& polySet);
// in the direction of seg.direction,
// find the segment in poly of the same orientation that has the shortest distance
Segment FindNearestOverlappingInterval(Segment& seg, Polygon90Set& poly);

struct DFSLNode {
    DFSLNode();
    std::vector<Tile*> tileList; 
    std::vector<DFSLEdge> edgeList;
    std::set<int> overlaps;
    std::string nodeName;
    DFSLTessType nodeType;
    int area;
    int index;
};

// note: use ONLY on vertical and horizontal segments 
struct Segment {
    Cord segStart;
    Cord segEnd;
    DIRECTION direction; // direction of the normal vector of this segment
};

struct DFSLEdge {
    int fromIndex;
    int toIndex; 
    std::vector<Segment> tangentSegments;
};

struct MigrationEdge {
    int fromIndex;
    int toIndex;
    Segment segment;
    Rectangle migratedArea; 
    double edgeCost;
    MigrationEdge();
    MigrationEdge(int from, int to, Rectangle& area, Segment& seg, double cost);
};

struct LegalInfo {
    // bounding box related
    Cord BL;
    int width;
    int height;
    int bbArea;
    double aspectRatio; // w/h

    // utilization
    int actualArea;
    double util;
};

}

#endif
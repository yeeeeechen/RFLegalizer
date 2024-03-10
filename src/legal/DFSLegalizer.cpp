#include "DFSLegalizer.h"
#include "DFSLConfig.hpp"
#include <vector>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <sstream>
#include <ctime> 
#include <cstdarg>

namespace DFSL {

DFSLNode::DFSLNode(): area(0), index(0) {}

DFSLegalizer::DFSLegalizer()
{
    config.initAllConfigs();
}

DFSLegalizer::~DFSLegalizer()
{
}


void DFSLegalizer::DFSLPrint(int level, const char* fmt, ...){
    using namespace DFSLC;
    va_list args;
    va_start( args, fmt );
    if (config.getConfigValue<int>("OutputLevel") >= level){
        if ( level == DFSL_ERROR )
            printf( "[DFSL] ERROR  : " );
        else if ( level == DFSL_WARNING )
            printf( "[DFSL] WARNING: " );
        else if (level == DFSL_STANDARD){
            printf( "[DFSL] INFO   : ");
        }
        vprintf( fmt, args );
        va_end( args );
    }
}

void DFSLegalizer::initDFSLegalizer(Floorplan* floorplan){
    mFP = floorplan;
    constructGraph();
}

void DFSLegalizer::addBlockNode(Tessera* tess, bool isFixed){
    DFSLNode newNode;
    newNode.nodeName = tess->getName();
    newNode.nodeType = isFixed ? DFSLTessType::FIXED : DFSLTessType::SOFT;
    newNode.index = mAllNodes.size();
    for(Tile* tile : tess->TileArr){
        newNode.tileList.push_back(tile); 
        newNode.area += tile->getArea();
        mTilePtr2NodeIndex.insert(std::pair<Tile*,int>(tile, mAllNodes.size()));
    }
    mAllNodes.push_back(newNode);
}

void DFSLegalizer::constructGraph(){
    mAllNodes.clear();
    mTilePtr2NodeIndex.clear();
    mFixedTessNum = mFP->getPreplacedRectilinearCount();
    mSoftTessNum = mFP->getSoftRectilinearCount();

    // find fixed and soft tess
    for(int t = 0; t < mFixedTessNum; t++){
        Rectilinear* rect = mFP->fixedTesserae[t];
        addBlockNode(tess, true);
    }

    for(int t = 0; t < mSoftTessNum; t++){
        Rectilinear* rect = mFP->softTesserae[t];
        addBlockNode(tess, false);
    }

    // find overlaps 
    for(int t = 0; t < mFixedTessNum); t++){
        Rectilinear* rect = mFP->fixedTesserae[t];
        for(Tile* overlap : tess->OverlapArr){
            addOverlapInfo(overlap);
        }
    }

    for(int t = 0; t < mSoftTessNum; t++){
        Rectilinear* rect = mFP->softTesserae[t];
        for(Tile* overlap : tess->OverlapArr){
            addOverlapInfo(overlap);
        }
    }
    mOverlapNum = mAllNodes.size() - mFixedTessNum - mSoftTessNum;

    // find all blanks
    std::vector <Cord> blankRecord;
    // This line would cuase bug...
    DFSLTraverseBlank(mFP->getRandomTile(), blankRecord);
    mBlankNum = mAllNodes.size() - mFixedTessNum - mSoftTessNum - mOverlapNum;


    // find neighbors
    // overlap and block
    int overlapStartIndex = mFixedTessNum + mSoftTessNum;
    int overlapEndIndex = overlapStartIndex + mOverlapNum;
    for (int from = overlapStartIndex; from < overlapEndIndex; from++){
        DFSLNode& overlap = mAllNodes[from];
        for (int to: overlap.overlaps){
            if (to >= mFixedTessNum){
                findEdge(from, to);
            }
        }
    }

    // block and block, block and whitespace
    int softStartIndex = mFixedTessNum;
    int softEndIndex = softStartIndex + mSoftTessNum;
    int blankStartIndex = mFixedTessNum + mSoftTessNum + mOverlapNum;
    int blankEndIndex = blankStartIndex + mBlankNum;
    for (int from = softStartIndex; from < softEndIndex; from++){
        std::set<int> allNeighbors;
        getTessNeighbors(from, allNeighbors);
        for (int to: allNeighbors){
            if (softStartIndex <= to && to < softEndIndex){
                findEdge(from, to);
            }
            else if (blankStartIndex <= to && to < blankEndIndex){
                findEdge(from, to);
            }
        }
    }
    
    for (OverlapArea& tempArea: mTransientOverlapArea){
        for (int from = overlapStartIndex; from < overlapEndIndex; from++){
            DFSLNode& overlap = mAllNodes[from];
            if (overlap.overlaps.count(tempArea.index1) == 1 && overlap.overlaps.count(tempArea.index2) == 1){
                overlap.area = tempArea.area;
            }
        }
    }
}

void DFSLegalizer::addOverlapInfo(Tile* tile){
    std::vector<int> allOverlaps;
    for (int fixedIndex: tile->OverlapFixedTesseraeIdx){
        allOverlaps.push_back(fixedIndex);
    }
    
    for (int softIndex: tile->OverlapSoftTesseraeIdx){
        allOverlaps.push_back(softIndex + mFixedTessNum);
    }

    for (int i = 0; i < allOverlaps.size(); i++){
        int tessIndex1 = allOverlaps[i];

        for (int j = i+1; j < allOverlaps.size(); j++){
            int tessIndex2 = allOverlaps[j];
            addSingleOverlapInfo(tile, tessIndex1, tessIndex2);
        }
    }
}

void DFSLegalizer::addSingleOverlapInfo(Tile* tile, int overlapIdx1, int overlapIdx2){
    bool found = false;
    for (int i = mFixedTessNum + mSoftTessNum; i < mAllNodes.size(); i++){
        DFSLNode& tess = mAllNodes[i];
        if (tess.overlaps.count(overlapIdx1) == 1 && tess.overlaps.count(overlapIdx2) == 1){
            for (Tile* existingTile: tess.tileList){
                if (existingTile == tile){
                    found = true;
                    break;
                }
            }
            if (!found){
                tess.tileList.push_back(tile);
                tess.area += tile->getArea();
                mTilePtr2NodeIndex.insert(std::pair<Tile*,int>(tile, i));
                found = true;
            }
            break;
        }
    }
    if (!found){
        DFSLNode newNode;
        newNode.area += tile->getArea();
        newNode.tileList.push_back(tile);
        newNode.overlaps.insert(overlapIdx1);
        newNode.overlaps.insert(overlapIdx2);
        newNode.nodeName = toOverlapName(overlapIdx1, overlapIdx2); 
        newNode.nodeType = DFSLTessType::OVERLAP;
        newNode.index = mAllNodes.size();
        mTilePtr2NodeIndex.insert(std::pair<Tile*,int>(tile, mAllNodes.size()));
        mAllNodes.push_back(newNode);
    }
}

std::string DFSLegalizer::toOverlapName(int tessIndex1, int tessIndex2){
    std::string name1 = mAllNodes[tessIndex1].nodeName;
    std::string name2 = mAllNodes[tessIndex2].nodeName;
    return "OVERLAP_" + name1 + "_" + name2;
}

void DFSLegalizer::DFSLTraverseBlank(Tile* tile, std::vector <Cord> &record){
    record.push_back(tile->getLowerLeft());
    
    if(tile->getType() == tileType::BLANK){
        DFSLNode newNode;
        newNode.tileList.push_back(tile);
        newNode.nodeName = std::to_string((intptr_t)tile);
        newNode.nodeType = DFSLTessType::BLANK;
        newNode.index = mAllNodes.size();
        newNode.area += tile->getArea();
        mTilePtr2NodeIndex.insert(std::pair<Tile*,int>(tile, mAllNodes.size()));
        mAllNodes.push_back(newNode);
    }

    if(tile->rt != nullptr){        
        if(!checkVectorInclude(record, tile->rt->getLowerLeft())){
            DFSLTraverseBlank(tile->rt, record);
        }
    }

    if(tile->lb != nullptr){
        if(!checkVectorInclude(record, tile->lb->getLowerLeft())){
            DFSLTraverseBlank(tile->lb, record);
        }
    }

    if(tile->bl != nullptr){
        if(!checkVectorInclude(record, tile->bl->getLowerLeft())){
            DFSLTraverseBlank(tile->bl, record);
        }
    }

    if(tile->tr != nullptr){
        if(!checkVectorInclude(record, tile->tr->getLowerLeft())){
            DFSLTraverseBlank(tile->tr, record);
        }
    }
    
    return;
}

void DFSLegalizer::findEdge(int fromIndex, int toIndex){
    DFSLNode& fromNode = mAllNodes[fromIndex];
    DFSLNode& toNode = mAllNodes[toIndex];

    // this takes care of the case where a block is completely covered by overlap tiles,
    // no edge from overlap to block
    if (fromNode.tileList.size() == 0 || toNode.tileList.size() == 0){
        return;
    }
    std::vector<Segment> allTangentSegments;
    for (int dir = 0; dir < 4; dir++){
        // find Top, right, bottom, left neighbros
        std::vector<Segment> currentSegment;
        for (Tile* tile: fromNode.tileList){
            std::vector<Tile*> neighbors;
            switch (dir) {
            case 0:
                mFP->findTopNeighbors(tile, neighbors);
                break;
            case 1:
                mFP->findRightNeighbors(tile, neighbors);
                break;
            case 2:
                mFP->findDownNeighbors(tile, neighbors);
                break;
            case 3:
                mFP->findLeftNeighbors(tile, neighbors);
                break;
            }
            
            for (Tile* neighbor: neighbors){
                auto itlow = mTilePtr2NodeIndex.lower_bound(neighbor);
                auto itup = mTilePtr2NodeIndex.upper_bound(neighbor);
                for (auto it = itlow; it != itup; ++it){
                    int nodeIndex = it->second;
                    if (nodeIndex == toIndex){
                        // find tangent
                        Segment tangent;
                        Cord fromStart, fromEnd, toStart, toEnd;
                        if (dir == 0){
                            // neighbor on top of fromNode 
                            fromStart = tile->getUpperLeft();
                            fromEnd = tile->getUpperRight();
                            toStart = neighbor->getLowerLeft(); 
                            toEnd = neighbor->getLowerRight();
                            tangent.direction = DIRECTION::TOP;
                        }
                        else if (dir == 1){
                            // neighbor is right of fromNode 
                            fromStart = tile->getLowerRight();
                            fromEnd = tile->getUpperRight();
                            toStart = neighbor->getLowerLeft(); 
                            toEnd = neighbor->getUpperLeft();
                            tangent.direction = DIRECTION::RIGHT;
                        }
                        else if (dir == 2){
                            // neighbor is bottom of fromNode 
                            fromStart = tile->getLowerLeft();
                            fromEnd = tile->getLowerRight();
                            toStart = neighbor->getUpperLeft(); 
                            toEnd = neighbor->getUpperRight();
                            tangent.direction = DIRECTION::DOWN;
                        }
                        else {
                            // neighbor is left of fromNode 
                            fromStart = tile->getLowerLeft();
                            fromEnd = tile->getUpperLeft();
                            toStart = neighbor->getLowerRight(); 
                            toEnd = neighbor->getUpperRight();
                            tangent.direction = DIRECTION::LEFT;
                        }
                        tangent.segStart = fromStart <= toStart ? toStart : fromStart;
                        tangent.segEnd = fromEnd <= toEnd ? fromEnd : toEnd;
                        currentSegment.push_back(tangent);
                    }
                }
            }
        }
        
        // check segment, splice touching segments together and add to allTangentSegments
        int segLength = 0;
        if (currentSegment.size() > 0){
            if (dir == 0 || dir == 2){
                std::sort(currentSegment.begin(), currentSegment.end(), compareXSegment);
            }
            else {
                std::sort(currentSegment.begin(), currentSegment.end(), compareYSegment);
            }
            Cord segBegin = currentSegment.front().segStart;
            for (int j = 1; j < currentSegment.size(); j++){
                if (currentSegment[j].segStart != currentSegment[j-1].segEnd){
                    Cord segEnd = currentSegment[j-1].segEnd;
                    Segment splicedSegment;
                    splicedSegment.segStart = segBegin;
                    splicedSegment.segEnd = segEnd;
                    splicedSegment.direction = currentSegment[j-1].direction;
                    allTangentSegments.push_back(splicedSegment);
                    segLength += (segEnd.x - segBegin.x) + (segEnd.y - segBegin.y);  

                    segBegin = currentSegment[j].segStart;
                }
            }
            Cord segEnd = currentSegment.back().segEnd;
            Segment splicedSegment;
            splicedSegment.segStart = segBegin;
            splicedSegment.segEnd = segEnd;
            splicedSegment.direction = currentSegment.back().direction;
            allTangentSegments.push_back(splicedSegment);
            segLength += (segEnd.x - segBegin.x) + (segEnd.y - segBegin.y);  
        }
    }

    // construct edge in graph
    DFSLEdge newEdge;
    newEdge.tangentSegments = allTangentSegments;
    newEdge.fromIndex = fromIndex;
    newEdge.toIndex = toIndex; 
    mAllNodes[fromIndex].edgeList.push_back(newEdge);
}

void DFSLegalizer::getTessNeighbors(int nodeId, std::set<int>& allNeighbors){
    DFSLNode& node = mAllNodes[nodeId];
    std::vector<Tile*> allNeighborTiles;
    for (Tile* tile : node.tileList){
        mFP->findAllNeighbors(tile, allNeighborTiles);
    }

    for (Tile* tile : allNeighborTiles){
        auto itlow = mTilePtr2NodeIndex.lower_bound(tile);
        auto itup = mTilePtr2NodeIndex.upper_bound(tile);
        for (auto it = itlow; it != itup; ++it){
            int nodeIndex = it->second;
            if (nodeIndex != nodeId){
                allNeighbors.insert(nodeIndex);
            }
        }
    }
}

void DFSLegalizer::printFloorplanStats(){
    DFSLPrint(2, "Remaining overlaps: %d\n", mOverlapNum);
    long overlapArea = 0, physicalArea = 0, dieArea = mFP->getCanvasWidth() * mFP->getCanvasHeight();
    int overlapStart = mFixedTessNum + mSoftTessNum;
    int overlapEnd = overlapStart + mOverlapNum;
    for (int i = overlapStart; i < overlapEnd; i++){
        DFSLNode& currentOverlap = mAllNodes[i];
        overlapArea += currentOverlap.area;
    }
    physicalArea = dieArea;
    int blankStartIndex = mFixedTessNum + mSoftTessNum + mOverlapNum;
    int blankEndIndex = blankStartIndex + mBlankNum;
    for (int i = blankStartIndex; i < blankEndIndex; i++){
        DFSLNode& currentBlank = mAllNodes[i];
        physicalArea -= currentBlank.area;
    }
    double overlapOverDie = (double) overlapArea / (double) dieArea;
    double overlapOverPhysical = (double) overlapArea / (double) physicalArea;
    DFSLPrint(2, "Total Overlap Area: %6ld\t(o/d = %5.4f%\to/p = %5.4f%)\n", overlapArea, overlapOverDie * 100, overlapOverPhysical * 100);
}

// mode 0: resolve area big -> area small
// mode 1: resolve area small -> area big
// mode 2: resolve overlaps near center -> outer edge
// mode 3: completely random
RESULT DFSLegalizer::legalize(int mode){
    // todo: create backup (deep copy)
    RESULT result;
    int iteration = 0;
    std::srand(69);
    
    // struct timespec start, end;
    // clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    
    DFSLPrint(2, "Starting Legalization\n");
    while (1) {
        if (iteration % 10 == 0){
            printFloorplanStats();
        }
        std::cout << std::flush;
        int overlapStart = mFixedTessNum + mSoftTessNum;
        int overlapEnd = overlapStart + mOverlapNum;

        if (overlapStart == overlapEnd){
            break;
        }

        bool overlapResolved = false;
        std::vector<bool> solveable(mAllNodes.size(), true);
        int resolvableOverlaps = mOverlapNum;
        
        while (!overlapResolved){
            int bestMetric;
            if (mode == 1 || mode == 2){
                bestMetric = INT_MAX;
            }
            else {
                bestMetric = -INT_MAX;
            }

            if (resolvableOverlaps == 0){
                DFSLPrint(0, "Overlaps unable to resolve\n");
                result = RESULT::OVERLAP_NOT_RESOLVED;
                return result;
            }

            int bestIndex = -1;
            int mode3RandomChoice = std::rand() % resolvableOverlaps;
            for (int i = overlapStart; i < overlapEnd; i++){
                DFSLNode& currentOverlap = mAllNodes[i];
                switch (mode)
                {
                case 1:
                    if (currentOverlap.area < bestMetric && solveable[i]){
                        bestMetric = currentOverlap.area;
                        bestIndex = i;
                    }
                    break;
                
                case 2:{
                    int chipCenterx = mFP->getCanvasWidth() / 2;
                    int chipCentery = mFP->getCanvasHeight() / 2;
                    
                    int min_x, max_x, min_y, max_y;
                    min_x = min_y = INT_MAX;
                    max_x = max_y = -INT_MAX;

                    for (Tile* tile: currentOverlap.tileList){
                        if (tile->getLowerLeft().x < min_x){
                            min_x = tile->getLowerLeft().x;
                        }            
                        if (tile->getLowerLeft().y < min_y){
                            min_y = tile->getLowerLeft().y;
                        }            
                        if (tile->getUpperRight().x > max_x){
                            max_x = tile->getUpperRight().x;
                        }            
                        if (tile->getUpperRight().y > max_y){
                            max_y = tile->getUpperRight().y;
                        }
                    }
                    int overlapCenterx = (min_x + max_x) / 2;
                    int overlapCentery = (min_y + max_y) / 2;
                    int distSquared = pow(overlapCenterx - chipCenterx, 2) + pow(overlapCentery - chipCentery, 2);

                    if (distSquared < bestMetric && solveable[i]){
                        bestMetric = distSquared;
                        bestIndex = i;
                    }

                    break;
                }
                case 3: {
                    if (solveable[i]){
                        if (mode3RandomChoice == 0){
                            bestIndex = i;
                        }
                        else {
                            mode3RandomChoice--;
                        }
                    }
                    break;
                }
                default:
                    // area big -> small, default
                    
                    if (currentOverlap.area > bestMetric && solveable[i]){
                        bestMetric = currentOverlap.area;
                        bestIndex = i;
                    }
                    break;
                }
            }

            solveable[bestIndex] = overlapResolved = migrateOverlap(bestIndex);
            if (!overlapResolved){
                resolvableOverlaps--;
            }
            
        }

        constructGraph();
        iteration++;
        
        // clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
        // if (end.tv_sec - start.tv_sec > 2*60){
        //     DFSLPrint(0, "Timeout\n");
        //     result = RESULT::OVERLAP_NOT_RESOLVED;
        //     return result;
        // }
    }

    // mLF->visualiseArtpiece("debug_DFSL_result.txt", true);

    // test each tess for legality
    result = RESULT::SUCCESS;
    int nodeStart = 0;
    int nodeEnd = mFixedTessNum + mSoftTessNum;
    int violations = 0;
    for (int i = nodeStart; i < nodeEnd; i++){
        int requiredArea = i < mFixedTessNum ? mFP->fixedTesserae[i]->getLegalArea() :  mFP->softTesserae[i-mFixedTessNum]->getLegalArea();
        if (requiredArea == 0){
            continue;
        }
        DFSLNode& node = mAllNodes[i];
        LegalInfo legal = getLegalInfo(node.tileList);
        if (legal.util < UTIL_RULE){
            DFSLPrint(1, "util for %s fail (%4.3f < %4.3f)\n", node.nodeName.c_str(), legal.util, UTIL_RULE);
            result = RESULT::CONSTRAINT_FAIL;
            violations++;
        }

        if (legal.aspectRatio > ASPECT_RATIO_RULE && node.nodeType == DFSLTessType::SOFT){
            DFSLPrint(1, "aspect ratio for %s fail (%3.2f > %3.2f)\n", node.nodeName.c_str(), legal.aspectRatio, ASPECT_RATIO_RULE);
            result = RESULT::CONSTRAINT_FAIL;
            violations++;
        }
        else if (legal.aspectRatio < 1.0 / ASPECT_RATIO_RULE && node.nodeType == DFSLTessType::SOFT){
            DFSLPrint(1, "aspect ratio for %s fail (%3.2f < %3.2f)\n", node.nodeName.c_str(), legal.aspectRatio, 1.0 / ASPECT_RATIO_RULE);
            result = RESULT::CONSTRAINT_FAIL;
            violations++;
        }

        if (legal.actualArea < requiredArea && node.nodeType == DFSLTessType::SOFT){
            DFSLPrint(1, "Required area for soft block %s fail (%d < %d)\n", node.nodeName.c_str(), legal.actualArea, requiredArea);
            result = RESULT::CONSTRAINT_FAIL; 
            violations++;           
        } 
        else if (legal.actualArea != requiredArea && node.nodeType == DFSLTessType::FIXED){
            DFSLPrint(1, "Required area for fixed block %s fail (%d != %d)\n", node.nodeName.c_str(), legal.actualArea, requiredArea);
            result = RESULT::CONSTRAINT_FAIL;
            violations++;
        }

        // test for fragmented polygons & holes
        Polygon90Set polySet;
        TileVec2PolySet(node.tileList, polySet);
        std::vector<Polygon90WithHoles> polyContainer;
        polySet.get_polygons(polyContainer);
        if (polyContainer.size() > 1){
            std::ostringstream messageStream;
            for (Polygon90WithHoles poly: polyContainer){
                Rectangle boundingBox;
                gtl::extents(boundingBox, poly);
                messageStream << "\t(" << gtl::xl(boundingBox) << ", " << gtl::yl(boundingBox) << "), W=" 
                << gtl::delta(boundingBox, gtl::orientation_2d_enum::HORIZONTAL) << ", H="
                << gtl::delta(boundingBox, gtl::orientation_2d_enum::VERTICAL) << '\n';
            }
            DFSLPrint(1, "Block %s has %d disjoint components:\n%s", node.nodeName.c_str(), polyContainer.size(), messageStream.str().c_str());
            result = RESULT::CONSTRAINT_FAIL;
            violations++;
        }
        else {
            if (polyContainer[0].size_holes() > 0){
                DFSLPrint(1, "Block %s has %d holes\n", node.nodeName.c_str(), polyContainer[0].size_holes());
                result = RESULT::CONSTRAINT_FAIL;
                violations++;
            }
        }

    }
    DFSLPrint(2, "Total Violations: %d\n", violations);
    return result;
}

// find actual rectangle, equal to resolvable area
Rectangle DFSLegalizer::getRectFromEdge(MigrationEdge& edge, bool findRemainder, Rectangle& remainderRect, bool useCeil = true){
    int xl = 0, xh = 0, yl = 0, yh = 0;
    int rxl = 0, rxh = 0, ryl = 0, ryh = 0;
    if (edge.segment.direction == DIRECTION::TOP){
        xl = edge.segment.segStart.x < edge.segment.segEnd.x ? edge.segment.segStart.x : edge.segment.segEnd.x;
        xh = edge.segment.segStart.x < edge.segment.segEnd.x ? edge.segment.segEnd.x : edge.segment.segStart.x;
        yl = edge.segment.segStart.y;
        int width = xh - xl;
        int requiredHeight = useCeil ? ceil((double) mResolvableArea / (double) width) : floor((double) mResolvableArea / (double) width);
        yh = yl + requiredHeight;
    }
    else if (edge.segment.direction == DIRECTION::RIGHT){
        yl = edge.segment.segStart.y < edge.segment.segEnd.y ? edge.segment.segStart.y : edge.segment.segEnd.y;
        yh = edge.segment.segStart.y < edge.segment.segEnd.y ? edge.segment.segEnd.y : edge.segment.segStart.y;
        xl = edge.segment.segStart.x;
        int height = yh - yl;
        int requiredWidth = useCeil ? ceil((double) mResolvableArea / (double) height) : floor((double) mResolvableArea / (double) height);
        xh = xl + requiredWidth;
    }
    else if (edge.segment.direction == DIRECTION::DOWN){
        xl = edge.segment.segStart.x < edge.segment.segEnd.x ? edge.segment.segStart.x : edge.segment.segEnd.x;
        xh = edge.segment.segStart.x < edge.segment.segEnd.x ? edge.segment.segEnd.x : edge.segment.segStart.x;
        yh = edge.segment.segStart.y;
        int width = xh - xl;
        int requiredHeight = useCeil ? ceil((double) mResolvableArea / (double) width) : floor((double) mResolvableArea / (double) width);
        yl = yh - requiredHeight;
    }
    else if (edge.segment.direction == DIRECTION::LEFT) {
        yl = edge.segment.segStart.y < edge.segment.segEnd.y ? edge.segment.segStart.y : edge.segment.segEnd.y;
        yh = edge.segment.segStart.y < edge.segment.segEnd.y ? edge.segment.segEnd.y : edge.segment.segStart.y;
        xh = edge.segment.segStart.x;
        int height = yh - yl;
        int requiredWidth = useCeil ? ceil((double) mResolvableArea / (double) height) : floor((double) mResolvableArea / (double) height);
        xl = xh - requiredWidth;
    }
    else {
        DFSLNode& fromNode = mAllNodes[edge.fromIndex];
        DFSLNode& toNode = mAllNodes[edge.toIndex];
        DFSLPrint(1, "Edge (%s -> %s) has no DIRECTION\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
    }
    
    int remainingArea = mResolvableArea - (xh - xl) * (yh - yl);
    if (remainingArea > 0 && findRemainder){    
        if (edge.segment.direction == DIRECTION::TOP){
            rxl = xl;
            rxh = rxl + remainingArea;
            ryl = yh;
            ryh = ryl + 1;
        }
        else if (edge.segment.direction == DIRECTION::RIGHT){
            rxl = xh;
            rxh = rxl + 1;
            ryl = yl;
            ryh = ryl + remainingArea;
        }
        else if (edge.segment.direction == DIRECTION::DOWN){
            rxl = xl;
            rxh = rxl + remainingArea;
            ryh = yl;
            ryl = ryh - 1;
        }
        else if (edge.segment.direction == DIRECTION::LEFT) {
            rxh = xl;
            rxl = rxh - 1;
            ryl = yl;
            ryh = ryl + remainingArea;
        }
        else {
            DFSLNode& fromNode = mAllNodes[edge.fromIndex];
            DFSLNode& toNode = mAllNodes[edge.toIndex];
            DFSLPrint(1, "Edge (%s -> %s) has no DIRECTION\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
        }
        remainderRect = Rectangle(rxl, ryl, rxh, ryh);
    }

    return Rectangle(xl,yl,xh,yh);
}

bool DFSLegalizer::splitOverlap2(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];
    if (!(fromNode.nodeType == DFSLTessType::OVERLAP && toNode.nodeType == DFSLTessType::SOFT)){
        DFSLPrint(0, "splitOverlap can only be used on overlap->soft edge.\n");
        return false;
    }
    
    DFSLEdge* fullEdge = NULL;
    for (DFSLEdge& fromEdge: fromNode.edgeList){
        if (fromEdge.fromIndex == edge.fromIndex && fromEdge.toIndex == edge.toIndex){
            fullEdge = &fromEdge;
            break;
        }
    }
    if (fullEdge == NULL){
        DFSLPrint(0, "OB edge not found.\n");
        return false;
    }
  
    // resolve rects of overlaps in order of (length of perimeter touching block)/(total perimeter)
}

bool DFSLegalizer::splitOverlap(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];
    if (!(fromNode.nodeType == DFSLTessType::OVERLAP && toNode.nodeType == DFSLTessType::SOFT)){
        DFSLPrint(0, "splitOverlap can only be used on overlap->soft edge.\n");
        return false;
    }
    
    DFSLEdge* fullEdge = NULL;
    for (DFSLEdge& fromEdge: fromNode.edgeList){
        if (fromEdge.fromIndex == edge.fromIndex && fromEdge.toIndex == edge.toIndex){
            fullEdge = &fromEdge;
            break;
        }
    }
    if (fullEdge == NULL){
        DFSLPrint(0, "OB edge not found.\n");
        return false;
    }

    // find segment directions
    // if overlap flows x area to A, then x amount of area should be assigned to B in overlap
    std::vector<bool> sideOccupied(4, false);
    for (Segment& segment: fullEdge->tangentSegments){
        switch (segment.direction){
        case DIRECTION::TOP:
            sideOccupied[0] = true;
            break;
        case DIRECTION::RIGHT:
            sideOccupied[1] = true;
            break;
        case DIRECTION::DOWN:
            sideOccupied[2] = true;
            break;
        case DIRECTION::LEFT:
            sideOccupied[3] = true;
            break;
        default:
            DFSLPrint(1, "OB edge segment has no direction\n");
            break;
        }
    }


    // From ascending order of area, resolve each overlap tile individually 
    std::vector<Tile*>& overlapTileList = fromNode.tileList;
    std::vector<bool> tileChosen(overlapTileList.size(), false);
    int remainingMigrateArea = mResolvableArea;
    int smallestRecIndex = -1;
    while (true){
        // find Tile with smallest area
        int smallestArea = INT_MAX;
        for (int i = 0; i < overlapTileList.size(); i++){
            Tile* tile = overlapTileList[i];
            int currentArea = tile->getArea();
            if (currentArea < smallestArea && !tileChosen[i]){
                smallestRecIndex = i;
                smallestArea = currentArea;
            }
        }

        tileChosen[smallestRecIndex] = true;
        Tile* overlapTile = overlapTileList[smallestRecIndex];
        // if Tile area is smaller than mresolvable area, directly resolve that tile
        if (overlapTile->getArea() <= remainingMigrateArea){
            int indexToRemove = edge.toIndex;
            removeIndexFromOverlap(indexToRemove, overlapTile);
            
            remainingMigrateArea -= overlapTile->getArea();
            newTiles.push_back(overlapTile);
        }
        // otherwise, find the largest rectangle within that rectangle whose area
        // is smaller than the remainingMigrateArea
        else {
            // for each unoccupied side of the rectangle,
            // calculate the area of the rectangle extending from B
            // keep the one with the area closest to the actual resolvableArea
            int closestArea = -1;
            DIRECTION bestDirection = DIRECTION::TOP;
            Rectangle bestRectangle;
            if (!sideOccupied[0]){
                // grow from top side
                int width = overlapTile->getWidth();
                int height = (int) floor((double) remainingMigrateArea / (double) width);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::TOP;

                    int newYl = overlapTile->getUpperRight().y - height;
                    bestRectangle = tile2Rectangle(overlapTile);
                    gtl::yl(bestRectangle, newYl);
                }
            }
            if (!sideOccupied[1]){
                // grow from right side
                int height = overlapTile->getHeight();
                int width = (int) floor((double) remainingMigrateArea / (double) height);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::RIGHT;

                    int newXl = overlapTile->getUpperRight().x - width;
                    bestRectangle = tile2Rectangle(overlapTile);
                    gtl::xl(bestRectangle, newXl);
                }
            }
            if (!sideOccupied[2]){
                // grow from bottom side
                int width = overlapTile->getWidth();
                int height = (int) floor((double) remainingMigrateArea / (double) width);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::DOWN;

                    int newYh = overlapTile->getLowerLeft().y + height;
                    bestRectangle = tile2Rectangle(overlapTile);
                    gtl::yh(bestRectangle, newYh);
                }
            }
            if (!sideOccupied[3]){
                // grow from left side
                int height = overlapTile->getHeight();
                int width = (int) floor((double) remainingMigrateArea / (double) height);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::LEFT;

                    int newXh = overlapTile->getLowerLeft().x + width;
                    bestRectangle = tile2Rectangle(overlapTile);
                    gtl::xh(bestRectangle, newXh);
                }
            }
            
            if (closestArea > 0){
                // split tiles
                Tile* newTile = mFP->splitTile(overlapTile, bestRectangle);
                switch (bestDirection){
                    case DIRECTION::TOP:
                        overlapTile = newTile->lb;
                        break;
                    case DIRECTION::RIGHT:
                        overlapTile = newTile->bl;
                        break;
                    case DIRECTION::DOWN:
                        overlapTile = newTile->rt;
                        break;
                    case DIRECTION::LEFT:
                        overlapTile = newTile->tr;
                        break;
                    default:
                    break;
                }
                if (newTile == NULL){
                    DFSLPrint(0, "Split tile failed.\n");
                    return false;
                }
                else {
                    int indexToRemove = edge.toIndex;
                    removeIndexFromOverlap(indexToRemove, newTile);
                }
                newTiles.push_back(newTile);

                remainingMigrateArea -= newTile->getArea();
            }

            // if exact area migration, find remainder 
            // and split old overlap tile
            if (config.getConfigValue<bool>("ExactAreaMigration") && remainingMigrateArea > 0){
                int height, width;
                Rectangle remainderRectangle;
                switch (bestDirection){
                case DIRECTION::TOP:
                    // grow from top side
                    height = 1;
                    width = remainingMigrateArea;
                    remainderRectangle = tile2Rectangle(overlapTile);
                    gtl::xh(remainderRectangle, gtl::xl(remainderRectangle) + width);
                    gtl::yl(remainderRectangle, gtl::yh(remainderRectangle) - height);
                    
                    break;
                case DIRECTION::RIGHT:
                    // grow from right side
                    height = remainingMigrateArea;
                    width = 1;
                    remainderRectangle = tile2Rectangle(overlapTile);
                    gtl::xl(remainderRectangle, gtl::xh(remainderRectangle) - width);
                    gtl::yh(remainderRectangle, gtl::yl(remainderRectangle) + height);

                    break;
                case DIRECTION::DOWN:
                    // grow from down side
                    height = 1;
                    width = remainingMigrateArea;
                    remainderRectangle = tile2Rectangle(overlapTile);
                    gtl::xh(remainderRectangle, gtl::xl(remainderRectangle) + width);
                    gtl::yh(remainderRectangle, gtl::yl(remainderRectangle) + height);

                    break;
                case DIRECTION::LEFT:
                    // grow from left side
                    height = remainingMigrateArea;
                    width = 1;
                    remainderRectangle = tile2Rectangle(overlapTile);
                    gtl::xh(remainderRectangle, gtl::xl(remainderRectangle) + width);
                    gtl::yh(remainderRectangle, gtl::yl(remainderRectangle) + height);

                    break;
                default:
                    break;
                }
                // split tiles
                Tile* remainderTile = mFP->splitTile(overlapTile, remainderRectangle);
                if (remainderTile == NULL){
                    DFSLPrint(0, "Split tile failed (in exact area migration).\n");
                    return false;
                }
                else {
                    int indexToRemove = edge.toIndex;
                    removeIndexFromOverlap(indexToRemove, remainderTile);
                }
                newTiles.push_back(remainderTile);
            }

            break;
        }
    }
    return true;
}

bool DFSLegalizer::splitSoftBlock(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];

    if (!(fromNode.nodeType == DFSLTessType::SOFT && toNode.nodeType == DFSLTessType::SOFT)){
        DFSLPrint(0, "splitSoftBlock can only be used on soft->soft edge.\n");
        return false;
    }

    Rectangle migratedRect, remainderRect(0,0,0,0);
    if (config.getConfigValue<bool>("ExactAreaMigration")){
        migratedRect = getRectFromEdge(edge, true, remainderRect, false);
    }
    else {
        migratedRect = getRectFromEdge(edge, false, remainderRect, true);
    }

    if (gtl::area(migratedRect) > 0){
        for (Tile* tile: toNode.tileList){
            Rectangle tileRect = tile2Rectangle(tile);

            // known boost polygon issue: intersect does not pass the value of considerTouch to intersects
            bool intersects = gtl::intersects(tileRect, migratedRect, false);
            if (intersects){
                gtl::intersect(tileRect, migratedRect, false);
                Tile* newTile = mFP->splitTile(tile, tileRect);

                if (newTile == NULL){
                    DFSLPrint(0, "BB Split tile failed.\n");
                    return false;
                }
                else {
                    Tessera* fromTess = mFP->softTesserae[edge.fromIndex - mFixedTessNum];
                    Tessera* toTess = mFP->softTesserae[edge.toIndex - mFixedTessNum];
                    
                    bool removed = removeFromVector(newTile, toTess->TileArr);
                    if (!removed){
                        DFSLPrint(0, "In splitSoftBlock, tile not found in toTess\n");
                        return false;
                    }
                    fromTess->TileArr.push_back(newTile);
                }
                newTiles.push_back(newTile);
            }
        }
    }

    if (gtl::area(remainderRect) > 0 && config.getConfigValue<bool>("ExactAreaMigration")){
        std::vector<Tile*> updatedTileList = mFP->softTesserae[edge.toIndex - mFixedTessNum]->TileArr;
        for (Tile* tile: updatedTileList){
            Rectangle tileRect = tile2Rectangle(tile);

            // known boost polygon issue: intersect does not pass the value of considerTouch to intersects
            bool intersects = gtl::intersects(tileRect, remainderRect, false);
            if (intersects){
                gtl::intersect(tileRect, remainderRect, false);
                Tile* newTile = mFP->splitTile(tile, tileRect);

                if (newTile == NULL){
                    DFSLPrint(0, " BB Split tile failed\n");
                    return false;
                }
                else {
                    Tessera* fromTess = mFP->softTesserae[edge.fromIndex - mFixedTessNum];
                    Tessera* toTess = mFP->softTesserae[edge.toIndex - mFixedTessNum];
                    
                    bool removed = removeFromVector(newTile, toTess->TileArr);
                    if (!removed){
                        DFSLPrint(0, "In splitSoftBlock, tile not found in toTess\n");
                        return false;
                    }
                    fromTess->TileArr.push_back(newTile);
                }
                newTiles.push_back(newTile);
            }
        }
    }
    return true;
}

bool DFSLegalizer::migrateOverlap(int overlapIndex){
    mBestPath.clear();
    mCurrentPath.clear();
    mBestCost = (double) INT_MAX;
    mMigratingArea = mAllNodes[overlapIndex].area;

    DFSLPrint(3, "\nMigrating Overlap: %s\n", mAllNodes[overlapIndex].nodeName.c_str());
    for (DFSLEdge& edge: mAllNodes[overlapIndex].edgeList){
        dfs(edge, 0.0);
    }
    DFSLPrint(3, "Path cost: %f\n", mBestCost);

    if (mBestPath.size() == 0){
        DFSLPrint(3, "Path not found. Layout unchanged\n");
        return false;
    }

    std::ostringstream messageStream;
    messageStream << "Path: " << mAllNodes[overlapIndex].nodeName << " ";
    for (MigrationEdge& edge: mBestPath){
        if (mAllNodes[edge.toIndex].nodeType == DFSLTessType::BLANK){
            std::string direction;
            switch (edge.segment.direction)
            {
            case DIRECTION::TOP:
                direction = "above";
                break;
            case DIRECTION::RIGHT:
                direction = "right of";
                break;
            case DIRECTION::DOWN:
                direction = "below";
                break;
            case DIRECTION::LEFT:
                direction = "left of";
                break;
            default:
                direction = "error";
                break;
            }
            messageStream << "-> Whitespace " << direction << ' ' << mAllNodes[edge.fromIndex].nodeName 
                        << " (LL: " << mAllNodes[edge.toIndex].tileList[0]->getLowerLeft() << ") ";
        }
        else {
            messageStream << "-> " << mAllNodes[edge.toIndex].nodeName << ' ';
        }
    }
    DFSLPrint(3, messageStream.str().c_str());

    // go through path, find maximum resolvable area
    mResolvableArea = mMigratingArea;
    for (MigrationEdge& edge: mBestPath){
        DFSLNode& fromNode = mAllNodes[edge.fromIndex];
        DFSLNode& toNode = mAllNodes[edge.toIndex];

        if (fromNode.nodeType == DFSLTessType::SOFT && toNode.nodeType == DFSLTessType::SOFT){
            if (gtl::area(edge.migratedArea) < mResolvableArea){
                mResolvableArea = gtl::area(edge.migratedArea);
            }
        }
        else if (fromNode.nodeType == DFSLTessType::SOFT && toNode.nodeType == DFSLTessType::BLANK){
            if (gtl::area(edge.migratedArea) < mResolvableArea){
                mResolvableArea = gtl::area(edge.migratedArea);
            }
        }
    }
    DFSLPrint(3, "Resolvable Area: %d\n", mResolvableArea);

    // start changing physical layout
    for (MigrationEdge& edge: mBestPath){
        DFSLNode& fromNode = mAllNodes[edge.fromIndex];
        DFSLNode& toNode = mAllNodes[edge.toIndex];
        DFSLPrint(3, "* %s -> %s:\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());

        if (fromNode.nodeType == DFSLTessType::OVERLAP && toNode.nodeType == DFSLTessType::SOFT){
            if (mResolvableArea < mMigratingArea){
                // create transient overlap area
                std::vector<Tile*> newTiles;
                bool result = splitOverlap(edge, newTiles);

                DFSLPrint(3, "Overlap not completely resolvable (overlap area: %d, resolvable area: %d)\n", mMigratingArea, mResolvableArea);
                if (result){
                    std::ostringstream messageStream;

                    int actualAreaCount = 0;
                    for (Tile* tile: newTiles){
                        messageStream << "\t" << *tile << '\n';
                        actualAreaCount += tile->getArea();
                    }
                    if (actualAreaCount != mResolvableArea && config.getConfigValue<bool>("ExactAreaMigration")){
                        DFSLPrint(1, "Migration area mismatch (actual: %d)\n", actualAreaCount);
                    }
                    DFSLPrint(3, "Splitting overlap tile. New tile: \n%s", messageStream.str().c_str());
                }
                else {
                    DFSLPrint(3, "Split failed. Storing in value of remaining overlap area.\n");
                    OverlapArea tempArea;
                    tempArea.index1 = *(fromNode.overlaps.begin());
                    tempArea.index2 = *(fromNode.overlaps.rbegin());
                    tempArea.area = mMigratingArea - mResolvableArea;

                    mTransientOverlapArea.push_back(tempArea);
                }
            }
            else {
                DFSLPrint(3, "Removing %s attribute from %d tiles\n", toNode.nodeName.c_str(), fromNode.tileList.size());
                int indexToRemove = edge.toIndex; // should be soft index
                for (Tile* overlapTile: fromNode.tileList){
                    removeIndexFromOverlap(indexToRemove, overlapTile);
                }
            }
        }
        else if (fromNode.nodeType == DFSLTessType::SOFT && toNode.nodeType == DFSLTessType::SOFT){
            if (edge.segment.direction == DIRECTION::NONE) {
                DFSLPrint(0, "BB edge ( %s -> %s ) must have DIRECTION\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
                return false;
            }

            std::vector<Tile*> newTiles;
            bool result = splitSoftBlock(edge, newTiles);
            if (!result){
                DFSLPrint(0, "BB flow ( %s -> %s ) FAILED.\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
                return false;
            }
            else {
                std::ostringstream messageStream;

                int actualAreaCount = 0;
                for (Tile* tile: newTiles){
                    messageStream << "\t" << *tile << '\n';
                    actualAreaCount += tile->getArea();
                }
                if (actualAreaCount != mResolvableArea && config.getConfigValue<bool>("ExactAreaMigration")){
                    DFSLPrint(1, "Migration area mismatch (actual: %d)\n", actualAreaCount);
                }
                DFSLPrint(3, "Splitting tiles. New %s tile: \n%s", fromNode.nodeName.c_str(), messageStream.str().c_str());
            }

        }
        else if (fromNode.nodeType == DFSLTessType::SOFT && toNode.nodeType == DFSLTessType::BLANK){
            if (edge.segment.direction == DIRECTION::NONE) {
                std::ostringstream messageStream;
                messageStream << toNode.tileList[0]->getLowerLeft();
                DFSLPrint(0, "BW edge ( %s -> %s ) must have DIRECTION\n", fromNode.nodeName.c_str(), messageStream.str().c_str());
                return false;
            }
            
            std::vector<Tile*> newTiles;
            bool result = placeInBlank(edge, newTiles);
            if (!result){
                DFSLPrint(0, "BW flow ( %s -> %s ) FAILED.\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
                return false;
            }
            else {
                std::ostringstream messageStream;
                int actualAreaCount = 0;
                for (Tile* tile: newTiles){
                    messageStream << "\t" << *tile << '\n';
                    actualAreaCount += tile->getArea();
                }
                if (actualAreaCount != mResolvableArea && config.getConfigValue<bool>("ExactAreaMigration")){
                    DFSLPrint(1, "Migration area mismatch (actual: %d)\n", actualAreaCount);
                }
                DFSLPrint(3, "Placing tiles. New %s tiles: \n%s", fromNode.nodeName.c_str(), messageStream.str().c_str());
            }

        }
            // todo: deal with white -> white
        else {
            DFSLPrint(1, "Doing nothing for migrating path: %s -> %s\n", fromNode.nodeName.c_str(),  toNode.nodeName.c_str());
        }
    }

    // mLF->outputTileFloorplan("debug_DFSL.txt");
    return true;
}

bool DFSLegalizer::placeInBlank(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];
    Tessera* blockTess = mFP->softTesserae[fromNode.index - mFixedTessNum]; 

    Rectangle newRect, remainderRect(0,0,0,0);
    if (config.getConfigValue<bool>("ExactAreaMigration")){
        newRect = getRectFromEdge(edge, true, remainderRect, false);
    }
    else {
        newRect = getRectFromEdge(edge, false, remainderRect, true);
    }

    if (gtl::area(newRect) > 0){
        Tile* newTile = new Tile(tileType::BLOCK, Cord(gtl::xl(newRect), gtl::yl(newRect)),
                                    gtl::delta(newRect, gtl::orientation_2d_enum::HORIZONTAL), 
                                    gtl::delta(newRect, gtl::orientation_2d_enum::VERTICAL));

        // add to tess, place in physical layout
        newTiles.push_back(newTile);
        blockTess->TileArr.push_back(newTile);

        mFP->insertTile(*newTile);
    }

    if (config.getConfigValue<bool>("ExactAreaMigration") && gtl::area(remainderRect) > 0){
        Tile* remainderTile = new Tile(tileType::BLOCK, Cord(gtl::xl(remainderRect), gtl::yl(remainderRect)),
                            gtl::delta(remainderRect, gtl::orientation_2d_enum::HORIZONTAL), 
                            gtl::delta(remainderRect, gtl::orientation_2d_enum::VERTICAL));

        // add to tess, place in physical layout
        newTiles.push_back(remainderTile);
        blockTess->TileArr.push_back(remainderTile);

        mFP->insertTile(*remainderTile);
    }
    return true;
}

void DFSLegalizer::removeIndexFromOverlap(int indexToRemove, Tile* overlapTile){
    if (indexToRemove < mFixedTessNum){
        // should not happen
        DFSLPrint(0, "Migrating overlap to fixed block: %s\n", mFP->fixedTesserae[indexToRemove]->getName());
        return;
    }

    int fixedOverlapNum = overlapTile->OverlapFixedTesseraeIdx.size();
    int softOverlapNum = overlapTile->OverlapSoftTesseraeIdx.size();
    if (fixedOverlapNum + softOverlapNum == 2){
        // remove from overlaparr of other tess, add to tilearr
        int otherIndex;
        for (int index: overlapTile->OverlapFixedTesseraeIdx){
            if (index != indexToRemove){
                otherIndex = index;
            }
        }
        for (int index: overlapTile->OverlapSoftTesseraeIdx){
            if (index + mFixedTessNum != indexToRemove){
                otherIndex = index + mFixedTessNum;
            }
        }

        overlapTile->OverlapFixedTesseraeIdx.clear();
        overlapTile->OverlapSoftTesseraeIdx.clear();
        overlapTile->setType(tileType::BLOCK);

        Tessera* otherTess = otherIndex < mFixedTessNum ? mFP->fixedTesserae[otherIndex] : mFP->softTesserae[otherIndex - mFixedTessNum];
        removeFromVector(overlapTile, otherTess->OverlapArr);
        otherTess->TileArr.push_back(overlapTile);

        removeFromVector(overlapTile, mFP->softTesserae[indexToRemove - mFixedTessNum]->OverlapArr);
    }
    else {
        removeFromVector(overlapTile, mFP->softTesserae[indexToRemove - mFixedTessNum]->OverlapArr);
        removeFromVector(indexToRemove - mFixedTessNum, overlapTile->OverlapSoftTesseraeIdx);                                
    }
}

bool removeFromVector(int a, std::vector<int>& vec){
    for (std::vector<int>::iterator it = vec.begin() ; it != vec.end(); ++it){
        if (*it == a){
            vec.erase(it);
            return true;
        }
    }
    return false;
}

bool removeFromVector(Tile* a, std::vector<Tile*>& vec){
    for (std::vector<Tile*>::iterator it = vec.begin() ; it != vec.end(); ++it){
        if (*it == a){
            vec.erase(it);
            return true;
        }
    }
    return false;
}

void DFSLegalizer::dfs(DFSLEdge& edge, double currentCost){
    int toIndex = edge.toIndex;
    MigrationEdge edgeResult = getEdgeCost(edge);
    double edgeCost = edgeResult.edgeCost;
    currentCost += edgeCost;
    mCurrentPath.push_back(edgeResult);
    int blankStart = mFixedTessNum + mSoftTessNum + mOverlapNum;
    int blankEnd = blankStart + mBlankNum;
    if (blankStart <= toIndex && toIndex < blankEnd){
        if (currentCost < mBestCost){
            mBestPath = mCurrentPath;
            mBestCost = currentCost;
        }
    }

    if (currentCost < config.getConfigValue<double>("MaxCostCutoff")){
        for (DFSLEdge& nextEdge: mAllNodes[toIndex].edgeList){
            bool skip = false;
            for (MigrationEdge& edge: mCurrentPath){
                if (edge.fromIndex == nextEdge.toIndex || edge.toIndex == nextEdge.toIndex) {
                    skip = true;
                    break;
                }
            }
            if (skip){
                continue;
            }
            else {
                dfs(nextEdge, currentCost);
            }
        }
    }

    mCurrentPath.pop_back();
}

MigrationEdge DFSLegalizer::getEdgeCost(DFSLEdge& edge){
    enum class EDGETYPE : unsigned char { OB, BB, BW, WW, BAD_EDGE };
    EDGETYPE edgeType; 

    if (mAllNodes[edge.fromIndex].nodeType == DFSLTessType::OVERLAP && mAllNodes[edge.toIndex].nodeType == DFSLTessType::SOFT){
        edgeType = EDGETYPE::OB;
    }
    else if (mAllNodes[edge.fromIndex].nodeType == DFSLTessType::SOFT && mAllNodes[edge.toIndex].nodeType == DFSLTessType::SOFT){
        edgeType = EDGETYPE::BB;
    } 
    else if (mAllNodes[edge.fromIndex].nodeType == DFSLTessType::SOFT && mAllNodes[edge.toIndex].nodeType == DFSLTessType::BLANK){
        edgeType = EDGETYPE::BW;
    }
    else if (mAllNodes[edge.fromIndex].nodeType == DFSLTessType::BLANK && mAllNodes[edge.toIndex].nodeType == DFSLTessType::BLANK){
        edgeType = EDGETYPE::WW;
    }
    else {
        DFSLPrint(1, "Edge shouldn't exist\n");
        edgeType = EDGETYPE::BAD_EDGE;
    }

    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];
    double edgeCost = 0.0;
    Rectangle returnRectangle(0,0,1,1);
    Segment bestSegment;
    
    switch (edgeType)
    {
    case EDGETYPE::OB:
    {
        Tessera* toTess = mFP->softTesserae[edge.toIndex - mFixedTessNum];
        std::set<Tile*> overlap;
        std::set<Tile*> withoutOverlap;
        std::set<Tile*> withOverlap;

        for (Tile* tile: toTess->TileArr){
            withoutOverlap.insert(tile); 
            withOverlap.insert(tile);
        }
        for (Tile* tile: toTess->OverlapArr){
            withoutOverlap.insert(tile); 
            withOverlap.insert(tile);
        }
        
        for (Tile* tile: fromNode.tileList){
            overlap.insert(tile);
            withoutOverlap.erase(tile);
        }

        LegalInfo overlapInfo = getLegalInfo(overlap);
        LegalInfo withOverlapInfo = getLegalInfo(withOverlap);
        LegalInfo withoutOverlapInfo = getLegalInfo(withoutOverlap);

        // get area
        double overlapArea = overlapInfo.actualArea;
        double blockArea = withoutOverlapInfo.actualArea;
        double areaWeight = (overlapArea / blockArea) * config.getConfigValue<double>("OBAreaWeight");

        // get util
        double withOverlapUtil = withOverlapInfo.util;
        double withoutOverlapUtil = withoutOverlapInfo.util;
        // positive reinforcement if util is improved
        double utilCost = (withOverlapUtil < withoutOverlapUtil) ? (withoutOverlapUtil - withOverlapUtil) * config.getConfigValue<double>("OBUtilPosRein") :  
                                                        (1.0 - withoutOverlapUtil) * config.getConfigValue<double>("OBUtilWeight");


        // get value of long/short side without overlap
        double aspectRatio = withoutOverlapInfo.aspectRatio;
        aspectRatio = aspectRatio > 1.0 ? aspectRatio : 1.0/aspectRatio;
        double aspectCost = pow(aspectRatio - 1.0, 4) * config.getConfigValue<double>("OBAspWeight");

        edgeCost += areaWeight + utilCost + aspectCost;

        break;
    }

    case EDGETYPE::BB:
    {
        Polygon90Set fromBlock, toBlock;
        fromBlock.clear();
        toBlock.clear();
        TileVec2PolySet(fromNode.tileList, fromBlock);
        TileVec2PolySet(toNode.tileList, toBlock);
        LegalInfo oldFromBlockInfo = getLegalInfo(fromBlock);
        LegalInfo oldToBlockInfo = getLegalInfo(toBlock);

        // predict new rectangle
        double lowestCost = (double) LONG_MAX;
        Rectangle bestRectangle;
        int bestSegmentIndex = -1;
        for (int s = 0; s < edge.tangentSegments.size(); s++){
            Segment seg = edge.tangentSegments[s];
            Segment wall = FindNearestOverlappingInterval(seg, toBlock);
            Cord BL;
            int width;
            int height;
            if (seg.direction == DIRECTION::TOP){
                width = abs(seg.segEnd.x - seg.segStart.x); 
                int requiredHeight = ceil((double) mMigratingArea / (double) width);
                int availableHeight = wall.segStart.y - seg.segStart.y;
                assert(availableHeight > 0);
                height = availableHeight > requiredHeight ? requiredHeight : availableHeight;
                BL.x = seg.segStart.x < seg.segEnd.x ? seg.segStart.x : seg.segEnd.x ;
                BL.y = seg.segStart.y;
            }
            else if (seg.direction == DIRECTION::RIGHT){
                height = abs(seg.segEnd.y - seg.segStart.y); 
                int requiredWidth = ceil((double) mMigratingArea / (double) height);
                int availableWidth = wall.segStart.x - seg.segStart.x;
                assert(availableWidth > 0);
                width = availableWidth > requiredWidth ? requiredWidth : availableWidth;
                BL.x = seg.segStart.x;
                BL.y = seg.segStart.y < seg.segEnd.y ? seg.segStart.y : seg.segEnd.y ;
            }
            else if (seg.direction == DIRECTION::DOWN){
                width = abs(seg.segEnd.x - seg.segStart.x); 
                int requiredHeight = ceil((double) mMigratingArea / (double) width);
                int availableHeight = seg.segStart.y - wall.segStart.y;
                assert(availableHeight > 0);
                height = availableHeight > requiredHeight ? requiredHeight : availableHeight;
                BL.x = seg.segStart.x < seg.segEnd.x ? seg.segStart.x : seg.segEnd.x ;
                BL.y = seg.segStart.y - height;
            }
            else if (seg.direction == DIRECTION::LEFT) {
                height = abs(seg.segEnd.y - seg.segStart.y); 
                int requiredWidth = ceil((double) mMigratingArea / (double) height);
                int availableWidth = seg.segStart.x - wall.segStart.x;
                assert(availableWidth > 0);
                width = availableWidth > requiredWidth ? requiredWidth : availableWidth;
                BL.x = seg.segStart.x - width;
                BL.y = seg.segStart.y < seg.segEnd.y ? seg.segStart.y : seg.segEnd.y ;
            }
            else {
                std::ostringstream messageStream;
                messageStream << toNode.tileList[0]->getLowerLeft();
                DFSLPrint(0, "BB edge ( %s -> %s ) has no DIRECTION\n", fromNode.nodeName.c_str(),  messageStream.str().c_str());
            }

            Rectangle newArea(BL.x, BL.y, BL.x+width, BL.y+height);
            Polygon90Set newFromBlock = fromBlock + newArea;
            Polygon90Set newToBlock = toBlock - newArea;

            LegalInfo newFromBlockInfo = getLegalInfo(newFromBlock);
            LegalInfo newToBlockInfo = getLegalInfo(newToBlock);

            // area cost
            double areaCost = ((double) oldFromBlockInfo.actualArea / (double) oldToBlockInfo.actualArea) * config.getConfigValue<double>("BBAreaWeight");

            // get util
            double oldFromBlockUtil = oldFromBlockInfo.util;
            double newFromBlockUtil = newFromBlockInfo.util;
            double oldToBlockUtil = oldToBlockInfo.util;
            double newToBlockUtil = newToBlockInfo.util;
            // positive reinforcement if util is improved
            double utilCost, fromUtilCost, toUtilCost;
            fromUtilCost = (oldFromBlockUtil < newFromBlockUtil) ? (newFromBlockUtil - oldFromBlockUtil) * config.getConfigValue<double>("BBFromUtilPosRein") :  
                                                        (1.0 - newFromBlockUtil) * config.getConfigValue<double>("BBFromUtilWeight");
            toUtilCost = (oldToBlockUtil < newToBlockUtil) ? (newToBlockUtil - oldToBlockUtil) * config.getConfigValue<double>("BBToUtilPosRein") :  
                                                        pow(1.0 - newToBlockUtil, 2) * config.getConfigValue<double>("BBToUtilWeight");
            
            // get aspect ratio with new area
            double aspectRatio = newFromBlockInfo.aspectRatio;
            aspectRatio = aspectRatio > 1.0 ? aspectRatio : 1.0/aspectRatio;
            double arCost;
            arCost = (aspectRatio - 1.0) * config.getConfigValue<double>("BBAspWeight");

            double cost = areaCost + fromUtilCost + toUtilCost + arCost;
                            
            if (cost < lowestCost){
                lowestCost = cost;
                bestSegmentIndex = s;
                bestRectangle = newArea;
            }
        }

        bestSegment = edge.tangentSegments[bestSegmentIndex];
        returnRectangle = bestRectangle;
        edgeCost += lowestCost + config.getConfigValue<double>("BBFlatCost");

        break;
    }
    case EDGETYPE::BW:
    {
        Polygon90Set oldBlock;
        TileVec2PolySet(fromNode.tileList, oldBlock);

        LegalInfo oldBlockInfo = getLegalInfo(oldBlock);

        // predict new tile
        double lowestCost = (double) LONG_MAX;
        Rectangle bestRectangle;
        int bestSegmentIndex = -1;
        for (int s = 0; s < edge.tangentSegments.size(); s++){
            Segment seg = edge.tangentSegments[s];
            Cord BL;
            int width;
            int height;
            if (seg.direction == DIRECTION::TOP){
                width = abs(seg.segEnd.x - seg.segStart.x); 
                int requiredHeight = ceil((double) mMigratingArea / (double) width);
                height = toNode.tileList[0]->getHeight() > requiredHeight ? requiredHeight : toNode.tileList[0]->getHeight();
                BL.x = seg.segStart.x < seg.segEnd.x ? seg.segStart.x : seg.segEnd.x ;
                BL.y = seg.segStart.y;
            }
            else if (seg.direction == DIRECTION::RIGHT){
                height = abs(seg.segEnd.y - seg.segStart.y); 
                int requiredWidth = ceil((double) mMigratingArea / (double) height);
                width = toNode.tileList[0]->getWidth() > requiredWidth ? requiredWidth : toNode.tileList[0]->getWidth();
                BL.x = seg.segStart.x;
                BL.y = seg.segStart.y < seg.segEnd.y ? seg.segStart.y : seg.segEnd.y ;
            }
            else if (seg.direction == DIRECTION::DOWN){
                width = abs(seg.segEnd.x - seg.segStart.x); 
                int requiredHeight = ceil((double) mMigratingArea / (double) width);
                height = toNode.tileList[0]->getHeight() > requiredHeight ? requiredHeight : toNode.tileList[0]->getHeight();
                BL.x = seg.segStart.x < seg.segEnd.x ? seg.segStart.x : seg.segEnd.x ;
                BL.y = seg.segStart.y - height;
            }
            else if (seg.direction == DIRECTION::LEFT) {
                height = abs(seg.segEnd.y - seg.segStart.y); 
                int requiredWidth = ceil((double) mMigratingArea / (double) height);
                width = toNode.tileList[0]->getWidth() > requiredWidth ? requiredWidth : toNode.tileList[0]->getWidth();
                BL.x = seg.segStart.x - width;
                BL.y = seg.segStart.y < seg.segEnd.y ? seg.segStart.y : seg.segEnd.y ;
            }
            else {
                std::ostringstream messageStream;
                messageStream << toNode.tileList[0]->getLowerLeft();
                DFSLPrint(1, "BW edge ( %s -> %s ) has no DIRECTION\n", fromNode.nodeName.c_str(),  messageStream.str().c_str());
            }

            Rectangle newArea(BL.x, BL.y, BL.x+width, BL.y+height);
            Polygon90Set newBlock = oldBlock + newArea;

            LegalInfo newBlockInfo = getLegalInfo(newBlock);

            // get util
            double oldBlockUtil = oldBlockInfo.util;
            double newBlockUtil = newBlockInfo.util;
            // positive reinforcement if util is improved
            double utilCost;
            utilCost = (oldBlockUtil < newBlockUtil) ? (newBlockUtil - oldBlockUtil) * config.getConfigValue<double>("BWUtilPosRein") :  
                                                        (1.0 - newBlockUtil) * config.getConfigValue<double>("BWUtilWeight");

            // get aspect ratio with new area
            double aspectRatio = newBlockInfo.aspectRatio;
            aspectRatio = aspectRatio > 1.0 ? aspectRatio : 1.0/aspectRatio;
            double arCost;
            arCost = pow(aspectRatio - 1.0, 4.0) * config.getConfigValue<double>("BWAspWeight");

            double cost = utilCost + arCost;
                            
            if (cost < lowestCost){
                lowestCost = cost;
                bestSegmentIndex = s;
                bestRectangle = newArea;
            }
        }

        bestSegment = edge.tangentSegments[bestSegmentIndex];
        returnRectangle = bestRectangle;
        edgeCost += lowestCost; 

        break;
    }

    case EDGETYPE::WW:
        edgeCost += config.getConfigValue<double>("WWFlatCost");
        break;

    default:
        edgeCost += config.getConfigValue<double>("MaxCostCutoff") * 100;
        break;
    }

    // note: returnTile & bestSegment are not initialized if OB edge
    return MigrationEdge(edge.fromIndex, edge.toIndex, returnRectangle, bestSegment, edgeCost);
}

void TileVec2PolySet(std::vector<Tile*>& tileVec, Polygon90Set& polySet){
    for (Tile* tile: tileVec){
        Polygon90WithHoles tileRectangle;
        std::vector<Point> newAreaVertices = {
            {tile->getLowerLeft().x, tile->getLowerLeft().y}, 
            {tile->getLowerRight().x, tile->getLowerRight().y }, 
            {tile->getUpperRight().x, tile->getUpperRight().y}, 
            {tile->getUpperLeft().x, tile->getUpperLeft().y}
        };
        gtl::set_points(tileRectangle, newAreaVertices.begin(), newAreaVertices.end());
        polySet += tileRectangle;
    }
}

// find the segment, such that when an area is extended in the normal direction of seg will 
// collide with this segment.
Segment FindNearestOverlappingInterval(Segment& seg, Polygon90Set& poly){
    int segmentOrientation; // 0 = segments are X direction, 1 = Y direction
    Segment closestSegment = seg;
    int closestDistance = INT_MAX;
    if (seg.direction == DIRECTION::DOWN || seg.direction == DIRECTION::TOP){
        segmentOrientation = 0;
    }
    else if (seg.direction == DIRECTION::RIGHT || seg.direction == DIRECTION::LEFT){
        segmentOrientation = 1;
    }
    else {
        return seg;
    }

    std::vector<Polygon90WithHoles> polyContainer;
    // int currentDepth, currentStart, currentEnd;
    poly.get_polygons(polyContainer);
    for (Polygon90WithHoles poly: polyContainer) {
        Point firstPoint, prevPoint, currentPoint(0,0);
        int counter = 0;
        for (auto it = poly.begin(); it != poly.end(); it++){
            prevPoint = currentPoint;
            currentPoint = *it;

            if (counter++ == 0){
                firstPoint = currentPoint;
                continue; 
            }
            Segment currentSegment;
            currentSegment.segStart = prevPoint < currentPoint ? Cord(prevPoint.x(), prevPoint.y()) : Cord(currentPoint.x(), currentPoint.y());
            currentSegment.segEnd = prevPoint < currentPoint ? Cord(currentPoint.x(), currentPoint.y()) : Cord(prevPoint.x(), prevPoint.y());
            int currentOrientation = prevPoint.y() == currentPoint.y() ? 0 : 1;

            if (segmentOrientation == currentOrientation){
                switch (seg.direction)
                {
                case DIRECTION::TOP:
                {
                    // test if y coord is larger
                    bool overlaps = ((currentSegment.segStart.x < seg.segEnd.x) && (seg.segStart.x < currentSegment.segEnd.x));
                    bool isAbove = currentSegment.segStart.y > seg.segStart.y;
                    if (overlaps && isAbove){
                        int distance = currentSegment.segStart.y - seg.segStart.y;
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                case DIRECTION::RIGHT:
                {
                    // test if x coord is larger
                    bool overlaps = ((currentSegment.segStart.y < seg.segEnd.y) && (seg.segStart.y < currentSegment.segEnd.y));
                    bool isRight = currentSegment.segStart.x > seg.segStart.x;
                    if (overlaps && isRight){
                        int distance = currentSegment.segStart.x - seg.segStart.x;
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                case DIRECTION::DOWN:
                {
                    // test if y coord is smaller
                    bool overlaps = ((currentSegment.segStart.x < seg.segEnd.x) && (seg.segStart.x < currentSegment.segEnd.x));
                    bool isBelow = currentSegment.segStart.y < seg.segStart.y;
                    if (overlaps && isBelow){
                        int distance = seg.segStart.y -currentSegment.segStart.y;
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                case DIRECTION::LEFT:
                {
                    // test if x coord is smaller
                    bool overlaps = ((currentSegment.segStart.y < seg.segEnd.y) && (seg.segStart.y < currentSegment.segEnd.y));
                    bool isLeft = currentSegment.segStart.x < seg.segStart.x;
                    if (overlaps && isLeft){
                        int distance = seg.segStart.x - currentSegment.segStart.x;
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
            else {
                continue;
            }
            
        }
        //compare to first point again
        Segment lastSegment;
        lastSegment.segStart = currentPoint < firstPoint ? Cord(currentPoint.x(), currentPoint.y()) : Cord(firstPoint.x(), firstPoint.y());
        lastSegment.segEnd = currentPoint < firstPoint ? Cord(firstPoint.x(), firstPoint.y()) : Cord(currentPoint.x(), currentPoint.y());
        int lastOrientation = currentPoint.y() == firstPoint.y() ? 0 : 1;  
                 
        if (segmentOrientation == lastOrientation){
            switch (seg.direction)
            {
            case DIRECTION::TOP:
            {
                // test if y coord is larger
                bool overlaps = ((lastSegment.segStart.x < seg.segEnd.x) && (seg.segStart.x < lastSegment.segEnd.x));
                bool isAbove = lastSegment.segStart.y > seg.segStart.y;
                if (overlaps && isAbove){
                    int distance = lastSegment.segStart.y - seg.segStart.y;
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            case DIRECTION::RIGHT:
            {
                // test if x coord is larger
                bool overlaps = ((lastSegment.segStart.y < seg.segEnd.y) && (seg.segStart.y < lastSegment.segEnd.y));
                bool isRight = lastSegment.segStart.x > seg.segStart.x;
                if (overlaps && isRight){
                    int distance = lastSegment.segStart.x - seg.segStart.x;
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            case DIRECTION::DOWN:
            {
                // test if y coord is smaller
                bool overlaps = ((lastSegment.segStart.x < seg.segEnd.x) && (seg.segStart.x < lastSegment.segEnd.x));
                bool isBelow = lastSegment.segStart.y < seg.segStart.y;
                if (overlaps && isBelow){
                    int distance = seg.segStart.y -lastSegment.segStart.y;
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            case DIRECTION::LEFT:
            {
                // test if x coord is smaller
                bool overlaps = ((lastSegment.segStart.y < seg.segEnd.y) && (seg.segStart.y < lastSegment.segEnd.y));
                bool isLeft = lastSegment.segStart.x < seg.segStart.x;
                if (overlaps && isLeft){
                    int distance = seg.segStart.x - lastSegment.segStart.x;
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }   

    return closestSegment;
}

LegalInfo getLegalInfo(std::vector<Tile*>& tiles){
    LegalInfo legal;
    int min_x, max_x, min_y, max_y;
    min_x = min_y = INT_MAX;
    max_x = max_y = -INT_MAX;
    legal.actualArea = 0;
    for (Tile* tile: tiles){
        if (tile->getLowerLeft().x < min_x){
            min_x = tile->getLowerLeft().x;
        }            
        if (tile->getLowerLeft().y < min_y){
            min_y = tile->getLowerLeft().y;
        }            
        if (tile->getUpperRight().x > max_x){
            max_x = tile->getUpperRight().x;
        }            
        if (tile->getUpperRight().y > max_y){
            max_y = tile->getUpperRight().y;
        }
        legal.actualArea += tile->getArea();   
    }
    legal.width = max_x - min_x;
    legal.height = max_y - min_y;

    legal.bbArea = legal.width * legal.height; 
    legal.BL = Cord(min_x, min_y);  
    if (legal.width == 0 || legal.height == 0){
        legal.aspectRatio = INT_MAX;
    }
    else {
        legal.aspectRatio = ((double) legal.width) / ((double) legal.height);
    }
    if (legal.actualArea == 0 || legal.bbArea == 0){
        legal.util = 0;
    }
    else {
        legal.util = ((double) legal.actualArea) / ((double) legal.bbArea);
    }

    return legal;
} 

LegalInfo getLegalInfo(std::set<Tile*>& tiles){
    LegalInfo legal;
    int min_x, max_x, min_y, max_y;
    min_x = min_y = INT_MAX;
    max_x = max_y = -INT_MAX;
    legal.actualArea = 0;
    for (Tile* tile: tiles){
        if (tile->getLowerLeft().x < min_x){
            min_x = tile->getLowerLeft().x;
        }            
        if (tile->getLowerLeft().y < min_y){
            min_y = tile->getLowerLeft().y;
        }            
        if (tile->getUpperRight().x > max_x){
            max_x = tile->getUpperRight().x;
        }            
        if (tile->getUpperRight().y > max_y){
            max_y = tile->getUpperRight().y;
        }
        legal.actualArea += tile->getArea();   
    }
    legal.width = max_x - min_x;
    legal.height = max_y - min_y;

    legal.bbArea = legal.width * legal.height; 
    legal.BL = Cord(min_x, min_y);  
    if (legal.width == 0 || legal.height == 0){
        legal.aspectRatio = INT_MAX;
    }
    else {
        legal.aspectRatio = ((double) legal.width) / ((double) legal.height);
    }
    if (legal.actualArea == 0 || legal.bbArea == 0){
        legal.util = 0;
    }
    else {
        legal.util = ((double) legal.actualArea) / ((double) legal.bbArea);
    }

    return legal;
} 

// 0 : Errors
// 1 : Warnings
// 2 : Standard info
// 3 : Verbose info
void DFSLegalizer::setOutputLevel(DFSL_PRINTLEVEL level){
    config.setConfigValue<int>("OutputLevel",(int)level);
}

LegalInfo getLegalInfo(Polygon90Set& tiles){
    LegalInfo legal;
    Rectangle bbox;
    tiles.extents(bbox);

    legal.actualArea = gtl::area(tiles);
    legal.width = gtl::delta(bbox, gtl::orientation_2d_enum::HORIZONTAL);
    legal.height = gtl::delta(bbox, gtl::orientation_2d_enum::VERTICAL);
    legal.bbArea = legal.width * legal.height;
    legal.BL = Cord(gtl::xl(bbox), gtl::yl(bbox));
    if (legal.width == 0 || legal.height == 0){
        legal.aspectRatio = INT_MAX;
    }
    else {
        legal.aspectRatio = ((double) legal.width) / ((double) legal.height);
    }
    if (legal.actualArea == 0 || legal.bbArea == 0){
        legal.util = 0;
    }
    else {
        legal.util = ((double) legal.actualArea) / ((double) legal.bbArea);
    }

    return legal;
} 

Rectangle tile2Rectangle(Tile* tile){
    return Rectangle(tile->getLowerLeft().x, tile->getLowerLeft().y, tile->getUpperRight().x, tile->getUpperRight().y);
}

MigrationEdge::MigrationEdge(int from, int to, Rectangle& area, Segment& seg, double cost):
    fromIndex(from), toIndex(to), segment(seg), migratedArea(area), edgeCost(cost){ ; }

MigrationEdge::MigrationEdge():
    fromIndex(-1), toIndex(-1), segment(Segment()), migratedArea(0,0,1,1), edgeCost(0.0) { ; }


static bool compareXSegment(Segment a, Segment b){
    return a.segStart.y == b.segStart.y ? a.segStart.x < b.segStart.x : a.segStart.y < b.segStart.y ;
}

static bool compareYSegment(Segment a, Segment b){
    return a.segStart.x == b.segStart.x ? a.segStart.y < b.segStart.y : a.segStart.x < b.segStart.x ;
}

}
#include "DFSLEdge.h"

namespace DFSL {

DFSLEdge::DFSLEdge(): mFromIndex(-1), mToIndex(-1){ ; }

DFSLEdge::DFSLEdge(int from, int to): mFromIndex(from), mToIndex(to){ ; }

DFSLEdge::DFSLEdge(int from, int to, std::vector<Segment>& tangentSegments):
mFromIndex(from), mToIndex(to), mTangentSegments(tangentSegments) { ; }

int DFSLEdge::getFrom() const{
    return mFromIndex;
}

int DFSLEdge::getTo() const{
    return mToIndex;
}

std::vector<Segment>& DFSLEdge::tangentSegments(){
    return mTangentSegments;
}

}
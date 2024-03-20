#ifndef _DFSLEDGE_H_
#define _DFSLEDGE_H_

#include <vector>
#include "Segment.h"

namespace DFSL{

class DFSLEdge;

class DFSLEdge {
private:
    int mFromIndex;
    int mToIndex; 
    std::vector<Segment> mTangentSegments;
public:
    DFSLEdge();
    DFSLEdge(int from, int to);
    DFSLEdge(int from, int to, std::vector<Segment>& tangentSegments);
    int getFrom() const;
    int getTo() const;
    std::vector<Segment>& tangentSegments();
};

}

#endif
#include "Segment.h"

namespace DFSL{

Segment::Segment(Cord begin, Cord end, DIRECTION dir):
    segStart(begin), segEnd(end), direction(dir) { ; }

Segment::Segment():
    segStart(Cord(0,0)), segEnd(Cord(0,0)), direction(DIRECTION::NONE) { ; }

bool compareXSegment(Segment a, Segment b){
    return a.segStart.y() == b.segStart.y() ? a.segStart.x() < b.segStart.x() : a.segStart.y() < b.segStart.y() ;
}

bool compareYSegment(Segment a, Segment b){
    return a.segStart.x() == b.segStart.x() ? a.segStart.y() < b.segStart.y() : a.segStart.x() < b.segStart.x() ;
}

}
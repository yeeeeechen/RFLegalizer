#include "Segment.h"

namespace DFSL{

bool compareXSegment(Segment a, Segment b){
    return a.segStart.y() == b.segStart.y() ? a.segStart.x() < b.segStart.x() : a.segStart.y() < b.segStart.y() ;
}

bool compareYSegment(Segment a, Segment b){
    return a.segStart.x() == b.segStart.x() ? a.segStart.y() < b.segStart.y() : a.segStart.x() < b.segStart.x() ;
}

}
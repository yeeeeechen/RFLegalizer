#ifndef _DFSLSEGMENT_H_
#define _DFSLSEGMENT_H_

#include "fp/cord.h"

namespace DFSL {

struct Segment;
enum class DIRECTION : unsigned char { TOP, RIGHT, DOWN, LEFT, NONE };

// note: use ONLY on vertical and horizontal segments 
struct Segment {
    Cord segStart;
    Cord segEnd;
    DIRECTION direction; // direction of the normal vector of this segment
};

bool compareXSegment(Segment a, Segment b);
bool compareYSegment(Segment a, Segment b);

}

#endif
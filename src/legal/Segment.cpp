#include "Segment.h"
#include <utility>

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

            if (prevPoint > currentPoint){
                std::swap(prevPoint, currentPoint);
            }

            // direction for currentSegment doesn't matter, can be anything
            Segment currentSegment(prevPoint, currentPoint, DIRECTION::DOWN);
            int currentOrientation = prevPoint.y() == currentPoint.y() ? 0 : 1;

            if (segmentOrientation == currentOrientation){
                switch (seg.direction)
                {
                case DIRECTION::TOP:
                {
                    // test if y coord is larger
                    bool overlaps = ((currentSegment.segStart.x() < seg.segEnd.x()) && (seg.segStart.x() < currentSegment.segEnd.x()));
                    bool isAbove = currentSegment.segStart.y() > seg.segStart.y();
                    if (overlaps && isAbove){
                        int distance = currentSegment.segStart.y() - seg.segStart.y();
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
                    bool overlaps = ((currentSegment.segStart.y() < seg.segEnd.y()) && (seg.segStart.y() < currentSegment.segEnd.y()));
                    bool isRight = currentSegment.segStart.x() > seg.segStart.x();
                    if (overlaps && isRight){
                        int distance = currentSegment.segStart.x() - seg.segStart.x();
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
                    bool overlaps = ((currentSegment.segStart.x() < seg.segEnd.x()) && (seg.segStart.x() < currentSegment.segEnd.x()));
                    bool isBelow = currentSegment.segStart.y() < seg.segStart.y();
                    if (overlaps && isBelow){
                        int distance = seg.segStart.y() -currentSegment.segStart.y();
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
                    bool overlaps = ((currentSegment.segStart.y() < seg.segEnd.y()) && (seg.segStart.y() < currentSegment.segEnd.y()));
                    bool isLeft = currentSegment.segStart.x() < seg.segStart.x();
                    if (overlaps && isLeft){
                        int distance = seg.segStart.x() - currentSegment.segStart.x();
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
        if (currentPoint > firstPoint){
            std::swap(currentPoint, firstPoint);
        }
        Segment lastSegment(currentPoint, firstPoint, DIRECTION::DOWN);
        int lastOrientation = currentPoint.y() == firstPoint.y() ? 0 : 1;  
                 
        if (segmentOrientation == lastOrientation){
            switch (seg.direction)
            {
            case DIRECTION::TOP:
            {
                // test if y coord is larger
                bool overlaps = ((lastSegment.segStart.x() < seg.segEnd.x()) && (seg.segStart.x() < lastSegment.segEnd.x()));
                bool isAbove = lastSegment.segStart.y() > seg.segStart.y();
                if (overlaps && isAbove){
                    int distance = lastSegment.segStart.y() - seg.segStart.y();
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
                bool overlaps = ((lastSegment.segStart.y() < seg.segEnd.y()) && (seg.segStart.y() < lastSegment.segEnd.y()));
                bool isRight = lastSegment.segStart.x() > seg.segStart.x();
                if (overlaps && isRight){
                    int distance = lastSegment.segStart.x() - seg.segStart.x();
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
                bool overlaps = ((lastSegment.segStart.x() < seg.segEnd.x()) && (seg.segStart.x() < lastSegment.segEnd.x()));
                bool isBelow = lastSegment.segStart.y() < seg.segStart.y();
                if (overlaps && isBelow){
                    int distance = seg.segStart.y() -lastSegment.segStart.y();
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
                bool overlaps = ((lastSegment.segStart.y() < seg.segEnd.y()) && (seg.segStart.y() < lastSegment.segEnd.y()));
                bool isLeft = lastSegment.segStart.x() < seg.segStart.x();
                if (overlaps && isLeft){
                    int distance = seg.segStart.x() - lastSegment.segStart.x();
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

}
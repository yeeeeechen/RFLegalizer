#include "DFSLConfig.h"

Config::Config(): 
    maxCostCutoff(5000.0),

    // overlap -> block flow costs
    OBAreaWeight(750.0),
    OBUtilWeight(1000.0),
    OBAspWeight(100.0),
    OBUtilPosRein(-500.0),

    // block -> whitespcae flow costs
    BWUtilWeight(1500.0),
    BWUtilPosRein(-500.0),
    BWAspWeight(500.0),

    // block -> block flow costs
    BBAreaWeight(150.0),
    BBFromUtilWeight(900.0),
    BBFromUtilPosRein(-500.0),    
    BBToUtilWeight(1750.0),
    BBToUtilPosRein(-100.0),
    BBAspWeight(50.0),
    BBFlatCost(150.0),

    // whitespace -> whitespace flow costs
    WWFlatCost(1000000.0),

    // turn this on if integer division causes modules to not have required area
    exactAreaMigration(false)
    { ; }

void Config::resetDefault(){
    Config copy = Config();
    *(this) = copy;
}
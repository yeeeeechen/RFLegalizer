#ifndef _DFSCONFIG_H_
#define _DFSCONFIG_H_

struct Config {
    Config();
    void resetDefault();

    double maxCostCutoff;

    double OBAreaWeight;
    double OBUtilWeight;
    double OBAspWeight;
    double OBUtilPosRein;

    double BWUtilWeight;
    double BWUtilPosRein;
    double BWAspWeight;

    double BBAreaWeight;
    double BBFromUtilWeight;
    double BBFromUtilPosRein;    
    double BBToUtilWeight;
    double BBToUtilPosRein;
    double BBAspWeight;
    double BBFlatCost;

    double WWFlatCost;  

    bool exactAreaMigration;
    bool migrationAreaLimit;
    double maxMigrationAreaInSingleIteration;
};

#endif
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cfloat>
#include <stdio.h>

#include "LFUnits.h"
#include "Tile.h"
#include "Tessera.h"
#include "LFLegaliser.h"
#include "DFSLegalizer.h"


int main(int argc, char const *argv[]) {

    if (argc < 2){
        std::cout << "Usage: " << argv[0] << " <filename> [legal strategy 0-4] [legal mode 0-3]\n";
        return 1;
    }

    std::vector<int> legalStrategies;
    if (argc >= 3){
        legalStrategies = { atoi(argv[2]) };
    }
    else {
        legalStrategies = { 0, 1, 2, 3, 4 };
    }

    std::vector<int> legalModes;
    if (argc >= 4){
        legalModes = { atoi(argv[3]) };
    }
    else {
        legalModes = { 0, 1, 2, 3 };
    }

    LFLegaliser *legaliser = nullptr;
    double bestHpwl = DBL_MAX;
    std::string filename = argv[1];
    
    legaliser = new LFLegaliser((len_t) 1000, (len_t) 1000);

    std::cout << "Reading input from " + filename + "...\n";
    legaliser->initFromGlobalFile(filename);
    legaliser->detectfloorplanningOverlaps();

    std::cout << "Splitting overlaps ...";
    legaliser->splitTesseraeOverlaps();
    std::cout << "done!" << std::endl;

    std::cout << "Painting All Tesserae to Canvas" << std::endl;
    legaliser->arrangeTesseraetoCanvas();
    
    std::cout << "Start combinable tile search, ";
    std::vector <std::pair<Tile *, Tile *>> detectMergeTile;
    legaliser->detectCombinableBlanks(detectMergeTile);
    std::cout << detectMergeTile.size() << " candidates found" << std::endl << std::endl;
    for(std::pair<Tile *, Tile *> tp : detectMergeTile){
        legaliser->visualiseAddMark(tp.first);
        legaliser->visualiseAddMark(tp.second);
    }

    legaliser->visualiseRemoveAllmark();
    while(!detectMergeTile.empty()){
        std::cout << "Merging Pair: #" << detectMergeTile.size() << std::endl;
        detectMergeTile[0].first->show(std::cout);
        detectMergeTile[0].second->show(std::cout);

        legaliser->combineVerticalMergeableBlanks(detectMergeTile[0].first, detectMergeTile[0].second);
        detectMergeTile.clear();
        legaliser->detectCombinableBlanks(detectMergeTile);
    }


    legaliser->visualiseArtpiece("outputs/phase2.txt", true);

    std::cout << std::endl << std::endl;
    DFSL::DFSLegalizer dfsl;

    for (int legalStrategy: legalStrategies){
        for (int legalMode: legalModes){
            LFLegaliser legalizedFloorplan(*(legaliser));
            dfsl.initDFSLegalizer(&(legalizedFloorplan));

            double storeOBAreaWeight;
            double storeOBUtilWeight;
            double storeOBAspWeight;
            double storeOBUtilPosRein;
            double storeBWUtilWeight;
            double storeBWAspWeight;
            double storeBWUtilPosRein;
            double storeBBAreaWeight;
            double storeBBFromUtilWeight;
            double storeBBFromUtilPosRein;    
            double storeBBToUtilWeight;
            double storeBBToUtilPosRein;
            double storeBBAspWeight;
            double storeBBFlatCost;
            
            dfsl.config.resetDefault();
            
            if (legalStrategy == 0){
                std::cout << "legalStrategy = 0, using default configs\n";
                // default configs
            }
            else if (legalStrategy == 1){
                // prioritize area 
                std::cout << "legalStrategy = 1, prioritizing area\n";
                dfsl.config.OBAreaWeight = storeOBAreaWeight = 1400.0;
                dfsl.config.OBUtilWeight = storeOBUtilWeight = 750.0;
                dfsl.config.OBAspWeight = storeOBAspWeight = 100.0;

                dfsl.config.BWUtilWeight = storeBWUtilWeight = 750.0;
                dfsl.config.BWAspWeight = storeBWAspWeight = 500.0;
            }
            else if (legalStrategy == 2){
                // prioritize util
                std::cout << "legalStrategy = 2, prioritizing utilization\n";
                dfsl.config.OBAreaWeight = storeOBAreaWeight  = 750.0;
                dfsl.config.OBUtilWeight = storeOBUtilWeight  = 900.0;
                dfsl.config.OBAspWeight = storeOBAspWeight = 100.0;
                dfsl.config.BWUtilWeight = storeBWUtilWeight = 2000.0;
                dfsl.config.BWAspWeight = storeBWAspWeight = 500.0;
                dfsl.config.BWUtilPosRein = storeBWUtilPosRein = -1000.0;
            }
            else if (legalStrategy == 3){
                // prioritize aspect ratio
                std::cout << "legalStrategy = 3, prioritizing aspect ratio\n";
                dfsl.config.OBAreaWeight = storeOBAreaWeight  = 750.0;
                dfsl.config.OBUtilWeight = storeOBUtilWeight  = 1100.0;
                dfsl.config.OBAspWeight = storeOBAspWeight = 500.0;
                dfsl.config.BWUtilWeight = storeBWUtilWeight = 1000.0;
                dfsl.config.BWAspWeight = storeBWAspWeight = 1200.0;
            }
            else if (legalStrategy == 4){
                // favor block -> block flow more
                std::cout << "legalStrategy = 4, prioritizing block -> block flow\n";
                dfsl.config.OBAreaWeight = storeOBAreaWeight  = 1000.0;
                dfsl.config.OBUtilWeight = storeOBUtilWeight  = 1250.0;
                dfsl.config.OBAspWeight = storeOBAspWeight = 1500.0;

                dfsl.config.BWUtilWeight = storeBWUtilWeight = 1800.0;
                dfsl.config.BWAspWeight = storeBWAspWeight = 700.0;

                dfsl.config.BBAreaWeight = storeBBAreaWeight = 100.0;
                dfsl.config.BBFromUtilWeight = storeBBFromUtilWeight = 450.0;
                dfsl.config.BBToUtilWeight = storeBBToUtilWeight = 500.0;
                dfsl.config.BBAspWeight = storeBBAspWeight = 20.0;
                dfsl.config.BBFlatCost = storeBBFlatCost = -50;
            }
            std::cout << "Legalization mode: " << legalMode << std::endl;

            DFSL::RESULT legalResult = dfsl.legalize(legalMode);

            std::cout << "DSFL DONE\n";
            if (legalResult == DFSL::RESULT::SUCCESS){
                std::cout << "Checking legality..." << std::endl;
                for (Tessera* tess: legalizedFloorplan.softTesserae){
                    if (!tess->isLegal()){
                        std::cout << tess->getName() << " is not legal!" << std::endl;
                    }
                }
            } 
            else if (legalResult == DFSL::RESULT::CONSTRAINT_FAIL ) {
                std::cout << "Constraints FAIL, restarting process...\n" << std::endl;
            } 
            else {
                std::cout << "Impossible to solve, restarting process...\n" << std::endl;
            }

           legalizedFloorplan.visualiseArtpiece("outputs/legal_" + std::to_string(legalStrategy) + "_" + std::to_string(legalMode) + ".txt", true);
           
            double finalScore = legalizedFloorplan.calculateHPWL();
            printf("Final Score = %12.6f\n", finalScore);
            if (finalScore < bestHpwl){
                bestHpwl = finalScore;
                std::cout << "Best Hpwl found\n";
                legalizedFloorplan.visualiseArtpiece("outputs/legal.txt", true);
            }
        }
    }
    
    printf("best hpwl = %12.6f\n", bestHpwl);
}
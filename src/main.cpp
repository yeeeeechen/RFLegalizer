#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cfloat>
#include <stdio.h>
#include <unistd.h>
#include <ctime>

#include "LFUnits.h"
#include "Tile.h"
#include "Tessera.h"
#include "LFLegaliser.h"
#include "DFSLegalizer.h"


int main(int argc, char *argv[]) {
    int legalStrategy = 0;
    int legalMode = 0;
    std::string inputFilename = "";
    std::string casename = "";
    
    // print current time and date
    const char* cyanText = "\u001b[36m";
    const char* resetText = "\u001b[0m";
    printf("Rectilinear Floorplan Legalizer: %sO%sverlap %sM%sigration via %sG%sraph Traversal\n", cyanText, resetText, cyanText, resetText, cyanText, resetText);

    const std::time_t now = std::time(nullptr);
    std::cout << "Run at: " << std::asctime(std::localtime(&now)) << '\n';

    int cmd_opt;
    // "a": -a doesn't require argument
    // "a:": -a requires a argument
    // "a::" argument is optional for -a 
    while((cmd_opt = getopt(argc, argv, ":hi:c:m:s:")) != -1) {
        switch (cmd_opt) {
        case 'h':
            std::cout << "Usage: " << argv[0] << " [-h] [-i <input file>] [-c <case name>] [-m <legalization mode 0-3>] [-s <legalization strategy 0-4>]\n";
            return 0;
        case 'i':
            inputFilename = optarg;
            break;
        case 'c':
            casename = optarg;
            break;
        case 'm':
            legalMode = std::stoi(optarg);
            break;
        case 's':
            legalStrategy = std::stoi(optarg);
            break;
        case '?':
            fprintf(stderr, "Illegal option:-%c\n", isprint(optopt)?optopt:'#');
            break;
        case ':': // requires first char of optstring to be ':'
            fprintf(stderr, "Option -%c requires an argument\n", optopt);
        default:
            return 1;
            break;
        }
    }

    // go thru arguments not parsed by getopt
    if (argc > optind) {
        for (int i = optind; i < argc; i++) {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        }
    }

    if (inputFilename == ""){
        std::cerr << "Missing input file\n";
        return 1;
    }    

    // find casename
    if (casename == ""){
        std::size_t casenameStart = inputFilename.find_last_of("/\\");
        std::size_t casenameEnd = inputFilename.find_last_of(".");
        if (casenameStart == std::string::npos){
            casenameStart = 0;
        }
        else {
            casenameStart++;
        }
        size_t casenameLength;
        if (casenameEnd <= casenameStart || casenameEnd == std::string::npos){
            casenameLength = std::string::npos;
        }
        else {
            casenameLength = casenameEnd - casenameStart;
        }
        casename = inputFilename.substr(casenameStart, casenameLength);
    }

    std::cout << "Input file: " << inputFilename << '\n';
    std::cout << "Case Name: " << casename << '\n';
    std::cout << "Legalization mode: " << legalMode << '\n';
    std::cout << "Legalization strategy: " << legalStrategy << '\n';
    std::cout << std::endl;

    // Start measuring CPU time
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    LFLegaliser *legaliser = nullptr;
    double bestHpwl = DBL_MAX;
    
    legaliser = new LFLegaliser((len_t) 1000, (len_t) 1000);

    std::cout << "Reading input from " + inputFilename + "...\n";
    bool success = legaliser->initFromGlobalFile(inputFilename);
    if (!success){
        return 1;
    }
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


    legaliser->outputTileFloorplan("outputs/" + casename + "_init.txt", casename);

    std::cout << std::endl << std::endl;
    DFSL::DFSLegalizer dfsl;

    LFLegaliser legalizedFloorplan(*(legaliser));
    dfsl.setOutputLevel(2);
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
        dfsl.config.OBAspWeight = storeOBAspWeight = 80.0;

        dfsl.config.BWUtilWeight = storeBWUtilWeight = 750.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 80.0;
    }
    else if (legalStrategy == 2){
        // prioritize util
        std::cout << "legalStrategy = 2, prioritizing utilization\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight  = 700.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight  = 900.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 40.0;
        dfsl.config.BWUtilWeight = storeBWUtilWeight = 2000.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 40.0;
        dfsl.config.BWUtilPosRein = storeBWUtilPosRein = -1000.0;
    }
    else if (legalStrategy == 3){
        // prioritize aspect ratio
        std::cout << "legalStrategy = 3, prioritizing aspect ratio\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight  = 750.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight  = 1100.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 300.0;
        dfsl.config.BWUtilWeight = storeBWUtilWeight = 1000.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 800.0;
    }
    else if (legalStrategy == 4){
        // favor block -> block flow more
        std::cout << "legalStrategy = 4, prioritizing block -> block flow\n";
        dfsl.config.OBAreaWeight = storeOBAreaWeight  = 1000.0;
        dfsl.config.OBUtilWeight = storeOBUtilWeight  = 1250.0;
        dfsl.config.OBAspWeight = storeOBAspWeight = 400.0;

        dfsl.config.BWUtilWeight = storeBWUtilWeight = 1800.0;
        dfsl.config.BWAspWeight = storeBWAspWeight = 700.0;

        dfsl.config.BBAreaWeight = storeBBAreaWeight = 100.0;
        dfsl.config.BBFromUtilWeight = storeBBFromUtilWeight = 450.0;
        dfsl.config.BBToUtilWeight = storeBBToUtilWeight = 500.0;
        dfsl.config.BBAspWeight = storeBBAspWeight = 10.0;
        dfsl.config.BBFlatCost = storeBBFlatCost = -30;
    }

    std::cout << "Legalization mode = " << legalMode << ",";
    switch (legalMode)
    {
    case 0:
        std::cout << "resolve area big -> area small\n";
        break;
    case 1:
        std::cout << "resolve area small -> area big\n";
        break;
    case 2:
        std::cout << "resolve overlaps near center -> outer edge\n";
        break;
    case 3:
        std::cout << "completely random\n";
        break;
    default:
        break;
    }

    DFSL::RESULT legalResult = dfsl.legalize(legalMode);
    // Stop measuring CPU time
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

    std::cout << "DSFL DONE\n";
    if (legalResult == DFSL::RESULT::SUCCESS){
        std::cout << "Success lmao\n" << std::endl;
    } 
    else if (legalResult == DFSL::RESULT::CONSTRAINT_FAIL ) {
        std::cout << "Constraints FAIL\n" << std::endl;
    } 
    else {
        std::cout << "Impossible to solve\n" << std::endl;
    }

    legalizedFloorplan.outputTileFloorplan("outputs/" + casename + "_legal_" + std::to_string(legalStrategy) + "_" + std::to_string(legalMode) + ".txt", casename);
    
    double finalScore = legalizedFloorplan.calculateHPWL();
    printf("Final Score = %12.6f\n", finalScore);
    if (finalScore < bestHpwl){
        bestHpwl = finalScore;
        std::cout << "Best Hpwl found\n";
        legalizedFloorplan.outputTileFloorplan("outputs/" + casename + "_legal.txt", casename);
        legalizedFloorplan.outputFullFloorplan("outputs/" + casename + "_legal_fp.txt", casename);
    }
    
    printf("Best hpwl = %12.6f\n", bestHpwl);   

    double elapsed = ( end.tv_sec - start.tv_sec ) + ( end.tv_nsec - start.tv_nsec ) / 1e9;
    std::cout << "CPU time used: " << elapsed << " seconds." << std::endl;
}
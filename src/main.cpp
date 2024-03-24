#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cfloat>
#include <stdio.h>
#include <unistd.h>
#include <ctime>

// legalizer
#include "legal/DFSLegalizer.h"

// fp
#include "fp/cSException.h"
#include "fp/globalResult.h"
#include "fp/floorplan.h"

int main(int argc, char *argv[]) {
    int legalStrategy = 0;
    int legalMode = 0;
    std::string inputFilePath = "";
    std::string casename = "";
    std::string outputDir = "./outputs";
    std::string configFilePath = "";
    bool verbose = false;
    bool useCustomConf = false;
    
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
    while((cmd_opt = getopt(argc, argv, ":hi:o:f:c:m:s:v")) != -1) {
        switch (cmd_opt) {
        case 'h':
            std::cout << "Usage: " << argv[0] << " [-h] [-i <input file>] [-o <output directory>] [-f <floorplan name>] [-m <legalization mode 0-3>] [-s <legalization strategy 0-4>] [-c <custom .conf file>] [-v]\n";
            std::cout << "\tNote: If a custom config file (-c) is provided, then the -s option will be ignored.\n";
            return 0;
        case 'i':
            inputFilePath = optarg;
            break;
        case 'o':
            outputDir = optarg;
            break;
        case 'f':
            casename = optarg;
            break;
        case 'm':
            legalMode = std::stoi(optarg);
            break;
        case 's':
            legalStrategy = std::stoi(optarg);
            break;
        case 'c':
            configFilePath = optarg;
            useCustomConf = true;
            break;
        case 'v':
            verbose = true;
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

    if (inputFilePath == ""){
        std::cout << "Usage: " << argv[0] << " [-h] [-i <input file>] [-o <output directory>] [-f <floorplan name>] [-m <legalization mode 0-3>] [-s <legalization strategy 0-4>] [-c <custom .conf file>]\n";
        std::cerr << "Where is your input file?🤨🤨🤨\n";
        return 1;
    }    

    // find casename
    if (casename == ""){
        std::size_t casenameStart = inputFilePath.find_last_of("/\\");
        std::size_t casenameEnd = inputFilePath.find_last_of(".");
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
        casename = inputFilePath.substr(casenameStart, casenameLength);
    }

    std::cout << "Input file: " << inputFilePath << '\n';
    std::cout << "Output directory: " << outputDir << '\n';
    std::cout << "Case Name: " << casename << '\n';
    std::cout << "Legalization mode: " << legalMode << '\n';
    if (useCustomConf){
        std::cout << "Reading Configs from: " << configFilePath << '\n';
    }
    else {
        std::cout << "Legalization strategy: " << legalStrategy << '\n';
    }
    std::cout << std::endl;

    // Start measuring CPU time
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    // get global result
    std::cout << "Reading Global Input..." << std::endl;
    GlobalResult gr;
    gr.readGlobalResult(inputFilePath);

    // convert global to FP
    std::cout << "Creating Floorplan..." << std::endl;
    Floorplan fp(gr, 1.0 / ASPECT_RATIO_RULE, ASPECT_RATIO_RULE, UTIL_RULE);

    // initialize
    std::cout << "Initializing Legalizer..." << std::endl;
    DFSL::DFSLegalizer dfsl;
    dfsl.initDFSLegalizer(&(fp));

    // SETTING configs:
    dfsl.config.setConfigValue<bool>("ExactAreaMigration", true);

    // read config file 
    if (useCustomConf){
        bool success = dfsl.config.readConfigFile(configFilePath);
        if (!success){
            std::cerr << "Error opening configs\n";
            return 0;
        }
    }
    else { if (legalStrategy == 1){
            // prioritize area 
            std::cout << "legalStrategy = 1, prioritizing area\n";
            dfsl.config.readConfigFile("configs/strategy1.conf");
        }
        else if (legalStrategy == 2){
            // prioritize util
            std::cout << "legalStrategy = 2, prioritizing utilization\n";
            dfsl.config.readConfigFile("configs/strategy2.conf");
        }
        else if (legalStrategy == 3){
            // prioritize aspect ratio
            std::cout << "legalStrategy = 3, prioritizing aspect ratio\n";
            dfsl.config.readConfigFile("configs/strategy3.conf");
        }
        else if (legalStrategy == 4){
            // favor block -> block flow more
            std::cout << "legalStrategy = 4, prioritizing block -> block flow\n";
            dfsl.config.readConfigFile("configs/strategy4.conf");
        }
        else {
            std::cout << "legalStrategy = 0, using default configs\n";
            // default configs
        }
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

    // start legalizing
    // DFSL::RESULT legalResult = dfsl.legalize(legalMode);

    // Stop measuring CPU time
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

    // check legalize output
   
    // get hpwl
    // double finalScore = legaliser->calculateHPWL();
    // printf("Final Score = %12.6f\n", finalScore);
    // if (finalScore < bestHpwl){
    //     bestHpwl = finalScore;
    //     std::cout << "Best Hpwl found\n";
    //     legaliser->outputTileFloorplan(outputDir + "/" + casename + "_legal.txt", casename);
    //     legaliser->outputFullFloorplan(outputDir + "/" + casename + "_legal_fp.txt", casename);
    // }
    
    // printf("Best hpwl = %12.6f\n", bestHpwl);   

    double elapsed = ( end.tv_sec - start.tv_sec ) + ( end.tv_nsec - start.tv_nsec ) / 1e9;
    std::cout << "CPU time used: " << elapsed << " seconds." << std::endl;
}
#include <string>
#include <iostream>
#include <algorithm>

#include "carver.hpp"

using namespace std;



void usage() {
    cout << "Usage: carver OPTION... INPUT" << endl;
    cout << "Mandatory arguments:" << endl;
    cout << "-m        carve mode (both/vertical/horizontal)" << endl;
    cout << "-o        output path" << endl;
    cout << "input path has to be given as the last argument" << endl;
    cout << "Optional arguments:" << endl;
    cout << "-c        carve amount, removes given proportion of pixels from side length (0-1)" << endl;
    cout << "-v        add verbosity" << endl;
    cout << "-h        print this help" << endl;
}

[[noreturn]] void terminate(int exitStatus, string message = "Program terminated!") {
    cout << "\033[1;31m" << message << "\033[0m" << endl;
    cout << "Run carver -h for help" << endl;
    exit(exitStatus);
}


bool cmdOptionExists(char** begin, char** end, const string& option)
{
    return std::find(begin, end, option) != end;
}

char* getCmdOption(char ** begin, char ** end, const std::string & option, bool required)
{
    if (!cmdOptionExists(begin,end,option) && required)
        terminate(1, "Command line option " + option + " missing");

    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

int main(int argc, char *argv[]) {

    int optionCount = 0;

    // Request for help
    if (cmdOptionExists(argv, argv+argc, "-h")) {
            usage();
            return 0;
    }

    // Carve mode
    carver::CarveMode carveMode;
    char* carveModeOpt = getCmdOption(argv, argv+argc, "-m", true);
    if (!carveModeOpt)
        terminate(1, "Carve mode value missing");
    string carveModeStr(carveModeOpt);
    optionCount+=2;
    if (carveModeStr == "both")
        carveMode = carver::BOTH;
    else if (carveModeStr == "vertical")
        carveMode = carver::VERTICAL;
    else if (carveModeStr == "horizontal")
        carveMode = carver::HORIZONTAL;
    else
        terminate(1, "Carve mode value invalid");

    // Output path
    char* outputOpt = getCmdOption(argv, argv+argc, "-o", true);
    if (!outputOpt)
        terminate(1, "Output path missing");
    string outputStr(outputOpt);
    optionCount+=2;

    // Carve amount
    float carveAmount;
    char* carveAmountOpt = getCmdOption(argv, argv+argc, "-c", false);
    if (!carveAmountOpt)
        carveAmount = 0.15f;
    else
        try {
            carveAmount = stof(string(carveAmountOpt));
            optionCount+=2;
        } catch (invalid_argument&) {
            terminate(1, "Invalid argument for carve amount");
        }

    // Verbosity
    bool verbose = false;
    if(cmdOptionExists(argv, argv+argc, "-v")) {
        verbose = true;
        optionCount++;
    }

    if (argc < optionCount + 2)
        terminate(1, "input path not provided");
    string filename = argv[optionCount + 1];

    // Instantiate carver
    carver::Carver carver;
    carver.setVerbosity(verbose);
    carver.setCarveMode(carveMode);
    carver.setCarveAmount(carveAmount);
    if(!carver.loadTargetImage(filename)) {
        terminate(1, "Image loading failed, please provide path as the last argument");
    }

    carver.carveImage(outputStr);

    return 0;
}

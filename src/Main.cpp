
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <string>
#include <iostream>

#include "carver.hpp"

using namespace std;



void usage() {
    cout << "\nInvalid parameters, please call: " << endl;
    cout << "Carver <mode> <amount> <image>" << endl;
    cout << "Mode can be either vertical, horizontal or both" << endl;
    cout << "Amount is the target scale factor (0-1)" << endl;
}

[[noreturn]] void terminate(int exitStatus, string message = "Program terminated!") {
    cout << "\033[1;31m" << message << "\033[0m" << endl;
    usage();
    exit(exitStatus);
}

void argparse(char *input[], carver::CarveMode &carveMode, float &carveAmount) {
    // Parse carve mode
    string carveModeString = input[1];
    if (carveModeString == "vertical") carveMode = carver::CarveMode::VERTICAL;
    else if (carveModeString == "horizontal") carveMode = carver::CarveMode::HORIZONTAL;
    else if (carveModeString == "both") carveMode = carver::CarveMode::BOTH;
    else {
        terminate(1, "Invalid carve mode!");
    }

    // Parse carve amount
    try {
        carveAmount = stof(input[2]);
        if (carveAmount > 1.0f || carveAmount < 0.0f) {
            terminate(1, "Carve amount out of range!");
        }
    } catch (invalid_argument &) {
        terminate(1, "Invalid carve amount!");
    }

}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        terminate(1);
    }

    string filename = argv[3];
    carver::CarveMode carveMode;
    float carveAmount;

    // Parse command line arguments
    argparse(argv, carveMode, carveAmount);

    // Instantiate carver
    carver::Carver carver;
    if(!carver.loadTargetImage(filename)) {
        terminate(1, "Image loading failed!");
    }

    carver.setVisualMode(false);
    carver.setCarveMode(carveMode);
    carver.setCarveAmount(carveAmount);
    carver.carveImage();

    return 0;
}

#include "carver.hpp"


namespace carver {
    Carver::Carver() {

    }

    void Carver::log(string message) {
        log(message, false);
    }

    void Carver::log(string message, bool overwrite) {
        if (verbose) {
            if (overwrite)
                cout << "\r" << message << flush;
            else
                cout << message << endl;
        }

    }

    void Carver::setVerbosity(bool verbose) {
        this->verbose = verbose;
    }

    bool Carver::loadTargetImage(string filepath) {
        originalImage = cv::imread(filepath, cv::IMREAD_COLOR);
        if (originalImage.empty()) {
            return false;
        } else {
            imageCols = originalImage.cols;
            imageRows = originalImage.rows;
            log("Loaded image " + filepath + " with dimensions "
                + to_string(imageCols) + "x" + to_string(imageRows));
            return true;
        }
    }

    void Carver::setCarveMode(CarveMode carveMode) {
        this->carveMode = carveMode;
    }

    void Carver::setCarveAmount(float carveAmount) {
        if (carveAmount < 0.0f || carveAmount > 1.0f) {
            throw out_of_range("Carve amount out of range");
        } else {
            this->carveAmount = carveAmount;
        }
    }

    cv::Mat Carver::calculateEnergy(cv::Mat &source) {
        cv::Mat xGradient;
        cv::Mat yGradient;
        cv::Mat target;

        // Blur to remove minor artifacts for more stable results
        if (blur) cv::GaussianBlur(source, target, blurKernel, 0, 0, cv::BORDER_DEFAULT);

        // Calculate gradient using sobel filters
        cv::Sobel(target, xGradient, CV_16S, 1, 0, 3, sobelScale, sobelDelta, cv::BORDER_DEFAULT);
        cv::convertScaleAbs(xGradient, xGradient);
        cv::Sobel(target, yGradient, CV_16S, 0, 1, 3, sobelScale, sobelDelta, cv::BORDER_DEFAULT);
        cv::convertScaleAbs(yGradient, yGradient);

        cv::addWeighted(xGradient, 0.5, yGradient, 0.5, 0, target);
        target.convertTo(target, CV_64F, 1.0/255.0);
        return target;
    }

    void Carver::calculateCumulativePixel(int r, int c, cv::Mat &energyMap, cv::Mat &cumulativeEnergyMap) {
        double pre0, pre1, pre2;
        pre0 = cumulativeEnergyMap.at<double>(r - 1, max(c - 1, 0));
        pre1 = cumulativeEnergyMap.at<double>(r - 1, c);
        pre2 = cumulativeEnergyMap.at<double>(r - 1, min(c + 1, energyMap.cols - 1));
        cumulativeEnergyMap.at<double>(r, c) = energyMap.at<double>(r, c) + std::min(pre0, min(pre1, pre2));
    }

    cv::Mat Carver::calculateCumulativeEnergy(cv::Mat &source) {
        int sourceRows = source.rows;
        int sourceCols = source.cols;
        cv::Mat target = cv::Mat(sourceRows, sourceCols, CV_64F, double(0));
        source.row(0).copyTo(target.row(0));

        #ifdef CONCURRENT
        #pragma omp parallel for schedule(static) collapse(2) num_threads(2)
        #endif
        for (int r = 1; r < sourceRows; r++) {
            for (int c = 0; c < sourceCols; c++) {
                calculateCumulativePixel(r, c, source, target);
            }
        }

        return target;
    }

    int Carver::findMinOffset(vector<double> nextHops) {
        switch (std::min_element(nextHops.begin(),nextHops.end()) - nextHops.begin()) {
        case 0:
            return -1;
        case 1:
            return 0;
        case 2:
            return 1;
        default:
            throw out_of_range("Nexthop index out of range");
        }
    }

    vector<int> Carver::calculateLowestEnergyPath(cv::Mat &source) {
        vector<int> path(source.rows);
        vector<double> nextHops(3);
        int delta = 0;

        // Find the minimum of the last row, that is our starting point
        cv::Point minIdxPoint;
        int minIdx;
        cv::Mat lastRow = source.row(source.rows - 1);
        cv::minMaxLoc(lastRow, nullptr, nullptr, &minIdxPoint, nullptr);
        minIdx = minIdxPoint.x;
        path[source.rows - 1] = minIdx;

        for (int r = source.rows - 2; r >= 0; r--) {
            nextHops[0] = source.at<double>(r, max(minIdx - 1, 0));
            nextHops[1] = source.at<double>(r, minIdx);
            nextHops[2] = source.at<double>(r, min(minIdx + 1, source.cols - 1));

            delta = findMinOffset(nextHops);

            // Add the discovered minimum index to our path, taking care not to cross image boundaries
            minIdx += delta;
            minIdx = min(max(minIdx, 0), source.cols - 1);
            path[r] = minIdx;
        }

        return path;
    }

    vector<int> Carver::getSeamToRemove(cv::Mat &energyMap, CarveMode direction) {
        // Intermediate variables needed by the algorithm.
        // cumulativeEnergyMap is allocated statically right away to resolve performance issues
        // related to changing the matrix size while iterating.
        cv::Mat source;
        cv::Mat cumulativeEnergyMap;
        cv::Mat result;
        vector<int> lowestEnergyPath;

        // Rotate the image on horizontal operations to be able to use the same algorithms as-is
        if (direction == HORIZONTAL) {
            cv::rotate(energyMap, source, cv::ROTATE_90_CLOCKWISE);
        } else {
            source = energyMap;
        }

        cumulativeEnergyMap = calculateCumulativeEnergy(source);
        lowestEnergyPath = calculateLowestEnergyPath(cumulativeEnergyMap);

        return lowestEnergyPath;
    }

    cv::Mat Carver::removeSeam(cv::Mat &source, vector<int> seam) {
        cv::Mat target = cv::Mat(source.rows, source.cols - 1, source.type(), double(0));

        for (int r = 0; r < source.rows; r++) {
            cv::Mat a,b;
            cv::Mat result;
            source(cv::Range(r,r+1), cv::Range(0, seam[r])).copyTo(a);
            source(cv::Range(r,r+1), cv::Range(seam[r] + 1, source.cols)).copyTo(b);
            if (a.empty()) {
                result = b;
            } else if (b.empty()) {
                result = a;
            } else {
                cv::hconcat(a,b,result);
            }

            result.copyTo(target.row(r));
        }

        return target;

    }

    cv::Mat Carver::removeVerticalSeam(cv::Mat &source, vector<int> verticalSeam) {
        return removeSeam(source, verticalSeam);
    }

    cv::Mat Carver::removeHorizontalSeam(cv::Mat &source, vector<int> horizontalSeam) {
        cv::Mat flipped;
        cv::Mat target;
        cv::rotate(source, flipped, cv::ROTATE_90_CLOCKWISE);
        flipped = removeSeam(flipped, horizontalSeam);
        cv::rotate(flipped, target, cv::ROTATE_90_COUNTERCLOCKWISE);
        return target;

    }

    cv::Mat Carver::removeSeams(cv::Mat &source, vector<int> verticalSeam, vector<int> horizontalSeam) {
        cv::Mat target = source;
        target = removeVerticalSeam(source, verticalSeam);
        target = removeHorizontalSeam(target, horizontalSeam);
        return target;
    }

    void Carver::carveImage(string outputPath) {
        cv::Mat target = carveImage();
        cv::imwrite(outputPath, target);
        log("Saved output image as " + outputPath);
    }

    void Carver::printStatus(int h, int v) {
        string vStatus = v+1 > vIterations ? "READY" : to_string(v) + "/" + to_string(vIterations);
        string hStatus = h+1 > hIterations ? "READY      " : to_string(h) + "/" + to_string(hIterations);
        log("Processing column " + vStatus  + " and row " + hStatus , true);
    }

    cv::Mat Carver::carveImage() {
        // Set the iteration counters and max values
        int v = 0;
        int h = 0;
        vIterations = carveMode == BOTH || carveMode == VERTICAL ?
                    static_cast<int>(imageCols * carveAmount) : 0;
        hIterations = carveMode == BOTH || carveMode == HORIZONTAL ?
                    static_cast<int>(imageRows * carveAmount): 0;

        log("Removing " + to_string(vIterations) + " columns and " +
            to_string(hIterations) + " rows");

        cv::Mat grayscale;
        cv::Mat energyMap;
        cv::Mat target = originalImage;

        while(v < vIterations || h < hIterations) {
            cv::cvtColor(target, grayscale, cv::COLOR_BGR2GRAY);
            energyMap = calculateEnergy(grayscale);
            if (v < vIterations && h < hIterations) {
                vector<int> verticalSeam;
                vector<int> horizontalSeam;

                #ifdef CONCURRENT
                // Invoke asynchronous processing for seam pathfinding
                auto verticalSeamFuture = async(launch::async, [this, &energyMap] {
                    return getSeamToRemove(energyMap, VERTICAL);
                });
                auto horizontalSeamFuture = async(launch::async, [this, &energyMap] {
                    return getSeamToRemove(energyMap, HORIZONTAL);
                });

                // Synchronize
                verticalSeam = verticalSeamFuture.get();
                horizontalSeam = horizontalSeamFuture.get();

                #else
                verticalSeam = getSeamToRemove(energyMap, VERTICAL);
                horizontalSeam = getSeamToRemove(energyMap, HORIZONTAL);
                #endif

                target = removeSeams(target, verticalSeam, horizontalSeam);
                v++;
                h++;
            } else if (v < vIterations) {
                vector<int> verticalSeam = getSeamToRemove(energyMap, VERTICAL);
                target = removeVerticalSeam(target, verticalSeam);
                v++;
            } else if (h < hIterations) {
                vector<int> horizontalSeam = getSeamToRemove(energyMap, HORIZONTAL);
                target = removeHorizontalSeam(target, horizontalSeam);
                h++;
            }
            printStatus(h, v);
        }
        log("");
        return target;
    }
}


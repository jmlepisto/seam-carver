#include <carver.hpp>


namespace carver {
void Carver::log_(string message) {
    log_(message, false);
}

void Carver::log_(string message, bool overwrite) {
    if (verbose_) {
        if (overwrite)
            cout << "\r" << message << flush;
        else
            cout << message << endl;
    }
}

void Carver::printStatus_(int h, int v) {
    string vStatus = v+1 > vIterations_ ?
                "READY" : to_string(v) + "/" + to_string(vIterations_);
    string hStatus = h+1 > hIterations_ ?
                "READY      " : to_string(h) + "/" + to_string(hIterations_);
    log_("Processing column " + vStatus  + " and row " + hStatus , true);
}

void Carver::setVerbosity(bool verbose) {
    this->verbose_ = verbose;
}

bool Carver::loadTargetImage(string filepath) {
    originalImage_ = cv::imread(filepath, cv::IMREAD_COLOR);
    if (originalImage_.empty()) {
        return false;
    } else {
        imageCols_ = originalImage_.cols;
        imageRows_ = originalImage_.rows;
        log_("Loaded image " + filepath + " with dimensions "
            + to_string(imageCols_) + "x" + to_string(imageRows_));
        return true;
    }
}

void Carver::setCarveMode(CarveMode carveMode) {
    this->carveMode_ = carveMode;
}

void Carver::setCarveAmount(float carveAmount) {
    if (carveAmount < 0.0f || carveAmount > 1.0f) {
        throw out_of_range("Carve amount out of range");
    } else {
        this->carveAmount_ = carveAmount;
    }
}

void Carver::setCarveCount(int carveCount) {
    if (carveCount < 0) {
        throw out_of_range("Carve count out of range being negative");
    }
    this->carveCount_ = carveCount;
}

cv::Mat Carver::calculateEnergy(cv::Mat &source) {
    cv::Mat xGradient, yGradient, target;

    // Blur to remove minor artifacts for more stable results
    if (blur_) cv::GaussianBlur(source, target, blurKernel_, 0, 0,
                               cv::BORDER_DEFAULT);

    // Calculate gradient using sobel filters
    cv::Sobel(target, xGradient, CV_16S, 1, 0, 3, sobelScale_, sobelDelta_,
              cv::BORDER_DEFAULT);
    cv::convertScaleAbs(xGradient, xGradient);
    cv::Sobel(target, yGradient, CV_16S, 0, 1, 3, sobelScale_, sobelDelta_,
              cv::BORDER_DEFAULT);
    cv::convertScaleAbs(yGradient, yGradient);

    cv::addWeighted(xGradient, 0.5, yGradient, 0.5, 0, target);
    target.convertTo(target, CV_64F, 1.0/255.0);
    return target;
}

void Carver::calculateCumulativePixel_(int r, int c, cv::Mat &energyMap,
                                      cv::Mat &cumulativeEnergyMap) {
    double pre0, pre1, pre2;
    pre0 = cumulativeEnergyMap.at<double>(r - 1, max(c - 1, 0));
    pre1 = cumulativeEnergyMap.at<double>(r - 1, c);
    pre2 = cumulativeEnergyMap.at<double>(r - 1, min(c + 1, energyMap.cols - 1));
    cumulativeEnergyMap.at<double>(r, c) = energyMap.at<double>(r, c) +
            std::min(pre0, min(pre1, pre2));
}

void Carver::calculateCumulativePixelRange_(int r0, int r1,
                                           cv::Mat &energyMap,
                                           cv::Mat &cumulativeEnergyMap) {
    for (int r = r0; r < r1; r++ ) {
        for (int c = 0; c < energyMap.cols; c++) {
            calculateCumulativePixel_(r, c, energyMap, cumulativeEnergyMap);
        }
    }
}

cv::Mat Carver::calculateCumulativeEnergy(cv::Mat &energyMap) {
    vector<thread> workers;
    cv::Mat target = cv::Mat(energyMap.rows, energyMap.cols, CV_64F,
                             double(0));
    energyMap.row(0).copyTo(target.row(0));

    #ifdef CONCURRENT
    int chunkSize = energyMap.rows / N_THREADS;
    int leftOver = energyMap.rows % N_THREADS;
    for (int i = 0; i < N_THREADS; i++) {
        int startIdx = i * chunkSize + 1;
        int stopIdx;
        if (i == N_THREADS - 1) {
            stopIdx = startIdx + chunkSize + leftOver - 1;
        } else {
            stopIdx = startIdx + chunkSize - 1;
        }

        workers.push_back(
                    thread([this, startIdx, stopIdx, &energyMap, &target] {
                        return calculateCumulativePixelRange_(
                            startIdx, stopIdx, energyMap, target);
        }));
    }

    #else
    for (int r = 1; r < energyMap.rows; r++) {
        for (int c = 0; c < energyMap.cols; c++) {
            calculateCumulativePixel(r, c, energyMap, target);
        }
    }
    #endif

    for (auto &worker : workers) {
        worker.join();
    }

    return target;
}

int Carver::findMinOffset_(vector<double> nextHops) {
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

        delta = findMinOffset_(nextHops);

        // Add the discovered minimum index to our path, taking care
        // not to cross image boundaries
        minIdx += delta;
        minIdx = min(max(minIdx, 0), source.cols - 1);
        path[r] = minIdx;
    }

    return path;
}

vector<int> Carver::getSeamToRemove(cv::Mat &energyMap,
                                    CarveMode direction) {
    // Intermediate variables needed by the algorithm.
    cv::Mat source;
    cv::Mat cumulativeEnergyMap;
    cv::Mat result;
    vector<int> lowestEnergyPath;

    // Rotate the image on horizontal operations to be able to use the same
    // algorithms as-is
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
    cv::Mat target = cv::Mat(source.rows, source.cols - 1, source.type(),
                             double(0));

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

cv::Mat Carver::removeVerticalSeam(cv::Mat &source,
                                   vector<int> verticalSeam) {
    return removeSeam(source, verticalSeam);
}

cv::Mat Carver::removeHorizontalSeam(cv::Mat &source,
                                     vector<int> horizontalSeam) {
    cv::Mat flipped, target;
    cv::rotate(source, flipped, cv::ROTATE_90_CLOCKWISE);
    flipped = removeSeam(flipped, horizontalSeam);
    cv::rotate(flipped, target, cv::ROTATE_90_COUNTERCLOCKWISE);
    return target;

}

cv::Mat Carver::removeSeams(cv::Mat &source, vector<int> verticalSeam,
                            vector<int> horizontalSeam) {
    cv::Mat target = source;
    target = removeVerticalSeam(source, verticalSeam);
    target = removeHorizontalSeam(target, horizontalSeam);
    return target;
}

void Carver::carveImage(string outputPath) {
    cv::Mat target = carveImage();
    cv::imwrite(outputPath, target);
    log_("Saved output image as " + outputPath);
}

cv::Mat Carver::carveImage() {
    // Set the iteration counters and max values
    int v = 0;
    int h = 0;

    if (!carveCount_) {
        vIterations_ = carveMode_ == BOTH || carveMode_ == VERTICAL ?
                    static_cast<int>(imageCols_ * carveAmount_) : 0;
        hIterations_ = carveMode_ == BOTH || carveMode_ == HORIZONTAL ?
                    static_cast<int>(imageRows_ * carveAmount_): 0;
    } else {
        vIterations_ = carveMode_ == BOTH || carveMode_ == VERTICAL ?
                    carveCount_ : 0;
        hIterations_ = carveMode_ == BOTH || carveMode_ == HORIZONTAL ?
                    carveCount_ : 0;

        if (!(hIterations_ < imageRows_) || !(vIterations_ < imageCols_))
            throw (out_of_range("Number of pixels to carve " +
                                to_string(carveCount_) +
                                " out of range for image of size " +
                                to_string(imageCols_) + "x" +
                                to_string(imageRows_)));
    }


    log_("Removing " + to_string(vIterations_) + " columns and " +
        to_string(hIterations_) + " rows");

    cv::Mat grayscale;
    cv::Mat energyMap;
    cv::Mat target = originalImage_;

    while(v < vIterations_ || h < hIterations_) {
        cv::cvtColor(target, grayscale, cv::COLOR_BGR2GRAY);
        energyMap = calculateEnergy(grayscale);
        if (v < vIterations_ && h < hIterations_) {
            vector<int> verticalSeam;
            vector<int> horizontalSeam;

            #ifdef CONCURRENT
            // Invoke asynchronous processing for seam pathfinding
            auto verticalSeamFuture =
                    async(launch::async, [this, &energyMap] {
                        return getSeamToRemove(energyMap, VERTICAL);
                    });
            auto horizontalSeamFuture =
                    async(launch::async, [this, &energyMap] {
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
        } else if (v < vIterations_) {
            vector<int> verticalSeam =
                    getSeamToRemove(energyMap, VERTICAL);
            target = removeVerticalSeam(target, verticalSeam);
            v++;
        } else if (h < hIterations_) {
            vector<int> horizontalSeam =
                    getSeamToRemove(energyMap, HORIZONTAL);
            target = removeHorizontalSeam(target, horizontalSeam);
            h++;
        }
        printStatus_(h, v);
    }
    log_("");
    return target;
}
} // namespace carver


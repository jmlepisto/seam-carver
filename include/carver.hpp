#ifndef CARVER_HPP
#define CARVER_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>

#include <string>
#include <iostream>
#include <future>

#define CONCURRENT
#define N_THREADS 2

using namespace std;
namespace carver {
enum CarveMode {VERTICAL, HORIZONTAL, BOTH};

/**
 * @brief The Carver class is responsible for all the mathematical
 * operations needed for image carving
 * @author Joni Lepistö <joni.m.lepisto@gmail.com>
 */
class Carver
{
public:
    /**
     * @brief setVerbosity sets this class to print information on stdout
     * @param verbose verbosity state
     */
    void setVerbosity(bool verbose);

    /**
     * @brief imread loads the given image as the carving target
     * @param filepath image path
     * @return true if image loading succeeded
     */
    bool loadTargetImage(string filepath);

    /**
     * @brief setCarveMode sets carve mode for the target
     * @param carveMode carve mode to set
     */
    void setCarveMode(CarveMode carveMode) noexcept(false);

    /**
     * @brief setCarveAmount sets carve amount for the target as
     * proportional to side length
     * @param carveAmount carve amount to set (0-1)
     */
    void setCarveAmount(float carveAmount) noexcept(false);

    /**
     * @brief setCarveCount sets the carve count for the target as
     * absolute pixels
     * @param carveCount number of pixels to carve from side length
     */
    void setCarveCount(int carveCount) noexcept(false);

    /**
     * @brief carveImage runs the carving iterations and returns
     * the reduced image
     * @return reduced image
     */
    cv::Mat carveImage();

    /**
     * @brief carveImage runs the carving iterations and saves the result
     * @param outputPath result filepath
     */
    void carveImage(string outputPath);

    /**
     * @brief calculateEnergy calculates energy map for the given image
     * using crossed sobel filters
     * @param source source grayscale image
     * @return energy map
     */
    cv::Mat calculateEnergy(cv::Mat &source);

    /**
     * @brief calculateCumulativeEnergy calculates cumulative energy
     * based on the given energy map
     * @param energyMap energy map
     * @return cumulative energy map
     */
    cv::Mat calculateCumulativeEnergy(cv::Mat &energyMap);

    /**
     * @brief calculateLowestEnergyPath calculates lowest energy path from
     * top of the given cumulative energy map to the bottom
     * @param cumulativeEnergyMap cumulative energy map
     * @return vector containing the column indices of the lowest energy
     * path from top to bottom
     */
    vector<int> calculateLowestEnergyPath(cv::Mat &cumulativeEnergyMap);

    /**
     * @brief getSeamToRemove calculates the minimum energy seam from
     * the given energy map
     * @param energyMap energy map for the image
     * @param direction seam direction, VERTICAL or HORIZONTAL
     * @return vector containing the indices for the lowest energy path in
     * the given direction
     */
    vector<int> getSeamToRemove(cv::Mat &energyMap, CarveMode direction);

    /**
     * @brief removeSeam removes the given seam from the image from
     * top to bottom
     * @param source source image
     * @param seam vertical seam indices
     * @return reduced target image
     */
    cv::Mat removeSeam(cv::Mat &source, vector<int> seam);

    /**
     * @brief removeSeams removes the given seams from the source image
     * and returns a reduced version
     * @param source source image
     * @param verticalSeam vertical seam indices
     * @param horizontalSeam horizontal seam indices
     * @return reduced target image
     */
    cv::Mat removeSeams(cv::Mat &source,
                        vector<int> verticalSeam,
                        vector<int> horizontalSeam);

    /**
     * @brief removeVerticalSeam removes the given vertical seam from
     * the source image and returns a reduced version
     * @param source source image
     * @param verticalSeam vertical seam indices
     * @return reduced target image
     */
    cv::Mat removeVerticalSeam(cv::Mat &source,
                               vector<int> verticalSeam);

    /**
     * @brief removeHorizontalSeam removes the given horizontal seam from
     * the source image and returns a reduced version
     * @param source source image
     * @param horizontalSeam horizontal seam indices
     * @return reduced target image
     */
    cv::Mat removeHorizontalSeam(cv::Mat &source,
                                 vector<int> horizontalSeam);

private:
    // Variables
    cv::Mat originalImage_;
    CarveMode carveMode_;
    bool verbose_ = false;
    float carveAmount_;
    int imageRows_;
    int imageCols_;
    int vIterations_;
    int hIterations_;
    int carveCount_;

    // Image processing configuration
    bool blur_ = true;
    cv::Size blurKernel_ = cv::Size(5,5);
    const static int sobelDelta_ = 0;
    const static int sobelScale_ = 1;

    /**
     * @brief findMinOffset locates the minimum entry from the given
     * nexthop vector and returns an offset value
     * @param nextHops nexthop values
     * @return offset value: -1, 0 or 1.
     */
    int findMinOffset_(vector<double> nextHops);

    /**
     * @brief log prints out message with a newline
     * @param message to be printed
     */
    void log_(string message);

    /**
     * @brief log prints out message with optionally overwriting
     * existing stdout line
     * @param message to be printed
     * @param overwrite should stdout be overwritten
     */
    void log_(string message, bool overwrite);

    /**
     * @brief calculateCumulativePixel calculates cumulative energy
     * for a single pixel
     * @param r row
     * @param c column
     * @param energyMap energy map used for the calculation
     * @param cumulativeEnergyMap target cumulative energy map
     */
    void calculateCumulativePixel_(int r, int c, cv::Mat &energyMap,
                                  cv::Mat &cumulativeEnergyMap);

    /**
     * @brief calculateCumulativePixelRange calculates cumulative energies
     * for a range of rows
     * @param r0 start row
     * @param r1 end row
     * @param energyMap energy map used for the calculation
     * @param cumulativeEnergyMap target cumulative energy map
     */
    void calculateCumulativePixelRange_(int r0, int r1, cv::Mat &energyMap,
                                       cv::Mat &cumulativeEnergyMap);

    void printStatus_(int h, int v);



};
} // namespace carver
#endif // CARVER_HPP

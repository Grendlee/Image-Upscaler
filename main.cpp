// Enable implementation for stb_image and stb_image_write (header-only image loading/writing libraries)
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <cstdlib> // for system()
#include <filesystem>
#include <gtest/gtest.h>

// Performs bilinear interpolation on a single pixel channel (R, G, or B)
float bilinearSample(const unsigned char* imageData, int inputWidth, int inputHeight, int numChannels, float sampleX, float sampleY, int colorChannel) {
    
    //given a sampleX and sampleY floating point number from the output image
    //we construct a grid of 4 int values that represent the pixel in the input image from the sampleX and sample Y
    int x0 = static_cast<int>(floor(sampleX));
    int x1 = x0 + 1;
    int y0 = static_cast<int>(floor(sampleY));
    int y1 = y0 + 1;

    //camp values to map to actual pixel values in the input image
    x0 = std::clamp(x0, 0, inputWidth - 1);
    x1 = std::clamp(x1, 0, inputWidth - 1);
    y0 = std::clamp(y0, 0, inputHeight - 1);
    y1 = std::clamp(y1, 0, inputHeight - 1);

    //how far along the x and y directions the sample point is from x0 and y0
    float dx = sampleX - x0;
    float dy = sampleY - y0;

    //get the actual pixel value for the specific channel and x or y value
    //this is the A B C D values in the input image 
    float A = imageData[(y0 * inputWidth + x0) * numChannels + colorChannel];
    float B = imageData[(y0 * inputWidth + x1) * numChannels + colorChannel];
    float C = imageData[(y1 * inputWidth + x0) * numChannels + colorChannel];
    float D = imageData[(y1 * inputWidth + x1) * numChannels + colorChannel];

    //perform billiniar interpollation
    return A * (1 - dx) * (1 - dy) +
           B * dx * (1 - dy) +
           C * (1 - dx) * dy +
           D * dx * dy;
}

void bilinearUpscaling(const std::string& inputPath, int scaleFactor = 4) {
    int inputWidth, inputHeight, inputChannels;

    unsigned char* inputImage = stbi_load(inputPath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);

    if (!inputImage) {
        std::cerr << "Failed to load input.jpg\n";
    }


    //create empty output image
    int outputWidth = inputWidth * scaleFactor;
    int outputHeight = inputHeight * scaleFactor;

    std::vector<unsigned char> outputImage(outputWidth * outputHeight * 3);


    //for each pixel in the output image, perfomr bilinear interpolation
    for (int outputY = 0; outputY < outputHeight; ++outputY) {
        for (int outputX = 0; outputX < outputWidth; ++outputX) {
            float sampleX = outputX / static_cast<float>(scaleFactor);
            float sampleY = outputY / static_cast<float>(scaleFactor);
            
            //interpolate for each colour in the pixel
            for (int channel = 0; channel < 3; ++channel) {
                float interpolatedValue = bilinearSample(
                    inputImage,
                    inputWidth,
                    inputHeight,
                    3,
                    sampleX,
                    sampleY,
                    channel
                );

                outputImage[(outputY * outputWidth + outputX) * 3 + channel] =
                    static_cast<unsigned char>(std::clamp(interpolatedValue, 0.0f, 255.0f));
            }
        }
    }
    std::cout << "Writing image: output_bilinear.png ("
    << outputWidth << "x" << outputHeight << ")\n";
    //write the output image to disk
    stbi_write_png("output_bilinear.png", outputWidth, outputHeight, 3, outputImage.data(), outputWidth * 3);
    stbi_image_free(inputImage);

    std::cout << "Bilinear-upscaled image saved as output_bilinear.png\n";

}

// Run ESRGAN
bool runESRGAN(const std::string& inputPath, const std::string& outputPath) {
    //on linux
    std::string command = "./realesrgan-ncnn-vulkan -i " + inputPath + " -o " + outputPath + " -n realesrgan-x4plus";


    //on windows
    //std::string command = ".\\realesrgan-ncnn-vulkan.exe -i " + inputPath + " -o " + outputPath + " -n realesrgan-x4plus";
    std::cout << "ESRGAN-upscaled image saved as output_ESRGAN.png\n";
    return system(command.c_str()) == 0;
}

void nearestNeighborSampling(const std::string& inputPath, const std::string& outputPath) {
    int inputWidth, inputHeight, inputChannels;

    // Load the input image (force 3 channels: RGB)
    unsigned char* inputImage = stbi_load(inputPath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
    if (!inputImage) {
        std::cerr << "Failed to load " << inputPath << "\n";
        return;
    }

    int scaleFactor = 4;
    int outputWidth = inputWidth * scaleFactor;
    int outputHeight = inputHeight * scaleFactor;

    std::vector<unsigned char> outputImage(outputWidth * outputHeight * 3);

    // Nearest-neighbor resize (copying true pixel values)
    for (int y = 0; y < outputHeight; ++y) {
        for (int x = 0; x < outputWidth; ++x) {
            int srcX = x / scaleFactor;
            int srcY = y / scaleFactor;

            for (int c = 0; c < 3; ++c) {
                outputImage[(y * outputWidth + x) * 3 + c] =
                    inputImage[(srcY * inputWidth + srcX) * 3 + c];
            }
        }
    }

    // Save resized image
    stbi_write_png(outputPath.c_str(), outputWidth, outputHeight, 3, outputImage.data(), outputWidth * 3);
    stbi_image_free(inputImage);

    std::cout << "Nearest-neighbor resized image saved as";
    std::cout << outputPath.c_str();
    std::cout << "\n";
}


//compute the MSE to help calc PSNR
double computeMSE(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
    double sum = 0.0;

    //source and output images must be the same size
    if (a.size() != b.size()) throw std::runtime_error("Image sizes do not match for MSE");

    for (size_t i = 0; i < a.size(); ++i) {
        double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sum += diff * diff;
    }
    
    return sum / a.size();
}

//calc PSNR
double computePSNR(const std::vector<unsigned char>& groundTruth, const std::vector<unsigned char>& testImage) {
    double mse = computeMSE(groundTruth, testImage);
    
    //image a is the same as image b
    if (mse == 0) return INFINITY;

    //PSNR formula
    return 10.0 * log10((255.0 * 255.0) / mse);
}


std::vector<unsigned char> loadImage(const std::string& path, int& width, int& height, int& channels) {
    std::cerr << "Trying to load: " << path << "\n";
    
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);
    
    if (!data) {
        std::cerr << "stbi_load failed. Reason: " << stbi_failure_reason() << "\n";
        throw std::runtime_error("Failed to load " + path);
    }

    std::vector<unsigned char> vec(data, data + width * height * 3);
    stbi_image_free(data);
    return vec;
}

void computeAndPrintPSNR(const std::string& ground, const std::string& test) {
    int w1, h1, c1;
    int w2, h2, c2;
    auto img1 = loadImage(ground, w1, h1, c1);
    auto img2 = loadImage(test, w2, h2, c2);
    std::cout << computePSNR(img1, img2) << " dB\n";
    // memory automatically released when vectors go out of scope
}


TEST(UpscaleTest, inputEXISTS) {
    EXPECT_TRUE(std::filesystem::exists("input.jpg")) 
        << "input.jpg not found. Need input.jpg to run the program.";
}

TEST(UpscaleTest, input_compressedEXISTS) {
    EXPECT_TRUE(std::filesystem::exists("input_compressed.jpg")) 
        << "input_compressed.jpg not found. Need input_compressed.jpg to run the program.";
}


// // Google Test: Bilinear PSNR
// TEST(UpscaleTest, BilinearUpscaleIsBetterThanInput) {
//     int w_input, h_input, c_input;
//     int w_upscaled, h_upscaled, c_upscaled;
//     int w_ground, h_ground, c_ground;

//     // Load images
//     auto inputImage    = loadImage("input.jpg", w_input, h_input, c_input);
//     auto upscaledImage = loadImage("output.png", w_upscaled, h_upscaled, c_upscaled);
//     auto groundTruth   = loadImage("source.png", w_ground, h_ground, c_ground);

//     // Check dimensions
//     ASSERT_EQ(w_upscaled, w_ground);
//     ASSERT_EQ(h_upscaled, h_ground);
//     ASSERT_EQ(c_upscaled, c_ground);

//     // Resize input.jpg to match source.png size (basic nearest-neighbor for fair PSNR)
//     std::vector<unsigned char> resizedInput(w_ground * h_ground * c_ground);
//     for (int y = 0; y < h_ground; ++y) {
//         for (int x = 0; x < w_ground; ++x) {
//             int srcX = static_cast<int>(x / static_cast<float>(w_ground) * w_input);
//             int srcY = static_cast<int>(y / static_cast<float>(h_ground) * h_input);
//             for (int c = 0; c < c_ground; ++c) {
//                 resizedInput[(y * w_ground + x) * c_ground + c] =
//                     inputImage[(srcY * w_input + srcX) * c_ground + c];
//             }
//         }
//     }

//     // Compute PSNRs
//     double psnr_input    = computePSNR(resizedInput, groundTruth);
//     double psnr_upscaled = computePSNR(upscaledImage, groundTruth);

//     std::cout << "PSNR(input.jpg → source.png):   " << psnr_input    << " dB\n";
//     std::cout << "PSNR(output.png → source.png):  " << psnr_upscaled << " dB\n";

//     // Assert that upscaling actually improved PSNR
//     EXPECT_GT(psnr_upscaled, psnr_input) << "Upscaling made quality worse than original input.";
// }

// TEST(UpscaleTest, ESRGANIsBetterThanInput) {
//     int w_input, h_input, c_input;
//     int w_esrgan, h_esrgan, c_esrgan;
//     int w_source, h_source, c_source;

//     // Load images
//     auto inputImage    = loadImage("input.jpg", w_input, h_input, c_input);
//     auto esrganImage   = loadImage("output_esrgan.png", w_esrgan, h_esrgan, c_esrgan);
//     auto groundTruth   = loadImage("source.png", w_source, h_source, c_source);

//     // Check ESRGAN output matches ground truth resolution
//     ASSERT_EQ(w_esrgan, w_source);
//     ASSERT_EQ(h_esrgan, h_source);
//     ASSERT_EQ(c_esrgan, c_source);

//     // Resize input.jpg to match source.png size (nearest-neighbor)
//     std::vector<unsigned char> resizedInput(w_source * h_source * c_source);
//     for (int y = 0; y < h_source; ++y) {
//         for (int x = 0; x < w_source; ++x) {
//             int srcX = static_cast<int>(x / static_cast<float>(w_source) * w_input);
//             int srcY = static_cast<int>(y / static_cast<float>(h_source) * h_input);
//             for (int c = 0; c < c_source; ++c) {
//                 resizedInput[(y * w_source + x) * c_source + c] =
//                     inputImage[(srcY * w_input + srcX) * c_source + c];
//             }
//         }
//     }

//     // Compute PSNR
//     double psnr_input   = computePSNR(resizedInput, groundTruth);
//     double psnr_esrgan  = computePSNR(esrganImage, groundTruth);

//     std::cout << "PSNR(input.jpg → source.png):      " << psnr_input  << " dB\n";
//     std::cout << "PSNR(output_esrgan.png → source):  " << psnr_esrgan << " dB\n";

//     // Assert ESRGAN is better
//     EXPECT_GT(psnr_esrgan, psnr_input) << "ESRGAN upscaling was worse than original input.";
// }


int main(int argc, char** argv) {

    // run tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    
    // run bilinear upscaler
    bilinearUpscaling("input_compressed.jpg");

    // run ESRGAN
    std::cout << "Running ESRGAN...\n";
    if (!runESRGAN("input_compressed.jpg", "output_esrgan.png")) {
        std::cerr << "ESRGAN failed to run\n";
        return 1;
    }

    //for visual comparison of upscaled images
    nearestNeighborSampling("input_compressed.jpg", "resized_true_input_compressed.png");

    //to calc PSNR
    nearestNeighborSampling("input.jpg", "resized_true_input.png");



    //load images for PSNR calculation
    int w1, h1, c1;
    int w2, h2, c2;

    std::cerr << std::endl;
    // Load ground truth image
    auto groundTruth = loadImage("resized_true_input.png", w1, h1, c1);

    auto upscaledImage = loadImage("output_bilinear.png", w2, h2, c2);
    
    if (w1 != w2 || h1 != h2 || c1 != c2) {
        std::cerr << "Image dimensions do not match\n";
    } else {
        std::cout << "PSNR for output_bilinear.png: " << computePSNR(groundTruth, upscaledImage) << " dB\n";
    }
    

    if (!std::filesystem::exists("output_esrgan.png")) {
        std::cerr << "output_esrgan.png was not generated. Skipping PSNR.\n";
        return 1;
    }
    
     

    std::cerr << std::endl;
    upscaledImage = loadImage("output_esrgan.png", w2, h2, c2);
    std::cout << "ESRGAN size: " << w1 << "x" << h1 << std::endl;
    std::cout << "Ground truth size: " << w2 << "x" << h2 << std::endl;

    // check if images have same dimensions
    if (w1 != w2 || h1 != h2 || c1 != c2) {
        std::cerr << "Images must have the same dimensions for PSNR\n";
    } else { //
        std::cout << "The PSNR for output_esrgan.png is: " << computePSNR(groundTruth, upscaledImage) << " dB\n";
    }

    std::cerr << std::endl;
    upscaledImage = loadImage("resized_true_input.png", w2, h2, c2);

    // check if images have same dimensions
    if (w1 != w2 || h1 != h2 || c1 != c2) {
        std::cerr << "Images must have the same dimensions for PSNR\n";
    } else { //
        std::cout << "The PSNR for resized_true_input.png is: " << computePSNR(groundTruth, upscaledImage) << " dB\n";
        std::cout << "If PSNR is inf, this is expected as there are no differences between the two images";
    }

    // // run tests
    // ::testing::InitGoogleTest(&argc, argv);
    // return RUN_ALL_TESTS();
}

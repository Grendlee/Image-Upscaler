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
// #include <gtest/gtest.h>

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

void bilinearUpscaling(const std::string& inputPath, const std::string& outputPath, int scaleFactor = 2) {
    int inputWidth, inputHeight, inputChannels;

    unsigned char* inputImage = stbi_load("input.jpg", &inputWidth, &inputHeight, &inputChannels, 3);

    if (!inputImage) {
        std::cerr << "Failed to load input.jpg\n";
        return 1;
    }


    //create empty output image
    int scaleFactor = 2;
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
    //write the output image to disk
    stbi_write_png("output.png", outputWidth, outputHeight, 3, outputImage.data(), outputWidth * 3);
    stbi_image_free(inputImage);

    std::cout << "Bilinear-upscaled image saved as output.png\n";

}

// Run ESRGAN
bool runESRGAN(const std::string& inputPath, const std::string& outputPath) {
    std::string command = ".\\realesrgan-ncnn-vulkan.exe -i " + inputPath + " -o " + outputPath + " -n realesrgan-x4plus";
    return system(command.c_str()) == 0;
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
double computePSNR(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
    double mse = computeMSE(a, b);
    
    //image a is the same as image b
    if (mse == 0) return INFINITY;

    //PSNR formula
    return 10.0 * log10((255.0 * 255.0) / mse);
}


//load image and return raw pixel data as a vector
std::vector<unsigned char> loadImage(const std::string& path, int& width, int& height, int& channels) {
    
    //load image
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);
    
    if (!data) throw std::runtime_error("Failed to load " + path);
    
    std::vector<unsigned char> vec(data, data + width * height * 3);
    
    stbi_image_free(data);
    
    return vec;
}

// Google Test: Bilinear PSNR
TEST(UpscaleTest, BilinearUpscaleIsBetterThanInput) {
    int w_input, h_input, c_input;
    int w_upscaled, h_upscaled, c_upscaled;
    int w_ground, h_ground, c_ground;

    // Load images
    auto inputImage    = loadImage("input.jpg", w_input, h_input, c_input);
    auto upscaledImage = loadImage("output.png", w_upscaled, h_upscaled, c_upscaled);
    auto groundTruth   = loadImage("source.png", w_ground, h_ground, c_ground);

    // Check dimensions
    ASSERT_EQ(w_upscaled, w_ground);
    ASSERT_EQ(h_upscaled, h_ground);
    ASSERT_EQ(c_upscaled, c_ground);

    // Resize input.jpg to match source.png size (basic nearest-neighbor for fair PSNR)
    std::vector<unsigned char> resizedInput(w_ground * h_ground * c_ground);
    for (int y = 0; y < h_ground; ++y) {
        for (int x = 0; x < w_ground; ++x) {
            int srcX = static_cast<int>(x / static_cast<float>(w_ground) * w_input);
            int srcY = static_cast<int>(y / static_cast<float>(h_ground) * h_input);
            for (int c = 0; c < c_ground; ++c) {
                resizedInput[(y * w_ground + x) * c_ground + c] =
                    inputImage[(srcY * w_input + srcX) * c_ground + c];
            }
        }
    }

    // Compute PSNRs
    double psnr_input    = computePSNR(resizedInput, groundTruth);
    double psnr_upscaled = computePSNR(upscaledImage, groundTruth);

    std::cout << "PSNR(input.jpg → source.png):   " << psnr_input    << " dB\n";
    std::cout << "PSNR(output.png → source.png):  " << psnr_upscaled << " dB\n";

    // Assert that upscaling actually improved PSNR
    EXPECT_GT(psnr_upscaled, psnr_input) << "Upscaling made quality worse than original input.";
}

TEST(UpscaleTest, ESRGANIsBetterThanInput) {
    int w_input, h_input, c_input;
    int w_esrgan, h_esrgan, c_esrgan;
    int w_source, h_source, c_source;

    // Load images
    auto inputImage    = loadImage("input.jpg", w_input, h_input, c_input);
    auto esrganImage   = loadImage("output_esrgan.png", w_esrgan, h_esrgan, c_esrgan);
    auto groundTruth   = loadImage("source.png", w_source, h_source, c_source);

    // Check ESRGAN output matches ground truth resolution
    ASSERT_EQ(w_esrgan, w_source);
    ASSERT_EQ(h_esrgan, h_source);
    ASSERT_EQ(c_esrgan, c_source);

    // Resize input.jpg to match source.png size (nearest-neighbor)
    std::vector<unsigned char> resizedInput(w_source * h_source * c_source);
    for (int y = 0; y < h_source; ++y) {
        for (int x = 0; x < w_source; ++x) {
            int srcX = static_cast<int>(x / static_cast<float>(w_source) * w_input);
            int srcY = static_cast<int>(y / static_cast<float>(h_source) * h_input);
            for (int c = 0; c < c_source; ++c) {
                resizedInput[(y * w_source + x) * c_source + c] =
                    inputImage[(srcY * w_input + srcX) * c_source + c];
            }
        }
    }

    // Compute PSNR
    double psnr_input   = computePSNR(resizedInput, groundTruth);
    double psnr_esrgan  = computePSNR(esrganImage, groundTruth);

    std::cout << "PSNR(input.jpg → source.png):      " << psnr_input  << " dB\n";
    std::cout << "PSNR(output_esrgan.png → source):  " << psnr_esrgan << " dB\n";

    // Assert ESRGAN is better
    EXPECT_GT(psnr_esrgan, psnr_input) << "ESRGAN upscaling was worse than original input.";
}


int main(int argc, char** argv) {

    // run bilinear upscaler
    performBilinearUpscaling("input.jpg", "output.png");
    std::cout << "Bilinear upscaled image saved as output.png\n";

    // run ESRGAN
    std::cout << "Running ESRGAN...\n";
    if (!runESRGAN("output.png", "output_esrgan.png")) {
        std::cerr << "ESRGAN failed to run\n";
        return 1;
    }
    std::cout << "ESRGAN output saved as output_esrgan.png\n";

    // run tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
# Image-Upscaler

# Upscaling methods
1. Bilinear interpolation (A great video explanation: https://www.youtube.com/watch?v=AqscP7rc8_M)
2. ESRGAN

# Testing Upscaling accuracy.
1. PSNR
2. SSIM




# Compile 



# Windows

Start Menu → Search:

Developer Command Prompt for VS 2022


cl main.cpp /EHsc /Feupscaler.exe /std:c++17


PSNR (dB) | Image Quality | Notes
∞ | Perfect match | Pixel-perfect identical images
> 40 | Excellent | Nearly indistinguishable by human eye
30–40 | Good | Slight difference, acceptable quality
20–30 | Poor | Noticeable degradation
< 20 | Bad | Heavily distorted
# 🖼️ Image-Upscaler

This project performs image upscaling using two different methods:

---

## Upscaling Methods

1. **Bilinear Interpolation**
   - A simple, fast method that blends pixels linearly in both directions.
   - 📺 [Video Explanation](https://www.youtube.com/watch?v=AqscP7rc8_M)

2. **Pre-trained ESRGAN (Enhanced Super Resolution GAN)**
   - A deep learning-based upscaling method that produces photorealistic details.
   - External Pre-trained ESRGAN found here: **[Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN/?tab=readme-ov-file)** for deep-learning-based super-resolution.
     
3. **True Pixel Resize (Nearest Neighbour)**
   - Directly replicates pixel values to scale the image.
   - Fastest method with no interpolation.
   - Useful for testing baseline performance or preserving sharp edges in pixel art.
   - Needed to compare peak-signal-to-noise-ration (PSNR) later.

---

## Accuracy Testing

To evaluate upscaling quality, this project uses:

1. **PSNR (Peak Signal-to-Noise Ratio)**  
   - Measures how close the upscaled image is to a known high-resolution image.
   - To compare the PSNR we need to resize the original input.jpg with nearest neighbour by a factor of 4, since both our bilinear upscaling and ESRGAN also upscale by a factor of 4. This way we can compare pixel-by-pixel noise.
   - Higher is better.  
   - A value above **30 dB** is generally considered good. A value above **40 dB** is considered excellent, indicating that the upscaled image is nearly indistinguishable from the original high-resolution image.

2. **SSIM (Structural Similarity Index)** (coming soon)  
   - Compares perceptual differences like edges and contrast.

Tests verify whether the upscaled image (bilinear or ESRGAN) is **closer to the high-res ground truth** than the original low-res input.

---

## Google Test Integration

Google Test is used to:

- Compare `input.jpg` to `source.png` (ground truth)
- Compare `output.png` and `output_esrgan.png` to `source.png`
- Assert that:
  - `PSNR(output.png) > PSNR(input.jpg)`
  - `PSNR(output_esrgan.png) > PSNR(input.jpg)`

---

## Compile

Use the following command to compile with g++:

```cmd
g++ -std=c++17 -O2 -o upscaler main.cpp
```

dev notes:

#ifndef STBI_MAX_DIMENSIONS
#define STBI_MAX_DIMENSIONS 16384
#endif

changed to  1<< 30

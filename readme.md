# Image-Upscaler

This project performs image upscaling using three different methods:

---

## Upscaling Methods

1. **Bilinear Interpolation**
   - A fast, traditional method that linearly blends pixels along horizontal and vertical axes.
   - 📺 [Video Explanation](https://www.youtube.com/watch?v=AqscP7rc8_M)

2. **Pre-trained ESRGAN (Enhanced Super Resolution GAN)**
   - A deep learning-based method producing photorealistic upscaled images.
   - External model: [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN/?tab=readme-ov-file)

3. **True Pixel Resize (Nearest Neighbour)**
   - Fastest method; replicates pixels exactly.
   - Useful for comparing pure pixel-level similarity, especially for PSNR baseline.

---

## Accuracy Testing

To evaluate the quality of upscaled images, the following metrics are used:

### 1. **PSNR (Peak Signal-to-Noise Ratio)**
   - Measures how numerically close an upscaled image is to a ground-truth high-resolution image.
   - All upscaled images are compared against a **4× nearest-neighbour-resized version** of the original high-res input for fair comparison.
   - Typical interpretation:
     - Above **30 dB** = good quality
     - Above **40 dB** = visually near-identical to ground truth

>  **Note:** Despite being more advanced, **ESRGAN can show lower PSNR than bilinear**.  
> This is because ESRGAN introduces **hallucinated high-frequency textures** to improve perceptual quality — which increases pixel-level difference, even when it **looks better** to the human eye.  
> Therefore, PSNR alone is **not a reliable metric** for GAN-based methods.

### 2. **LPIPS (Learned Perceptual Image Patch Similarity)** _(planned)_
   - Measures perceptual similarity using a deep neural network trained on human preference judgments.
   - Will be integrated via a **Python subprocess call** from C++, comparing ESRGAN and bilinear outputs to the resized ground truth.
   - Significantly better at judging perceptual quality for GAN-based outputs.

---

## Google Test Integration

Tests include:
- Comparing outputs (`output_bilinear.png`, `output_esrgan.png`) to `resized_true_input.png`
- Asserting:
  - `PSNR(output_bilinear.png) > PSNR(input.jpg)`
  - `PSNR(output_esrgan.png) > PSNR(input.jpg)` _(note: may not always hold due to GAN artifacts)_

---

## Compilation
 **Note:** Loading images on windows will cause a memory problem at "loadImage". Compiling using a Linux system is recommended.

```bash
g++ -std=c++17 -O2 -o upscaler main.cpp
```

---

## Platform-Specific Instructions

This project supports both **macOS/Linux** and **Windows**. The ESRGAN upscaling backend uses a precompiled binary (`realesrgan-ncnn-vulkan`) that differs by platform.

In the runESRGAN(), make sure to uncomment the correct version according to your operating system.


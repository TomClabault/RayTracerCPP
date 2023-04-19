#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include "image.h"

#include <cmath>
#include <iostream>

class ImageUtils
{
public:

    /**
     * Adapted from https://gist.github.com/marukrap/
     *
     * @brief Convert HSL to RGB color space
     * Converts a given set of HSL values `h', `s', `v' into RGB
     * coordinates. The output RGB values are in the range [0, 1], and
     * the input HSL values are in the ranges h = [0, 360], and s, v =
     * [0, 1], respectively.
     *
     * @param R Red component, used as output, range: [0, 1]
     * @param G Green component, used as output, range: [0, 1]
     * @param B Blue component, used as output, range: [0, 1]
     * @param H Hue component, used as input, range: [0, 360]
     * @param S Saturation component, used as input, range: [0, 1]
     * @param V Lightness component, used as input, range: [0, 1]
     */
    static void HSLtoRGB(float& R, float& G, float& B, float& H, float& S, float& L) {
        float C = (1 - std::fabs(2 * L - 1)) * S; // Chroma
        float HPrime = H / 60; // H'
        float X = C * (1 - std::fabs(std::fmod(HPrime, 2.f) - 1));
        float M = L - C / 2;

        R = 0.f;
        G = 0.f;
        B = 0.f;

        switch (static_cast<int>(HPrime))
        {
        case 0: R = C; G = X;        break; // [0, 1)
        case 1: R = X; G = C;        break; // [1, 2)
        case 2:        G = C; B = X; break; // [2, 3)
        case 3:        G = X; B = C; break; // [3, 4)
        case 4: R = X;        B = C; break; // [4, 5)
        case 5: R = C;        B = X; break; // [5, 6)
        }

        R += M;
        G += M;
        B += M;
    }

    //! Downscales an image by the given factor and stores the result in the downscaled_output parameter.
    static void downscale_image(const Image& input_image, Image& downscaled_output, const int factor)
    {
        if (input_image.width() % factor != 0)
        {
            std::cerr << "Image width isn't divisible by the factor";

            return;
        }

        if (input_image.height() % factor != 0)
        {
            std::cerr << "Image height isn't divisible by the factor";

            return;
        }


        int downscaled_width = input_image.width() / factor;
        int downscaled_height = input_image.height() / factor;
        downscaled_output = Image(downscaled_width, downscaled_height);

#pragma omp parallel for
        for (int y = 0; y < downscaled_height; y++)
        {
            for (int x = 0; x < downscaled_width; x++)
            {
                Color average;

                for (int i = 0; i < factor; i++)
                    for (int j = 0; j < factor; j++)
                        average = average + input_image(x * factor + j, y * factor + i);

                average = average / (factor * factor);

                downscaled_output(x, y) = average;
            }
        }
    }
};

#endif

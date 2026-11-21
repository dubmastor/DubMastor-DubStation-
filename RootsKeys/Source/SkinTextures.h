// SkinTextures.h — texturas procedurais (geradas uma vez, sem imagens externas)
//  - plástico preto granulado e fosco (teclado anos 80)
//  - madeira lisa de veios suaves (gabinete estilo Hammond)
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <vector>

namespace skin
{
// ruído de valor 2D determinístico (mesma textura sempre)
struct ValueNoise
{
    static float hash (int x, int y, int seed)
    {
        uint32_t h = (uint32_t) x * 374761393u + (uint32_t) y * 668265263u + (uint32_t) seed * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float) (h & 0xffffff) / 16777215.0f;
    }
    static float smooth (float t) { return t * t * (3.0f - 2.0f * t); }
    static float at (float x, float y, int seed)
    {
        const int xi = (int) std::floor (x), yi = (int) std::floor (y);
        const float fx = smooth (x - (float) xi), fy = smooth (y - (float) yi);
        const float a = hash (xi, yi, seed), b = hash (xi + 1, yi, seed);
        const float c = hash (xi, yi + 1, seed), d = hash (xi + 1, yi + 1, seed);
        return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
    }
    static float fbm (float x, float y, int seed, int octaves)
    {
        float sum = 0, amp = 0.5f, norm = 0;
        for (int o = 0; o < octaves; ++o)
        {
            sum += amp * at (x, y, seed + o * 17);
            norm += amp;
            x *= 2.03f; y *= 2.03f; amp *= 0.5f;
        }
        return sum / norm;
    }
};

inline juce::uint8 toByte (float v) { return (juce::uint8) juce::jlimit (0, 255, (int) std::lround (v)); }

// Plástico preto texturizado: relevo granulado iluminado de cima/esquerda + pontinhos
inline juce::Image makePlastic (int w, int h, float baseGrey = 30.0f)
{
    juce::Image img (juce::Image::RGB, w, h, false);
    std::vector<float> hm ((size_t) (w * h));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            hm[(size_t) (y * w + x)] = ValueNoise::fbm ((float) x * 0.55f, (float) y * 0.55f, 4, 3);

    juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const int x0 = juce::jmax (0, x - 1), x1 = juce::jmin (w - 1, x + 1);
            const int y0 = juce::jmax (0, y - 1), y1 = juce::jmin (h - 1, y + 1);
            const float dx = hm[(size_t) (y * w + x1)] - hm[(size_t) (y * w + x0)];
            const float dy = hm[(size_t) (y1 * w + x)] - hm[(size_t) (y0 * w + x)];
            float v = baseGrey + (-dx - dy) * 55.0f;                         // luz vinda de cima-esquerda
            v += (ValueNoise::hash (x, y, 99) > 0.985f ? 14.0f : 0.0f);      // pontinhos claros
            v += (ValueNoise::fbm ((float) x * 0.01f, (float) y * 0.01f, 7, 2) - 0.5f) * 6.0f; // variação suave
            const auto b = toByte (v);
            bd.setPixelColour (x, y, juce::Colour (b, b, (juce::uint8) juce::jmin (255, b + 2)));
        }
    return img;
}

// Madeira simples: veios longos horizontais, cor de nogueira média, acabamento acetinado
inline juce::Image makeWood (int w, int h)
{
    juce::Image img (juce::Image::RGB, w, h, false);
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
    const juce::Colour light (0xff8a5a32), dark (0xff5e3a1e);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const float warp = ValueNoise::fbm ((float) x * 0.0025f, (float) y * 0.02f, 11, 3);
            const float rings = (float) y * 0.11f + warp * 9.0f;
            float s = 0.5f + 0.5f * std::sin (rings);
            s = std::pow (s, 3.0f);                                           // veio fino e escuro
            const float fine = ValueNoise::fbm ((float) x * 0.02f, (float) y * 0.6f, 23, 2);
            float t = juce::jlimit (0.0f, 1.0f, 0.25f + 0.45f * s + 0.3f * (fine - 0.5f));
            const auto c = light.interpolatedWith (dark, t);
            bd.setPixelColour (x, y, c);
        }
    return img;
}
} // namespace skin

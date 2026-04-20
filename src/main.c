#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define WIDTH      512
#define HEIGHT     512
#define NUM_PIXELS (WIDTH) * (HEIGHT)

#define RGB_HEX(hex) (rgb) {((hex) >> 16) & 0xFF, ((hex) >> 8) & 0xFF, (hex) & 0xFF}
#define HONEYDEW     0xF0FFF0
#define NAVY         0x111827
#define INDIGO       0x6366F1
#define ROSE         0xEC4899
#define AMBER        0xFBBF24
#define TEAL         0x2DD4BF
#define SLATE        0x1E2D40

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define ROUNDED_BOX_RADIUS 64.0f

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb;

void save_as_ppm(const char *path, rgb *pixels, size_t width, size_t height)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open file '%s', error: %s", path, strerror(errno));
        return;
    }

    fprintf(fp, "P6 %zu %zu 255\n", width, height);
    if (fwrite(pixels, sizeof(rgb), width * height, fp) < width * height)
    {
        fprintf(stderr, "Failed to write pixels to file '%s', error: %s", path, strerror(errno));
        return;
    }

    if (fclose(fp) == EOF)
    {
        fprintf(stderr, "Failed to close file '%s', error: %s", path, strerror(errno));
        return;
    }

    printf("Generated '%s'!\n", path);
}

void fill_pixels(rgb *pixels, size_t num_pixels, rgb color)
{
    for (size_t i = 0; i < num_pixels; ++i)
    {
        pixels[i] = color;
    }
}

float sdf_circle(float px, float py, float cx, float cy, float r)
{
    return sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy)) - r;
}

float sdf_box(float px, float py, float cx, float cy, float hw, float hh)
{
    float qx = fabsf(cx - px) - hw;
    float qy = fabsf(cy - py) - hh;

    float ex      = MAX(qx, 0);
    float ey      = MAX(qy, 0);
    float outside = sqrtf((ex * ex) + (ey * ey));
    float inside  = MIN(MAX(qx, qy), 0);

    return outside + inside;
}

float sdf_rounded_box(float px, float py, float cx, float cy, float hw, float hh, float r)
{
    return sdf_box(px, py, cx, cy, hw, hh) - r;
}

float sdf_union(float a, float b)
{
    return MIN(a, b);
}

float sdf_intersection(float a, float b)
{
    return MAX(a, b);
}

float sdf_subtraction(float a, float b)
{
    return MAX(a, -b);
}

float clamp(float value, float low, float high)
{
    return MAX(low, MIN(value, high));
}

float smoothstep(float edge0, float edge1, float t)
{
    t = clamp((t - edge0) / (edge1 - edge0), 0, 1);

    return t * t * (3 - 2 * t);
}

void circle_hard(rgb *pixels, size_t width, size_t height, rgb bg, rgb fg)
{
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float d               = sdf_circle(x, y, width / 2.0f, height / 2.0f, width / 4.0f);
            pixels[y * width + x] = d <= 0 ? fg : bg;
        }
    }
}

rgb lerp_color(rgb a, rgb b, float t)
{
    t = clamp(t, 0.0f, 1.0f);
    return (rgb) {
        .r = (uint8_t) ((float) a.r * (1.0f - t) + (float) b.r * t + 0.5f),
        .g = (uint8_t) ((float) a.g * (1.0f - t) + (float) b.g * t + 0.5f),
        .b = (uint8_t) ((float) a.b * (1.0f - t) + (float) b.b * t + 0.5f),
    };
}

rgb smooth(float d, rgb bg, rgb fg)
{
    float alpha = smoothstep(1.0f, -1.0f, d);

    return lerp_color(bg, fg, alpha);
}

void circle_smooth(rgb *pixels, size_t width, size_t height, rgb bg, rgb fg)
{
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float d               = sdf_circle(x, y, width / 2.0f, height / 2.0f, width / 4.0f);
            pixels[y * width + x] = smooth(d, bg, fg);
        }
    }
}

void box_smooth(rgb *pixels, size_t width, size_t height, rgb bg, rgb fg)
{
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float d = sdf_box(x, y, width / 2.0f, height / 2.0f, width / 4.0f, height / 4.0f);
            pixels[y * width + x] = smooth(d, bg, fg);
        }
    }
}

void rounded_box_smooth(rgb *pixels, size_t width, size_t height, rgb bg, rgb fg)
{
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float d               = sdf_rounded_box(x,
                                                    y,
                                                    width / 2.0f,
                                                    height / 2.0f,
                                                    width / 4.0f - ROUNDED_BOX_RADIUS,
                                                    height / 4.0f - ROUNDED_BOX_RADIUS,
                                                    ROUNDED_BOX_RADIUS);
            pixels[y * width + x] = smooth(d, bg, fg);
        }
    }
}

void shapes(rgb *pixels, size_t width, size_t height, rgb bg, rgb circle, rgb box, rgb rounded_box)
{
    float shape_diameter = width / 4.0f;
    float radius         = shape_diameter / 2;
    float gap            = (width - 3 * shape_diameter) / 4;
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            int third = x / (width / 3);

            switch (third)
            {
            case 0: { // circle
                float cx = gap + radius;
                pixels[y * width + x] =
                    smooth(sdf_circle(x, y, cx, height / 2.0f, radius), bg, circle);
            }
            break;
            case 1: { // box
                float cx = gap + shape_diameter + gap + radius;
                pixels[y * width + x] =
                    smooth(sdf_box(x, y, cx, height / 2.0f, radius, radius), bg, box);
            }
            break;
            case 2: { // rounded_box
                float cx              = gap + shape_diameter + gap + shape_diameter + gap + radius;
                pixels[y * width + x] = smooth(sdf_rounded_box(x,
                                                               y,
                                                               cx,
                                                               height / 2.0f,
                                                               radius - ROUNDED_BOX_RADIUS,
                                                               radius - ROUNDED_BOX_RADIUS,
                                                               ROUNDED_BOX_RADIUS),
                                               bg,
                                               rounded_box);
            }
            break;
            }
        }
    }
}

void booleans(rgb *pixels, size_t width, size_t height, rgb bg, rgb a, rgb b, rgb result)
{
    float shape_diameter = width / 6.0f;
    float radius         = shape_diameter / 2;
    float gap            = (width - 3 * shape_diameter) / 4;
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            int third = x / (width / 3);

            switch (third)
            {
            case 0: { // union
                float cx_a = gap + shape_diameter / 3;
                float cx_b = gap + shape_diameter * 2 / 3;

                float d_a      = sdf_circle(x, y, cx_a, height / 2.0f, radius);
                float d_b      = sdf_circle(x, y, cx_b, height / 2.0f, radius);
                float d_result = sdf_union(d_a, d_b);

                float alpha_a     = smoothstep(1.0f, -1.0f, d_a);
                float alpha_b     = smoothstep(1.0f, -1.0f, d_b);
                float alpha_union = smoothstep(1.0f, -1.0f, d_result);

                rgb color = lerp_color(bg, a, alpha_a);
                color     = lerp_color(color, b, alpha_b);
                color     = lerp_color(color, result, alpha_union);

                pixels[y * width + x] = color;
            }
            break;
            case 1: { // intersection
                float cx_a = gap + shape_diameter + gap + shape_diameter / 3;
                float cx_b = gap + shape_diameter + gap + shape_diameter * 2 / 3;

                float d_a      = sdf_circle(x, y, cx_a, height / 2.0f, radius);
                float d_b      = sdf_circle(x, y, cx_b, height / 2.0f, radius);
                float d_result = sdf_intersection(d_a, d_b);

                float alpha_a     = smoothstep(1.0f, -1.0f, d_a);
                float alpha_b     = smoothstep(1.0f, -1.0f, d_b);
                float alpha_union = smoothstep(1.0f, -1.0f, d_result);

                rgb color = lerp_color(bg, a, alpha_a);
                color     = lerp_color(color, b, alpha_b);
                color     = lerp_color(color, result, alpha_union);

                pixels[y * width + x] = color;
            }
            break;
            case 2: { // subtraction
                float cx_a = gap + shape_diameter + gap + shape_diameter + gap + shape_diameter / 3;
                float cx_b =
                    gap + shape_diameter + gap + shape_diameter + gap + shape_diameter * 2 / 3;

                float d_a      = sdf_circle(x, y, cx_a, height / 2.0f, radius);
                float d_b      = sdf_circle(x, y, cx_b, height / 2.0f, radius);
                float d_result = sdf_subtraction(d_a, d_b);

                float alpha_a     = smoothstep(1.0f, -1.0f, d_a);
                float alpha_b     = smoothstep(1.0f, -1.0f, d_b);
                float alpha_union = smoothstep(1.0f, -1.0f, d_result);

                rgb color = lerp_color(bg, a, alpha_a);
                color     = lerp_color(color, b, alpha_b);
                color     = lerp_color(color, result, alpha_union);

                pixels[y * width + x] = color;
            }
            break;
            }
        }
    }
}

float mix(float a, float b, float t)
{
    return a + t * (b - a);
}

float smin(float a, float b, float k)
{
    float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return mix(b, a, h) - k * h * (1 - h);
}

void smooth_union(rgb *pixels, size_t width, size_t height, rgb bg, rgb fg)
{
    float shape_diameter = width / 4.0f;
    float radius         = shape_diameter / 2.0f;
    float gap            = (width - 2 * (shape_diameter + radius)) / 3;
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            int half = x / (width / 2);
            if (half == 0)
            {
                float cx_a = gap + radius;
                float cx_b = cx_a + radius;

                float d_a = sdf_circle(x, y, cx_a, height / 2.0f, radius);
                float d_b = sdf_circle(x, y, cx_b, height / 2.0f, radius);
                float d   = sdf_union(d_a, d_b);

                pixels[y * width + x] = smooth(d, bg, fg);
            }
            else
            {
                float cx_a = gap + shape_diameter + radius + gap + radius;
                float cx_b = cx_a + radius;

                float d_a = sdf_circle(x, y, cx_a, height / 2.0f, radius);
                float d_b = sdf_circle(x, y, cx_b, height / 2.0f, radius);
                float d   = smin(d_a, d_b, 64.0f);

                pixels[y * width + x] = smooth(d, bg, fg);
            }
        }
    }
}

void domain_distortion(rgb *pixels, size_t width, size_t height, rgb bg, rgb fg)
{
    float cx = width / 2.0f, cy = height / 2.0f;
    float radius = width / 5.0f;

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float distorted_px = x + sinf(y * 0.01f) * 20.0f;
            float d            = sdf_circle(distorted_px, y, cx, cy, radius);

            pixels[y * width + x] = smooth(d, bg, fg);
            float glow_alpha      = smoothstep(radius * 2.0f, 0.0f, d);
            rgb   glow_color      = lerp_color(bg, fg, glow_alpha * 0.3f);
            pixels[y * width + x] =
                lerp_color(pixels[y * width + x], glow_color, glow_alpha * 0.5f);
        }
    }
}

void space_scene(rgb *pixels, size_t width, size_t height, float t)
{
    float cx = width / 2.0f, cy = height / 2.0f;

    rgb bg      = RGB_HEX(NAVY);
    rgb sun_col = RGB_HEX(AMBER);
    rgb p1_col  = RGB_HEX(HONEYDEW);
    rgb p2_col  = RGB_HEX(ROSE);
    rgb p3_col  = RGB_HEX(TEAL);
    rgb sta_col = RGB_HEX(INDIGO);
    rgb orb_col = RGB_HEX(SLATE);

    float sun_r  = width * 0.09f;
    float prom_r = sun_r * 0.4f;
    float prom_x = cx + sun_r * 0.7f;
    float prom_y = cy - sun_r * 0.9f;
    float sun_k  = sun_r * 0.5f;

    float orbit1 = width * 0.20f;
    float orbit2 = width * 0.31f;
    float orbit3 = width * 0.41f;

    float r1 = width * 0.022f;
    float r2 = width * 0.032f;
    float r3 = width * 0.037f;

    float a1 = (25.0f + t * 360.0f * 3.0f) * M_PI / 180.0f;
    float a2 = (200.0f + t * 360.0f * 2.0f) * M_PI / 180.0f;
    float a3 = (315.0f + t * 360.0f * 1.0f) * M_PI / 180.0f;

    float p1x = cx + orbit1 * cosf(a1), p1y = cy + orbit1 * sinf(a1);
    float p2x = cx + orbit2 * cosf(a2), p2y = cy + orbit2 * sinf(a2);
    float p3x = cx + orbit3 * cosf(a3), p3y = cy + orbit3 * sinf(a3);

    float a_sta  = 130.0f * (float) M_PI / 180.0f;
    float sta_or = (orbit1 + orbit2) * 0.5f;
    float stax   = cx + sta_or * cosf(a_sta);
    float stay   = cy + sta_or * sinf(a_sta);
    float sta_bw = width * 0.012f;
    float sta_br = width * 0.004f;
    float sta_pw = width * 0.030f;
    float sta_ph = width * 0.004f;

    float stars[][3] = {
        {width * 0.08f, height * 0.07f, 2.0f},
        {width * 0.84f, height * 0.11f, 1.5f},
        {width * 0.13f, height * 0.78f, 1.5f},
        {width * 0.79f, height * 0.83f, 2.0f},
        {width * 0.93f, height * 0.44f, 1.5f},
        {width * 0.04f, height * 0.38f, 1.0f},
        {width * 0.61f, height * 0.04f, 1.5f},
        {width * 0.37f, height * 0.94f, 1.0f},
        {width * 0.71f, height * 0.93f, 1.0f},
        {width * 0.96f, height * 0.76f, 1.5f},
    };

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float px = (float) x, py = (float) y;
            rgb   color = bg;

            for (size_t i = 0; i < sizeof(stars) / sizeof(stars[0]); ++i)
            {
                float d = sdf_circle(px, py, stars[i][0], stars[i][1], stars[i][2]);
                color   = lerp_color(
                    color, lerp_color(bg, p1_col, 0.12f), smoothstep(stars[i][2] * 4.0f, 0.0f, d));
                color = lerp_color(color, p1_col, smoothstep(1.0f, -1.0f, d));
            }

            color = lerp_color(color,
                               orb_col,
                               smoothstep(1.2f, 0.0f, fabsf(sdf_circle(px, py, cx, cy, orbit1))) *
                                   0.8f);
            color = lerp_color(color,
                               orb_col,
                               smoothstep(1.2f, 0.0f, fabsf(sdf_circle(px, py, cx, cy, orbit2))) *
                                   0.8f);
            color = lerp_color(color,
                               orb_col,
                               smoothstep(1.2f, 0.0f, fabsf(sdf_circle(px, py, cx, cy, orbit3))) *
                                   0.8f);

            float d_sun      = sdf_circle(px, py, cx, cy, sun_r);
            float d_prom     = sdf_circle(px, py, prom_x, prom_y, prom_r);
            float d_sun_full = smin(d_sun, d_prom, sun_k);
            color = lerp_color(color,
                               lerp_color(bg, sun_col, 0.06f),
                               smoothstep(sun_r * 3.0f, 0.0f, d_sun_full)); // wide dim corona
            color = lerp_color(color,
                               lerp_color(bg, sun_col, 0.22f),
                               smoothstep(sun_r * 1.5f, 0.0f, d_sun_full)); // inner glow
            color = lerp_color(color,
                               lerp_color(bg, sun_col, 0.55f),
                               smoothstep(sun_r * 0.4f, 0.0f, d_sun_full)); // sharp edge
            color = lerp_color(color, sun_col, smoothstep(1.0f, -1.0f, d_sun_full));

            float d_p1 = sdf_circle(px, py, p1x, p1y, r1);
            color =
                lerp_color(color, lerp_color(bg, p1_col, 0.18f), smoothstep(r1 * 2.5f, 0.0f, d_p1));
            color = lerp_color(color, p1_col, smoothstep(1.0f, -1.0f, d_p1));

            float d_p2 = sdf_circle(px, py, p2x, p2y, r2);
            color =
                lerp_color(color, lerp_color(bg, p2_col, 0.18f), smoothstep(r2 * 2.5f, 0.0f, d_p2));
            color = lerp_color(color, p2_col, smoothstep(1.0f, -1.0f, d_p2));

            float d_p3       = sdf_circle(px, py, p3x, p3y, r3);
            float d_ring_out = sdf_circle(px, py, p3x, p3y, r3 * 1.85f);
            float d_ring_in  = sdf_circle(px, py, p3x, p3y, r3 * 1.3f);
            float d_ring     = sdf_subtraction(d_ring_out, d_ring_in);
            color =
                lerp_color(color, lerp_color(bg, p3_col, 0.18f), smoothstep(r3 * 2.5f, 0.0f, d_p3));
            color = lerp_color(color, p3_col, smoothstep(1.0f, -1.0f, d_ring) * 0.55f);
            color = lerp_color(color, p3_col, smoothstep(1.0f, -1.0f, d_p3));

            float d_sta_body =
                sdf_rounded_box(px, py, stax, stay, sta_bw - sta_br, sta_bw - sta_br, sta_br);
            float d_sta_panels = sdf_box(px, py, stax, stay, sta_pw, sta_ph);
            float d_station    = sdf_union(d_sta_body, d_sta_panels);
            color              = lerp_color(
                color, lerp_color(bg, sta_col, 0.20f), smoothstep(sta_bw * 3.0f, 0.0f, d_station));
            color = lerp_color(color, sta_col, smoothstep(1.0f, -1.0f, d_station));

            pixels[y * width + x] = color;
        }
    }
}

void sdf_metaballs(rgb *pixels, size_t width, size_t height, float t)
{
    float cx = width / 2.0f, cy = height / 2.0f;

    rgb bg = RGB_HEX(NAVY);
    rgb c1 = RGB_HEX(INDIGO);
    rgb c2 = RGB_HEX(ROSE);
    rgb c3 = RGB_HEX(TEAL);
    rgb c4 = RGB_HEX(AMBER);

    float r = width * 0.08f;
    float k = width * 0.06f;

    float a1 = t * 2.0f * (float) M_PI * 1.0f;
    float a2 = t * 2.0f * (float) M_PI * 2.0f;
    float a3 = t * 2.0f * (float) M_PI * 3.0f;
    float a4 = t * 2.0f * (float) M_PI * 5.0f;

    float p1x = cx + width * 0.20f * cosf(a1), p1y = cy + width * 0.20f * sinf(a1);
    float p2x = cx + width * 0.17f * cosf(a2), p2y = cy + width * 0.17f * sinf(a2);
    float p3x = cx + width * 0.22f * cosf(a3), p3y = cy + width * 0.22f * sinf(a3);
    float p4x = cx + width * 0.15f * cosf(a4), p4y = cy + width * 0.15f * sinf(a4);

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float d1 = sdf_circle(x, y, p1x, p1y, r);
            float d2 = sdf_circle(x, y, p2x, p2y, r);
            float d3 = sdf_circle(x, y, p3x, p3y, r);
            float d4 = sdf_circle(x, y, p4x, p4y, r);

            float d = d1;
            d       = smin(d, d2, k);
            d       = smin(d, d3, k);
            d       = smin(d, d4, k);

            float w1    = 1.0f / (d1 + r + 1.0f);
            float w2    = 1.0f / (d2 + r + 1.0f);
            float w3    = 1.0f / (d3 + r + 1.0f);
            float w4    = 1.0f / (d4 + r + 1.0f);
            float total = w1 + w2 + w3 + w4;

            rgb col = {.r = (uint8_t) (c1.r * (w1 / total) + c2.r * (w2 / total) +
                                       c3.r * (w3 / total) + c4.r * (w4 / total)),
                       .g = (uint8_t) (c1.g * (w1 / total) + c2.g * (w2 / total) +
                                       c3.g * (w3 / total) + c4.g * (w4 / total)),
                       .b = (uint8_t) (c1.b * (w1 / total) + c2.b * (w2 / total) +
                                       c3.b * (w3 / total) + c4.b * (w4 / total))};

            rgb color = bg;
            color = lerp_color(color, lerp_color(bg, col, 0.08f), smoothstep(r * 4.0f, 0.0f, d));
            color = lerp_color(color, lerp_color(bg, col, 0.40f), smoothstep(r * 1.5f, 0.0f, d));
            color = lerp_color(color, col, smoothstep(1.0f, -1.0f, d));

            pixels[y * width + x] = color;
        }
    }
}

int main()
{
    static rgb pixels[NUM_PIXELS];
    static rgb wide_pixels[WIDTH * 3 / 2 * HEIGHT];

    rgb honeydew = RGB_HEX(HONEYDEW);
    rgb navy     = RGB_HEX(NAVY);
    rgb indigo   = RGB_HEX(INDIGO);
    rgb rose     = RGB_HEX(ROSE);

    fill_pixels(pixels, NUM_PIXELS, navy);

    circle_hard(pixels, WIDTH, HEIGHT, navy, honeydew);
    save_as_ppm("output/circle_hard.ppm", pixels, WIDTH, HEIGHT);

    circle_smooth(pixels, WIDTH, HEIGHT, navy, honeydew);
    save_as_ppm("output/circle_smooth.ppm", pixels, WIDTH, HEIGHT);

    box_smooth(pixels, WIDTH, HEIGHT, navy, honeydew);
    save_as_ppm("output/box_smooth.ppm", pixels, WIDTH, HEIGHT);

    rounded_box_smooth(pixels, WIDTH, HEIGHT, navy, honeydew);
    save_as_ppm("output/rounded_box_smooth.ppm", pixels, WIDTH, HEIGHT);

    shapes(wide_pixels, WIDTH * 3 / 2, HEIGHT, navy, honeydew, indigo, rose);
    save_as_ppm("output/shapes.ppm", wide_pixels, WIDTH * 3 / 2, HEIGHT);

    booleans(wide_pixels, WIDTH * 3 / 2, HEIGHT, navy, indigo, rose, honeydew);
    save_as_ppm("output/booleans.ppm", wide_pixels, WIDTH * 3 / 2, HEIGHT);

    smooth_union(wide_pixels, WIDTH * 3 / 2, HEIGHT, navy, honeydew);
    save_as_ppm("output/smooth_union.ppm", wide_pixels, WIDTH * 3 / 2, HEIGHT);

    domain_distortion(pixels, WIDTH, HEIGHT, navy, honeydew);
    save_as_ppm("output/domain_distortion.ppm", pixels, WIDTH, HEIGHT);

    space_scene(pixels, WIDTH, HEIGHT, 0.0f);
    save_as_ppm("output/scene.ppm", pixels, WIDTH, HEIGHT);

    int fps        = 60;
    int duration   = 10;
    int num_frames = fps * duration;
    for (int frame = 0; frame < num_frames; ++frame)
    {
        float t = frame / (float) num_frames;
        char  filename[64];
        snprintf(filename, sizeof(filename), "output/metaballs_%04d.ppm", frame);
        sdf_metaballs(pixels, WIDTH, HEIGHT, t);
        save_as_ppm(filename, pixels, WIDTH, HEIGHT);
    }

    return 0;
}

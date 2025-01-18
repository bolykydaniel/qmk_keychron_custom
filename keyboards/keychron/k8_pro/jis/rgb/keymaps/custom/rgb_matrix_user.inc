// Define the custom effect
RGB_MATRIX_EFFECT(FIRE_AND_ICE)

// Define the effect parameters
#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#include "lib/lib8tion/lib8tion.h"
#include "custom_effects.h"
#include "quantum.h"
#include <math.h>
#include "quantum/wpm.h"

#define RIPPLE_DURATION 500    // Duration of ripple effect (ms)
#define MAX_RIPPLES 16         // Maximum number of concurrent ripples
#define BASE_RIPPLE_SPEED 30   // Base speed of ripple expansion
#define WPM_RIPPLE_FACTOR 0.5  // How much WPM affects ripple intensity

// Ripple structure
typedef struct {
    uint8_t row;
    uint8_t col;
    uint16_t start_time;
    bool active;
} ripple_t;

// Array to store active ripples
static ripple_t ripples[MAX_RIPPLES] = {{0}};

// Helper function to process ripples
static void process_ripples(uint8_t row, uint8_t col, uint8_t led_index, 
                          uint8_t *r, uint8_t *g, uint8_t *b) {
    uint16_t current_time = timer_read();
    uint8_t wpm = get_current_wpm();
    float wpm_multiplier = 1.0 + (wpm > 0 ? (wpm * WPM_RIPPLE_FACTOR / 100.0) : 0);
    
    // Process each active ripple
    for (uint8_t i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            uint16_t elapsed = current_time - ripples[i].start_time;
            
            if (elapsed > RIPPLE_DURATION) {
                ripples[i].active = false;
                continue;
            }
            
            // Calculate ripple parameters
            float progress = (float)elapsed / RIPPLE_DURATION;
            float ripple_radius = progress * BASE_RIPPLE_SPEED * wpm_multiplier;
            float distance = sqrt(pow(row - ripples[i].row, 2) + pow(col - ripples[i].col, 2));
            
            if (distance <= ripple_radius) {
                // Calculate ripple intensity
                float intensity = (1.0 - (distance / ripple_radius)) * (1.0 - progress);
                intensity *= wpm_multiplier;  // Increase intensity with WPM
                
                // Blend white ripple with current color
                *r = MIN(*r + (uint8_t)(255 * intensity), 255);
                *g = MIN(*g + (uint8_t)(255 * intensity), 255);
                *b = MIN(*b + (uint8_t)(255 * intensity), 255);
            }
        }
    }
}

// Function to add new ripple
void add_ripple(uint8_t row, uint8_t col) {
    for (uint8_t i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) {
            ripples[i].row = row;
            ripples[i].col = col;
            ripples[i].start_time = timer_read();
            ripples[i].active = true;
            break;
        }
    }
}

bool FIRE_AND_ICE(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    uint8_t time = scale16by8(g_rgb_timer, 8);
    
    // Create a moving boundary between fire and ice
    float middle_point = 0.5 + sin8(time * 2) / 512.0;
    
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        float row_pos = (float)row / (MATRIX_ROWS - 1);
        
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led_index = g_led_config.matrix_co[row][col];
            
            if (led_index >= led_min && led_index < led_max) {
                // Create flowing waves
                uint8_t wave = sin8(time + row * 20 + col * 20) / 2;
                uint8_t color_shift = sin8(time * 3 + row * 30 + col * 30) / 3;
                uint8_t intensity = sin8(time * 4) / 2 + 128;
                
                uint8_t brightness = wave + 128;
                uint8_t bright = 50 + ((uint16_t)brightness * 205) / 255;
                
                float dist_from_middle = fabsf(row_pos - middle_point);
                float blend_factor = 1.0 - MIN(dist_from_middle / 0.3, 1.0);
                
                // Initialize colors for this LED
                uint8_t red, green, blue;
                
                if (row_pos < middle_point) {
                    // Fire colors
                    red = bright + color_shift / 2;
                    green = (uint8_t)(bright * (0.2 + (sin8(time * 5) / 512.0)));
                    blue = (uint8_t)(bright * 0.1);
                    
                    red = (uint16_t)red * intensity / 255;
                    green = (uint16_t)green * intensity / 255;
                    
                    if (blend_factor > 0) {
                        uint8_t ice_blue = bright;
                        uint8_t ice_green = (uint8_t)(bright * 0.7);
                        red = lerp8by8(red, (uint8_t)(bright * 0.2), blend_factor * 255);
                        green = lerp8by8(green, ice_green, blend_factor * 255);
                        blue = lerp8by8(blue, ice_blue, blend_factor * 255);
                    }
                } else {
                    // Ice colors
                    blue = bright + color_shift / 2;
                    green = (uint8_t)(bright * (0.5 + (sin8(time * 5) / 512.0)));
                    red = (uint8_t)(bright * 0.2);
                    
                    blue = (uint16_t)blue * intensity / 255;
                    green = (uint16_t)green * intensity / 255;
                    
                    if (blend_factor > 0) {
                        uint8_t fire_red = bright;
                        uint8_t fire_green = (uint8_t)(bright * 0.3);
                        red = lerp8by8(red, fire_red, blend_factor * 255);
                        green = lerp8by8(green, fire_green, blend_factor * 255);
                        blue = lerp8by8(blue, (uint8_t)(bright * 0.1), blend_factor * 255);
                    }
                }
                
                // Process ripple effects
                process_ripples(row, col, led_index, &red, &green, &blue);
                
                // Set final color
                rgb_matrix_set_color(led_index, red, green, blue);
            }
        }
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS 
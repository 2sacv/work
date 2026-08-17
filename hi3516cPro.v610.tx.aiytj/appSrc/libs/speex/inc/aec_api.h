#ifndef API_SIMPLE_AEC3_API_H_
#define API_SIMPLE_AEC3_API_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// init handle
typedef struct Aec3Handle Aec3Handle;

// Configuration structure for AEC3
typedef struct {
  int sample_rate_hz;          // Sample rate in Hz (8000, 16000, 32000, 48000)
  int num_render_channels;     // Number of render channels
  int num_capture_channels;    // Number of capture channels
  int use_external_delay_estimator;  // Enable external delay estimator (0 or 1)
  int use_high_pass_filter;    // Enable high pass filter (0 or 1)
} Aec3Config;

// Initialize the AEC3 instance
// Returns a handle to the AEC3 instance on success, NULL on failure
Aec3Handle* aec3_create(const Aec3Config* config);

// Process echo cancellation
// render_data: pointer to render audio data (far-end)
// capture_data: pointer to capture audio data (near-end, will be modified)
// output_data: pointer to output audio data (optional, can be NULL)
// samples_per_channel: number of samples per channel
// returns 0 on success, non-zero on failure
int aec3_process(Aec3Handle* handle,
                 int16_t* render_data,
                 int16_t* capture_data,
                 int16_t* output_data,
                 size_t samples_per_channel);

// Set external delay estimation in milliseconds
void aec3_set_delay(Aec3Handle* handle, int delay_ms);

// Get echo cancellation metrics
typedef struct {
  double echo_return_loss;
  double echo_return_loss_enhancement;
  int delay_ms;
} Aec3Metrics;

int aec3_get_metrics(Aec3Handle* handle, Aec3Metrics* metrics);

// Destroy the AEC3 instance and free resources
void aec3_destroy(Aec3Handle* handle);

#ifdef __cplusplus
}
#endif

#endif  // API_SIMPLE_AEC3_API_H_
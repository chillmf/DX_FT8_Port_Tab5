#include <Arduino.h>
#include <driver/i2s_std.h>
#include "AudioBoard.h"
#include "DriverDeviceInfo.h"
#include "SDR_Audio.h"
#include "main.h"
#include "Process_DSP.h"
#include "Display.h"
#include "traffic_manager.h"
#include "constants.h"


// --- Pin Assignments ---
#define I2S_MCLK_IO      GPIO_NUM_2
#define I2S_BCLK_IO      GPIO_NUM_3
#define I2S_WS_IO        GPIO_NUM_5
#define I2S_DIN_IO       GPIO_NUM_18
#define I2S_DOUT_IO      GPIO_NUM_19

// --- Audio Buffer Profile ---
// Total Buffer size requested: 1280 * 4 = 5120 bytes.
// Split across double buffering (2 descriptors): 1280 / 2 = 640 frames per buffer block.
#define DMA_BUFFER_FRAME_SIZE 640 
#define DMA_BUFFER_BYTE_SIZE  (DMA_BUFFER_FRAME_SIZE * 4)

// Driver Pin & Board Management Context Instances
audio_driver::DriverDeviceInfo codec_pins;
audio_driver::AudioBoard board(audio_driver::AudioDriverSGTL5000, codec_pins);

// Native ESP32-P4 I2S Channel Handles
i2s_chan_handle_t tx_handle = NULL;
i2s_chan_handle_t rx_handle = NULL;

// Shared processing buffer
uint8_t audio_io_buffer[DMA_BUFFER_BYTE_SIZE];


// --- FT8 FFT Buffer Allocations ---
#define FT8_BUFFER_SIZE 1024

uint16_t ft8_index = 0;
uint8_t decimation_counter = 0;
const int offset_index = 8;
extern uint32_t dsp_current_time, dsp_old_time, dsp_start_time,dsp_flag_time;

void init_codec_driver(void) {
    Serial.println("Manually initializing I2C Core with explicit Pin Mapping...");

    codec_pins.addI2C(audio_driver::PinFunction::CODEC, -1, -1, 0x0A, 400000U, Wire);

    Serial.println("Configuring SGTL5000 Codec properties...");
    
    audio_driver::CodecConfig cfg;
    cfg.input_device  = audio_driver::ADC_INPUT_LINE1;  
    cfg.output_device = audio_driver::DAC_OUTPUT_LINE1; 
    
    cfg.i2s.bits = audio_driver::BIT_LENGTH_16BITS;
    cfg.i2s.rate = audio_driver::RATE_32K;
    cfg.i2s.fmt  = audio_driver::I2S_NORMAL;
    cfg.i2s.mode = audio_driver::MODE_SLAVE; 

    writeCodecRegister(0x0030, 0x4060); 
    writeCodecRegister(0x0026, 0x006C); 
    writeCodecRegister(0x0028, 0x01F2); 
    writeCodecRegister(0x002C, 0x0F22); 
    writeCodecRegister(0x003C, 0x4446); 
    writeCodecRegister(0x0030, 0x40FF); 
    writeCodecRegister(0x0002, 0x0073); 
    delay(400);
    writeCodecRegister(0x002E, 0x1D1D); 
    writeCodecRegister(0x0000, 0x0004); 
    writeCodecRegister(0x0006, 0x0030); 
    writeCodecRegister(0x000A, 0x0010); 
    writeCodecRegister(0x000E, 0x0000); 
    writeCodecRegister(0x0010, 0x3C3C); 
    writeCodecRegister(0x0022, 0x7F7F); 
    
    writeCodecRegister(0x002A, 0x0000); 
    writeCodecRegister(0x0020, 0x011); 
    writeCodecRegister(0x0024, 0x0000); 

    //writeCodecRegister(0x002A, 0x0173); 
    //writeCodecRegister(0x0020, 0x055); 
    //writeCodecRegister(0x0024, 0x0004); 

}

void lineInLevel(uint8_t left, uint8_t right) {
if (left > 15) left = 15;
	if (right > 15) right = 15;
	writeCodecRegister(0x0020, (left << 4) | right); 

}

// Initialize Full-Duplex I2S Core on the ESP32-P4
void init_i2s_duplex(void) {
    Serial.println("Initializing ESP32-P4 I2S Peripherals...");

    // Allocate configuration structures directly into Channel Configuration space
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 2;                        // Double-buffered constraint
    chan_cfg.dma_frame_num = DMA_BUFFER_FRAME_SIZE;   // Interrupt blocking threshold size

    i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);

    // Structural audio mapping layout profile configuration
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 32000,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256 // Generates clean 8.192 MHz clock on MCLK pin
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_IO,
            .bclk = I2S_BCLK_IO,
            .ws   = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din  = I2S_DIN_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false
            }
        }
    };

    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_init_std_mode(rx_handle, &std_cfg);

    i2s_channel_enable(tx_handle);
    i2s_channel_enable(rx_handle);
    
}

// --- IQ Processing Configurations ---
#define FIR_TAP_COUNT 128
#define FIR_MASK      127
#define FIR_SHIFT     15  // Scale factor matched to your Q15 coefficient scaling

const int32_t fir_coeffs_I[FIR_TAP_COUNT] = {
     0, 52, 14, -42, -99, -140, -147, -116,
    -56, 8, 46, 28, -57, -195, -347, -461, -490, -415, -261, -93, 5, -40,
    -252, -589, -949, -1199, -1235, -1022, -628, -208, 44, -21, -446, -1125,
    -1830, -2284, -2272, -1737, -830, 124, 733, 697, -62, -1330, -2642,
    -3441, -3284, -2042, 0, 2190, 3718, 3908, 2518, -86, -2950, -4769,
    -4292, -762, 5747, 14222, 22900, 29693, 32767, 31065, 24645, 14692,
    3227, -7438, -15302, -19178, -18947, -15492, -10360, -5261, -1577, -13,
    -501, -2343, -4531, -6122, -6538, -5705, -4001, -2063, -526, 197, 33,
    -770, -1772, -2525, -2734, -2341, -1518, -573, 180, 531, 445, 43, -456,
    -831, -939, -758, -375, 58, 392, 533, 470, 267, 25, -155, -214, -149, 0,
    164, 284, 324, 283, 190, 86, 7, -24, -6, 44, 103, 147, 164, 149, 0};


// Q-Filter Coefficients (-45 Degree Phase Shift)
const int32_t fir_coeffs_Q[FIR_TAP_COUNT] = {
    // Replace this placeholder array with your exact generated -45° bandpass integer array
    0, -112, -150, -165, -148, -104, -45, 5,
    23, -8, -87, -191, -284, -325, -285, -165, 0, 148, 213, 154, -26, -268,
    -471, -534, -393, -59, 374, 757, 938, 830, 455, -44, -446, -532, -181,
    572, 1517, 2340, 2733, 2524, 1771, 769, -34, -198, 525, 2062, 4000,
    5704, 6537, 6121, 4530, 2342, 500, 12, 1576, 5260, 10359, 15491, 18946,
    19177, 15301, 7437, -3228, -14693, -24646, -31066, -32767, -29694,
    -22901, -14223, -5748, 761, 4291, 4768, 2949, 85, -2519, -3909, -3719,
    -2191, 0, 2041, 3283, 3440, 2641, 1329, 61, -698, -734, -125, 829, 1736,
    2271, 2283, 1829, 1124, 445, 20, -45, 207, 627, 1021, 1234, 1198, 948,
    588, 251, 39, -6, 92, 260, 414, 489, 460, 346, 194, 56, -29, -47, -9,
    55, 115, 146, 139, 98, 41, -15, 0};


// FIR History Delay Ring-Buffers
int16_t hist_I[FIR_TAP_COUNT] = {0};
int16_t hist_Q[FIR_TAP_COUNT] = {0};
uint8_t hist_ptr = 0; 

// Local Oscillator phase counter
uint32_t lo_sample_index = 0;
int frame_counter = 0;

int attenuation;

void process_audio_stream(void){

    size_t bytes_read = 0;
    size_t bytes_written = 0;

    // Blocks on DMA interrupt when buffer is full
    esp_err_t rx_status = i2s_channel_read(rx_handle, audio_io_buffer, DMA_BUFFER_BYTE_SIZE, &bytes_read, portMAX_DELAY);

        if (rx_status == ESP_OK && bytes_read > 0) {
        
        
        // Process buffer frame-by-frame (Step size = 4 bytes: 2 bytes Left + 2 bytes Right)
        for (size_t i = 0; i < bytes_read; i += 4) {
            
            // 1. Extract raw 16-bit input audio (Left channel = Input I stream)
            int16_t input_I = *(int16_t*)&audio_io_buffer[i];

            // 2. Generate local oscillators at 10kHz (With Phase +45° and -45°)
            float rad_base = (2.0f * PI * 10000.0f * lo_sample_index) / 32000.0f;
            int16_t lo_I = (int16_t)(sinf(rad_base + (PI / 4.0f)) * 32767.0f); // +45 deg
            int16_t lo_Q = (int16_t)(sinf(rad_base - (PI / 4.0f)) * 32767.0f); // -45 deg

            // Advance oscillator step and wrap at 16
            lo_sample_index = (lo_sample_index + 1) % 16;

            // 3. Multiply input audio by the local oscillators
            int16_t mixed_I = (int16_t)(((int32_t)input_I * lo_I) >> 15);
            int16_t mixed_Q = (int16_t)(((int32_t)input_I * lo_Q) >> 15);

            // 4. Push mixed terms into their respective history buffers
            hist_I[hist_ptr] = mixed_I;
            hist_Q[hist_ptr] = mixed_Q;

            // 5. Convolve through separate I and Q 128-tap integer FIR structures
            int32_t acc_I = 0;
            int32_t acc_Q = 0;
            uint8_t read_ptr = hist_ptr;

            for (int k = 0; k < FIR_TAP_COUNT; k++) {
                acc_I += fir_coeffs_I[k] * hist_I[read_ptr];
                acc_Q += fir_coeffs_Q[k] * hist_Q[read_ptr];
                read_ptr = (read_ptr - 1) & FIR_MASK;
            }

            // Move delay line pointer forward
            hist_ptr = (hist_ptr + 1) & FIR_MASK;

            // Shift filter outputs down
            int32_t out_I = acc_I >> FIR_SHIFT;
            int32_t out_Q = acc_Q >> FIR_SHIFT;

            // 6. Combine paths: Sum for Output I stream, Subtract for Output Q stream
           int32_t final_I = out_I + out_Q;
           int32_t final_Q = out_I - out_Q;

            // Hard clamp to prevent 16-bit integer overflow clipping
            if (final_I >  32767) final_I =  32767;
            if (final_I < -32768) final_I = -32768;
            if (final_Q >  32767) final_Q =  32767;
            if (final_Q < -32768) final_Q = -32768;

  
            // --- 7. FT8 DECIMATION & BUFFER CAPTURE LOGIC ---
            // Decimate by factor of 5: capture every 5th calculated final_I sample
            
            if (decimation_counter == 5) {
                decimation_counter = 0; // Reset counter
                
                FT8_Data[ft8_index] = (int16_t)final_Q / attenuation;
                ft8_index++;

                // When array hits 1024 entries, process FFT and wrap index back to 0
                if (ft8_index > FT8_BUFFER_SIZE) {
                    process_FT8_FFT();
                    DSP_Flag = 1;
                    //if (xmit_flag && !Tune_On) process_xmit_message();
                    
                    //dsp_current_time = millis();
                    //dsp_flag_time = dsp_current_time - dsp_old_time;
                    //show_variable(500, 1240,(int)dsp_flag_time);
                    
                    ft8_index = 0; // Reset back to start of buffer
                    //dsp_old_time = millis();

                }
            
            }


            decimation_counter++;
            
            // 8. Overwrite buffer for analog stereo output (Left = Output I, Right = Output Q)
            *(int16_t*)&audio_io_buffer[i]     = (int16_t) final_I;
            *(int16_t*)&audio_io_buffer[i + 2] = (int16_t) final_Q;


        }

        // Pipe processed blocks straight to the Codec Line-Out DAC
        i2s_channel_write(tx_handle, audio_io_buffer, bytes_read, &bytes_written, portMAX_DELAY);
    }

} // End of process audio stream


    
    

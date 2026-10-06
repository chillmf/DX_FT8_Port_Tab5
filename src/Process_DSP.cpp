#include "SDR_Audio.h"
#include "Process_DSP.h"
#include "main.h"
#include <math.h>
#include <errno.h>
#include "Display.h"


static int16_t extract_signal[input_gulp_size * 3]; // was float
static int offset_step;

uint8_t export_fft_power[ft8_msg_samples * ft8_buffer_size * 4];

static float window[FFT_SIZE];
static float __attribute__((aligned(4))) window_dsp_buffer[FFT_SIZE];
static int16_t dsp_output[FFT_SIZE * 2];
static float FFT_Magnitude[FFT_SIZE / 2]; 
static float FFT_Mag_10[FFT_SIZE / 2];
static float mag_db[FFT_SIZE / 2 + 1];

uint8_t FFT_Buffer[ft8_buffer_size];
int16_t FT8_Data[2048 / 2];
float max_FFT;
int16_t max_db;


void process_FT8_FFT(void)
{
	for (int i = 0; i < input_gulp_size; i++)
	{
		extract_signal[i] = extract_signal[i + input_gulp_size];
		extract_signal[i + input_gulp_size] = extract_signal[i + 2 * input_gulp_size];
		extract_signal[i + 2 * input_gulp_size] = FT8_Data[i];
	}

	if (ft8_flag == 1)
	{
		int offset = offset_step * FT_8_counter;
		extract_power(offset);

		for (int k = 0; k < ft8_buffer_size; k++){
		FFT_Buffer[k] = export_fft_power[k + offset] / 8;
		}

		if(!xmit_flag)Display_WF();
		
		++FT_8_counter;

		//show_variable(200, 100, FT_8_counter);

		if (FT_8_counter == ft8_msg_samples) {
			decode_flag = 1;
			ft8_flag = 0;
		}
	}

}

// Compute FFT magnitudes (log power) for each time slot in the signal
void extract_power(int offset)
{
	// Loop over two possible time offsets (0 and block_size/2)
	for (int time_sub = 0; time_sub <= input_gulp_size / 2; time_sub += input_gulp_size / 2)
	{
		for (int i = 0; i < FFT_SIZE; i++)
			window_dsp_buffer[i] = ( ((float)extract_signal[i + time_sub]) / 1000.0) * window[i];

		static float real_buf[FFT_SIZE];
    	static float imag_buf[FFT_SIZE];
    	for (int i = 0; i < FFT_SIZE; i++) {
        real_buf[i] = window_dsp_buffer[i];
        imag_buf[i] = 0.0f;
    }

		compute_radix2_fft(real_buf, imag_buf, FFT_SIZE);
		//max_FFT = 0.0;

		for (int i = 0; i < FFT_SIZE / 2; i++) {
		float r = real_buf[i] ;
        float im = imag_buf[i] ;
		FFT_Magnitude[i] = (r * r + im * im);

    }

		for (int j = 0; j < FFT_SIZE / 2; j++)
		{
			mag_db[j] = 20.0 * log10f(FFT_Magnitude[j] + 0.1);
		}

		// Loop over two possible frequency bin offsets (for averaging)
		for (int freq_sub = 0; freq_sub < 2; ++freq_sub)
		{
			
			for (int k = 0; k < ft8_buffer_size; ++k)
			{
				int mag_offset = k * 2 + freq_sub;
				float db1 = mag_db[mag_offset];
				float db2 = mag_db[mag_offset + 1];
				float db = (db1 + db2) / 2;

				int scaled = (int)(db);

				export_fft_power[offset++] = (uint8_t)(scaled < 0) ? 0 : ((scaled > 254) ? 254 : scaled);

			}
		}
	}
}


static float ft_blackman_i(int i, int N)
{
	float alpha = 0.16f; // or 2860/18608
	float a0 = (1 - alpha) / 2;
	float a1 = 1.0f / 2;
	float a2 = alpha / 2;

	float x1 = cosf(2 * (float)M_PI * i / (N - 1));
	float x2 = 2 * x1 * x1 - 1; // Use double angle formula

	return a0 - a1 * x1 + a2 * x2;
}

void init_DSP(void)
{
	for (int i = 0; i < FFT_SIZE; ++i)
	{
		window[i] = ft_blackman_i(i, FFT_SIZE);
	}
	offset_step = (int16_t)ft8_buffer_size * 4;
}

static void compute_radix2_fft(float* real, float* imag, int n) {
    for (int i = 0, j = 0; i < n; i++) {
        if (i < j) {
            float temp_r = real[i]; real[i] = real[j]; real[j] = temp_r;
            float temp_i = imag[i]; imag[i] = imag[j]; imag[j] = temp_i;
        }
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j &= ~bit;
        j |= bit;
    }

    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * PI / len;
        float wlen_r = cosf(ang);
        float wlen_i = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f;
            float w_i = 0.0f;
            int half = len >> 1;
            for (int j = 0; j < half; j++) {
                int u = i + j;
                int v = i + j + half;
                float u_r = real[u];
                float u_i = imag[u];
                float v_r = real[v] * w_r - imag[v] * w_i;
                float v_i = real[v] * w_i + imag[v] * w_r;
                real[u] = u_r + v_r;
                imag[u] = u_i + v_i;
                real[v] = u_r - v_r;
                imag[v] = u_i - v_i;
                float next_w_r = w_r * wlen_r - w_i * wlen_i;
                float next_w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = next_w_r;
                w_i = next_w_i;
            }
        }
    }
}
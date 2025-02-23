#include <math.h>

#include "raylib.h"
#include "resource_dir.h"


unsigned const int SAMPLE_SIZE = 44100;
unsigned const int AUDIO_STREAM_BUFFER_SIZE = 1024;
unsigned const int OSCS_SIZE = 16;
const float SAMPLE_INSTANCE = 1.0f / SAMPLE_SIZE;

typedef struct osc_t {
	float phase;
	float phase_stride;
} Osc;


//updates the phase of the oscillator
void upd_osc(Osc* o) {
	o->phase += o->phase_stride;
		if(o->phase >= 1.0f)
			o->phase -= 1.0f;
}

void clr_signal_arr(float* buf) {
	for(int i = 0; i < AUDIO_STREAM_BUFFER_SIZE; i++) {
		buf[i] = 0.0f;
	}
}

void acc_signal_arr(float* buf, Osc* o, float amp) {
	for(int i = 0; i < AUDIO_STREAM_BUFFER_SIZE; i++) {
		upd_osc(o);
		buf[i] += sinf(2.0f*PI*o->phase)*amp;
		if (buf[i] > 1.0f) buf[i] = 1.0f;
    	if (buf[i] < -1.0f) buf[i] = -1.0f;
	}
}


//updates signal array by taking samples of a sine curve, using a formula found on
//https://zipcpu.com/dsp/2017/12/09/nco.html
void upd_signal_arr(float* buf, Osc* o) {	
	for(float i = 0; i < (float)AUDIO_STREAM_BUFFER_SIZE; i++) {
		upd_osc(o);
		buf[(int)i] = sinf(2.0f*PI*o->phase);
	}
}


int main() {

	//init stuff
	float signals[AUDIO_STREAM_BUFFER_SIZE];

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(1280, 800, "GSynthesizer");

	InitAudioDevice();
	SetMasterVolume(0.25f);
	SetAudioStreamBufferSizeDefault(AUDIO_STREAM_BUFFER_SIZE);
	AudioStream as = LoadAudioStream(SAMPLE_SIZE, sizeof(float)*8, 1);
	PlayAudioStream(as);

	SearchAndSetResourceDir("resources");

	//actual stuff
	float frequency = 0.5f;
	float phase_stride = 0.0f;

	Osc oscillators[OSCS_SIZE];

	for(int i = 0; i < OSCS_SIZE; i++) {
		oscillators[i].phase = 0.0f;
		oscillators[i].phase_stride = 0.0f;
	}

	while (!WindowShouldClose())
	{
		//first the calculations
		Vector2 mouse_pos = GetMousePosition();
		float U_pos = mouse_pos.x/(float)GetScreenWidth();
		float detune = 1.0f+U_pos;
		if(IsAudioStreamProcessed(as)) {

			
			clr_signal_arr(signals);


			for(int i = 0; i < OSCS_SIZE; i++) {
				float U_i = ((float)i)/(float)OSCS_SIZE;
				float base_frequency = 50.0f + (((float)i/OSCS_SIZE)*5.0f);
				frequency = (base_frequency + U_pos * 100.0f) * U_i * 10.0f;
				phase_stride = frequency * SAMPLE_INSTANCE;
				oscillators[i].phase_stride += (phase_stride - oscillators[i].phase_stride) * 0.1f;
				//amplitude = 1/osize in order to have an overall amplitude of 1 once they're all stacked upon each other
				acc_signal_arr(signals, &oscillators[i], 1.0f/(float)OSCS_SIZE);
			}


			UpdateAudioStream(as, signals, AUDIO_STREAM_BUFFER_SIZE);
		}
		
		BeginDrawing();
		ClearBackground(BLACK);
		DrawText(TextFormat("FREQ: %f", frequency), 100,100,20,WHITE);
		for(int i = 0; i < AUDIO_STREAM_BUFFER_SIZE; i++) {
			DrawPixel(50+i, 400+(int)(100*signals[i]), RED);
		}
		EndDrawing();
	}

	
	CloseWindow();
	CloseAudioDevice();
	return 0;
}

#include <math.h>

#include "raylib.h"
#include "resource_dir.h"


unsigned const int SAMPLE_SIZE = 44100;
unsigned const int AUDIO_STREAM_BUFFER_SIZE = 1024;
unsigned const int OSCS_SIZE = 2;
const float SAMPLE_INSTANCE = 1.0f / SAMPLE_SIZE;

typedef struct osc_t {
	float phase;
	float phase_stride;
	float phase_mod;
	float phase_mod_amp;
	float amp_mod;
} Osc;


//updates the phase of the oscillator
void upd_osc(Osc* o) {
	o->phase += o->phase_stride + o->phase_mod;
		if(o->phase >= 1.0f)
			o->phase -= 1.0f;
		if(o->phase < 0.0f)	
			o->phase += 1.0f;
}

void set_osc_freq(Osc* o, float freq) {
	o->phase_stride = freq * SAMPLE_INSTANCE;
}

float sine_wave_osc(Osc* o) {
	return sinf(2.0f*PI*o->phase);
}

float saw_wave_osc(Osc* o) {
	return o->phase;
}

float square_wave_osc(Osc* o) {
	if(sine_wave_osc(o) > 0.0f) {
		return 1.0f;
	} else {
		return -1.0f;
	}
}

float triangle_wave_osc(Osc* o) {
	if(sine_wave_osc(o) > 0.0f) {
		return 1.0f - 4.0f * fabs(o->phase - 0.25f);
	} else {
		return -1.0f + 4.0f * fabs(o->phase - 0.75f);
	}
}

void clr_signal_arr(float* buf) {
	for(int i = 0; i < AUDIO_STREAM_BUFFER_SIZE; i++) {
		buf[i] = 0.0f;
	}
}

void acc_signal_arr(float* buf, Osc* o, Osc* lfo, float amp) {
	for(int i = 0; i < AUDIO_STREAM_BUFFER_SIZE; i++) {
		upd_osc(lfo);
		o->phase_mod = sine_wave_osc(lfo) * lfo->phase_mod_amp;
		o->amp_mod = 1.0f + sine_wave_osc(lfo) * lfo->phase_mod_amp;
		upd_osc(o);

		buf[i] += sine_wave_osc(o)*amp*o->amp_mod;
		buf[i] = tanhf(buf[i]);
	}
}


//updates signal array by taking samples of a sine curve, using a formula found on
//https://zipcpu.com/dsp/2017/12/09/nco.html
void upd_signal_arr(float* buf, Osc* o) {	
	for(float i = 0; i < (float)AUDIO_STREAM_BUFFER_SIZE; i++) {
		upd_osc(o);
		buf[(int)i] = sine_wave_osc(o);
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
	Osc lfo = { .phase = 0.0f, .phase_stride=5.0f*SAMPLE_INSTANCE, .phase_mod_amp = 0.01f};

	for(int i = 0; i < OSCS_SIZE; i++) {
		oscillators[i].phase = 0.0f;
		oscillators[i].phase_stride = 0.0f;
	}

	char equation[100];
	int equation_i = 0;
	int synth_start = 0;

	while (!WindowShouldClose())
	{
		int key = GetCharPressed();
		// while(key > 0 && !synth_start) {
		// 	equation[equation_i] = (char)key;
		// 	equation[equation_i+1] = '\0';
		// 	equation_i++;
		// 	key=GetCharPressed();
		// 	if(equation_i > 10) { 
		// 		synth_start = 1;
		// 		break;
		// 		}
				
		// }
		// if(!synth_start) goto draw;
		//first the calculations
		Vector2 mouse_pos = GetMousePosition();
		float U_x_pos = mouse_pos.x/(float)GetScreenWidth();
		float U_y_pos = mouse_pos.y/(float)GetScreenHeight();
		float detune = 1.0f+U_x_pos;
		float additive = 500.0f;
		lfo.phase_mod_amp = U_y_pos*10.0f;
		if(IsAudioStreamProcessed(as)) {

			
			clr_signal_arr(signals);
			float base_frequency = 50.0f + (U_x_pos*1.0f) * sine_wave_osc(&lfo);
			//lfo.phase_mod_amp = U_y_pos ;
			for(int i = 0; i < OSCS_SIZE; i++) {
				if(i % 2 == 0) {
					additive = 1.0f;
				} else {
					additive = 2.0f;
				}
				float U_i = ((float)i)/(float)OSCS_SIZE;
				
				frequency = (base_frequency + U_x_pos * 100.0f) * U_i * U_i * 10.0f *additive;
				phase_stride = frequency * SAMPLE_INSTANCE;
				oscillators[i].phase_stride += (phase_stride - oscillators[i].phase_stride) * 0.02f;
				//amplitude = 1/osize in order to have an overall amplitude of 1 once they're all stacked upon each other
				acc_signal_arr(signals, &oscillators[i], &lfo, 1.0f/(float)OSCS_SIZE);
			}


			UpdateAudioStream(as, signals, AUDIO_STREAM_BUFFER_SIZE);
		}
		BeginDrawing();
		ClearBackground(BLACK);
		DrawText(TextFormat("FREQ: %f", frequency), 100,100,20,WHITE);
		DrawText(TextFormat("LFO Mod Amp: %f", lfo.phase_mod_amp), 100, 130, 20, WHITE);
		DrawText(TextFormat("%s", equation), 150, 150, 20, YELLOW);
		DrawFPS(10, 10);
		for(int i = 0; i < AUDIO_STREAM_BUFFER_SIZE; i++) {
			DrawPixel(50+i, 400+(int)(100*signals[i]), RED);
		}
		EndDrawing();
	}

	
	CloseWindow();
	CloseAudioDevice();
	return 0;
}

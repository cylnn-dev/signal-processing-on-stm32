//
// Created by CylNn on 24.12.2023.
//

#ifndef RTOS_PATCHED_DEMO_SIGNAL_GENERATORS_H
#define RTOS_PATCHED_DEMO_SIGNAL_GENERATORS_H

void calculate_dc_points(void);
void calculate_amplitudes(void);
void update_min_freq(void);
void calculate_lut(void);

void generate_cos(void);
void generate_saw(void);
void generate_triangle(void);
void generate_square(void);
void generate_impulse(void);

int get_next_amplitude(void);
int get_next_dc_point(void);


typedef void(*SignalFunction)(void);





#endif //RTOS_PATCHED_DEMO_SIGNAL_GENERATORS_H

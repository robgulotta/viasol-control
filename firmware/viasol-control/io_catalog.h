#pragma once
#include <Arduino.h>

extern const char* INPUT_KEYS[];
extern const int N_INPUTS;

extern const char* OUTPUT_KEYS[];
extern const int N_OUTPUTS;

float inputValueByKey(const String& key);
void applyOutput(const String& outputKey, bool on);

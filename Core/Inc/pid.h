#pragma once

// This function is used to calculate the output of PID
float PID_Compute(float setpointRPM, float measuredRPM);

// This function allows the setpoint to ramp up smoothly (ramp), instead of jumping straight to the target
float Ramp_Update(float current, float target, float rampRatePerSec, float dt);

extern float Kp;
extern float Ki;
extern float Kd;
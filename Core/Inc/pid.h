#pragma once

// This function is used to calculate the output of PID
float PID_Compute(float setpointRPM, float measuredRPM);

extern float Kp;
extern float Ki;
extern float Kd;
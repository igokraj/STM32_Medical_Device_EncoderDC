// **** THIS CODE IS USED TO HANDLE PID ****

// PID gains 
float Kp = 8.0f;
float Ki = 0.5f;
float Kd = 0.0f;

#define PWM_MAX 4199 // ARR (TIM1, 20 kHz)
#define SAMPLE_TIME_S  0.01f   // 100 Hz from TIM6

static float integral = 0.0f; // error sum
static float prevError = 0.0f; 

float PID_Compute(float setpointRPM, float measuredRPM)
{
float error = setpointRPM - measuredRPM;

integral += error * SAMPLE_TIME_S;

// Derivative calculation
float derivative = (error - prevError) / SAMPLE_TIME_S;

/* PID output (How much power to give to the DC motor?) calculation:
Kp * error -> reacts to the current error
Ki * integral -> reacts to the accumulated error in time 
Kd * derivative -> reacts to how fast the error is changing */
float output = (Kp * error) + (Ki * integral) + (Kd * derivative);


/* Clamp the output to the PWM range and prevent integral windup: if we're already saturated, adding more to "integral" wouldn't change anything real, so we undo the last addition instead */
if (output > PWM_MAX)
{
  output = PWM_MAX;
  integral -= error * SAMPLE_TIME_S;
}
else if (output < 0)
{
  output = 0;
  integral -= error * SAMPLE_TIME_S;
}

return output;



}
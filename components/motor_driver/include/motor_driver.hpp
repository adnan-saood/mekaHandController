#pragma once

class MotorDriver {
public:
    MotorDriver();
    void setPWM(float duty_cycle);  // duty_cycle in [-1.0, 1.0]
};
#pragma once
#include "sensors.hpp"

class Encoder {
public:
    EncoderData read() {
        EncoderData data;
        data.position.fill(0.0f); // Replace with actual reading logic
        data.velocity.fill(0.0f); // Replace with actual reading logic
        return data;
    }
};
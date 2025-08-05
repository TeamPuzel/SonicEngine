// Created by Lua (TeamPuzel) on May 29th 2025.
// Copyright (c) 2025 All rights reserved.
//
// The audio context header defines audio functionality.
// Normally I handle audio manually by computing a buffered waveform and maintaining its playback.
// For this game however it should be sufficient to offload the implementation to SDL3.
#pragma once
#include <primitive>

namespace rt {
    /// A type which manages audio playback.
    class AudioContext final {
      public:
        class Channel final {};
    };
}

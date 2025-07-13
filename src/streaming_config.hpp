#pragma once

// Streaming quality configuration
namespace StreamingConfig {
    // Video settings
    constexpr int MAX_HEIGHT = 1080;          // Maximum video height (1080p)
    constexpr int VIDEO_BITRATE = 8000;       // Video bitrate in kbps (8Mbps)
    constexpr int BUFFER_SIZE = 16000;        // Buffer size in kbps (16Mbps)
    constexpr int GOP_SIZE = 60;              // Group of pictures size (2 seconds at 30fps)
    constexpr int CRF_VALUE = 18;             // Constant Rate Factor (18 = high quality)
    
    // Audio settings  
    constexpr int AUDIO_BITRATE = 320;        // Audio bitrate in kbps (320k = high quality)
    constexpr int AUDIO_SAMPLE_RATE = 48000;  // Audio sample rate in Hz (48kHz)
    
    // Encoder settings
    const char* VIDEO_PRESET = "medium";      // x264 preset (medium = balanced quality/speed)
    const char* PIXEL_FORMAT = "yuv420p";    // Pixel format for compatibility
}

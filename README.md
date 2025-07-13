# MyChannel - Continuous Video Streaming System

A C++ application that creates a continuous live streaming channel by cycling through a playlist of videos and streaming them to YouTube Live in a loop.

## Project Overview

This application simulates a continuous broadcasting channel by:
- Managing a queue of video files
- Streaming each video to YouTube Live using RTMP
- Continuously looping through the playlist
- Providing real-time playback progress updates

## Features

- **Continuous Streaming**: Automatically cycles through a playlist of videos
- **YouTube Live Integration**: Streams directly to YouTube Live using RTMP protocol
- **Media Duration Detection**: Uses FFprobe to accurately determine video durations
- **Real-time Progress**: Shows playback progress for each video in real-time
- **Environment Configuration**: Secure handling of streaming credentials via environment variables
- **Queue Management**: Maintains a rotating queue of videos for continuous playback

## Prerequisites

- C++ compiler with C++11 support or later
- CMake for building
- FFmpeg with FFprobe (currently configured for Nix store path)
- YouTube Live Stream setup with RTMP URL and Stream Key

## Environment Setup

Before running the application, you need to set up your YouTube Live streaming credentials:

```bash
export YOUTUBE_RTMP_URL="your_youtube_rtmp_url"
export YOUTUBE_STREAM_KEY="your_stream_key"
```

Add these to your shell profile (e.g., `.zprofile`, `.bashrc`) for persistence.

## Building the Project

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Application

```bash
./mychannel
```

## Project Structure

```
mychannel/
├── src/
│   └── main.cpp          # Main application source
├── videos/
│   ├── video1_5s.webm    # Sample video files
│   └── video2_5s.webm
├── build/                # Build directory
├── CMakeLists.txt        # CMake configuration
└── README.md            # This file
```

## How It Works

1. **Initialization**: The application reads YouTube streaming credentials from environment variables
2. **Queue Setup**: Creates a queue with video files from the `videos/` directory
3. **Streaming Loop**: For each video:
   - Detects video duration using FFprobe
   - Streams the video to YouTube Live using FFmpeg
   - Displays real-time playback progress
   - Moves to the next video in the queue
4. **Continuous Operation**: Cycles through the playlist indefinitely

## Technical Details

- **Media Processing**: Uses FFmpeg for video streaming and FFprobe for duration detection
- **Threading**: Implements sleep-based timing for accurate playback simulation
- **Error Handling**: Includes error checking for missing environment variables and media processing failures
- **Memory Management**: Uses modern C++ practices with smart pointers and RAII

## Configuration

### Video Files
Place your video files in the `videos/` directory. Currently supports:
- WebM format (easily extendable to other formats)
- Any resolution/bitrate (FFmpeg handles encoding)

### Streaming Parameters
The application is configured for:
- Video: H.264 encoding, 3000k max bitrate, 6000k buffer
- Audio: AAC encoding, 128k bitrate, 44.1kHz sample rate
- Format: FLV for RTMP compatibility

## Use Cases

- **24/7 Live Channels**: Create continuous broadcasting channels
- **Content Rotation**: Automatically cycle through promotional videos
- **Testing**: Test streaming infrastructure with predictable content
- **Digital Signage**: Loop content for digital displays with live streaming

## Future Enhancements

- [ ] Dynamic playlist management (add/remove videos without restart)
- [ ] Multiple streaming destinations
- [ ] Web interface for remote control
- [ ] Configurable streaming parameters
- [ ] Support for different video formats
- [ ] Scheduling and time-based playlist changes

## License

This project is open source. Feel free to modify and distribute according to your needs.

## Troubleshooting

### Common Issues

1. **Missing Environment Variables**: Ensure `YOUTUBE_RTMP_URL` and `YOUTUBE_STREAM_KEY` are set
2. **FFmpeg Path**: Update the hardcoded Nix store paths if not using Nix
3. **Video Format Issues**: Ensure video files are in supported formats
4. **Network Issues**: Check internet connection and YouTube Live stream status

### Error Messages

- "Environment variables not set": Set the required YouTube streaming credentials
- "popen() failed": Check FFmpeg installation and paths
- "Error parsing duration": Verify video file integrity and format support

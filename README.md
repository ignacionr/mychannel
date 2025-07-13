# MyChannel - Live Streaming Queue Manager

A C++ application for managing and streaming video content to YouTube Live with an HTTP API for dynamic queue management, priority interruption support, and secure authentication.

## 🏗️ Architecture

The project follows a modular design with single responsibilities:

### Core Components

- **`main.cpp`** (67 lines) - Application orchestration and main streaming loop with interruption support
- **`utils.hpp/cpp`** (35 lines) - Shell command execution and URL validation utilities  
- **`media_queue.hpp/cpp`** (65 lines) - Thread-safe queue with priority insertion support
- **`media_info.hpp/cpp`** (55 lines) - Media duration detection (local files + YouTube)
- **`streaming.hpp/cpp`** (150+ lines) - Asynchronous YouTube streaming with process management and termination
- **`http_server.hpp/cpp`** (120+ lines) - HTTP API server with CORS support and token authentication

### Features

✅ **Async Streaming** - Non-blocking YouTube Live streaming with process tracking  
✅ **Priority Queue** - High-priority content can interrupt current streams  
✅ **Process Management** - Proper ffmpeg process tracking and termination  
✅ **Token Authentication** - Secure API endpoints with configurable authentication  
✅ **YouTube URL Support** - Direct streaming from YouTube videos using yt-dlp  
✅ **HTTP API** - RESTful endpoints for queue management  
✅ **Thread Safety** - Concurrent access to media queue  
✅ **CORS Enabled** - Web client compatibility  
✅ **Fallback Content** - Automatically plays `videos/News_Intro.mp4` when queue is empty  
✅ **MCP Server** - Model Context Protocol support for LLM integration  

## 🤖 MCP (Model Context Protocol) Support

MyChannel now includes full MCP server capabilities, allowing LLMs to programmatically control streaming operations:

- **8 MCP Tools** for queue management, stream control, and media analysis
- **No Additional Dependencies** - uses existing HTTP server infrastructure  
- **Authentication** - secure token-based access control
- **Real-time Control** - interrupt streams, manage queues, validate content

**MCP Endpoints:**
- `GET /mcp/tools` - Discover available tools
- `POST /mcp/call` - Execute MCP tools with JSON parameters

See **[MCP_README.md](./MCP_README.md)** for complete MCP documentation and usage examples.  

## 🚀 Quick Start

```bash
# Build
nix develop -c cmake -B build -S .
nix develop -c cmake --build build --config Debug

# Run
nix develop -c ./build/mychannel
```

## 📡 HTTP API

### Authentication

The API supports optional token-based authentication. Set the `MYCHANNEL_AUTH_TOKEN` environment variable to enable authentication for write operations (add, priority, clear). Read operations (status, queue) are always public.

**Authentication Methods:**
- Query parameter: `?token=your_token`
- HTTP header: `Authorization: Bearer your_token`

### Endpoints

| Method | Endpoint | Auth Required | Description |
|--------|----------|---------------|-------------|
| `GET` | `/status` | ❌ | Server health check |
| `GET` | `/queue` | ❌ | Get current queue contents |
| `POST` | `/queue/add?url=<youtube_url>` | ✅ | Add YouTube video to queue |
| `POST` | `/queue/add?path=<file_path>` | ✅ | Add local file to queue |
| `POST` | `/queue/priority?url=<youtube_url>` | ✅ | **NEW:** Add high-priority YouTube video (interrupts current stream) |
| `POST` | `/queue/priority?path=<file_path>` | ✅ | **NEW:** Add high-priority local file (interrupts current stream) |
| `POST` | `/queue/clear` | ✅ | Clear entire queue |

### Priority Queue Behavior

When you add content via the `/queue/priority` endpoint:
1. 🛑 **Current stream is immediately interrupted**
2. 📌 **New content is added to the front of the queue**
3. 🎬 **New content starts playing within seconds**
4. 🔄 **Normal queue processing resumes afterward**

## 🌐 Web Interface

Open `test_client.html` in your browser for a user-friendly queue management interface with:
- 🔐 **Authentication testing** - Validate your API token
- 📋 **Queue management** - View and manage current queue
- ➕ **Add content** - Add videos or local files to queue
- 🚨 **Priority insertion** - Add high-priority content that interrupts current stream
- 🗑️ **Queue clearing** - Clear all queued content

## 🔧 Environment Variables

```bash
# Required for streaming
export YOUTUBE_RTMP_URL="rtmp://a.rtmp.youtube.com/live2"
export YOUTUBE_STREAM_KEY="your-stream-key-here"

# Optional for API security
export MYCHANNEL_AUTH_TOKEN="your-secret-token-here"
```

**Security Notes:**
- If `MYCHANNEL_AUTH_TOKEN` is not set, all API endpoints are publicly accessible
- When set, write operations (add/priority/clear) require authentication
- Read operations (status/queue) are always public for monitoring purposes

## 📺 Fallback Content

When the queue is empty, the application automatically plays `videos/News_Intro.mp4`. Make sure this file exists in your videos directory. If the queue has content, it will continuously loop through the queued items.

## 📁 Project Structure

```
src/
├── main.cpp           # Main application orchestration with stream interruption
├── utils.hpp/cpp      # Command execution & URL utilities
├── media_queue.hpp/cpp # Thread-safe media queue with priority support
├── media_info.hpp/cpp # Duration detection (ffprobe/yt-dlp)
├── streaming.hpp/cpp  # Async YouTube streaming with process management
└── http_server.hpp/cpp # HTTP API server with authentication
```

Each module is focused on a single responsibility while supporting advanced features like process management and priority queuing.

## 🚨 Priority Queue Usage

The priority queue feature allows you to interrupt the current stream for urgent content:

### Use Cases
- **Breaking News**: Immediately broadcast urgent updates
- **Emergency Announcements**: Override scheduled content
- **Live Event Coverage**: Switch to live events instantly
- **Promotional Content**: Insert time-sensitive promotions

### Example Usage
```bash
# Add priority content via API
curl -X POST "http://localhost:8080/queue/priority?url=https://youtube.com/watch?v=URGENT_VIDEO&token=your_token"

# Or via web interface - click "Add Priority Item" button
```

### Process Management
The application now properly tracks ffmpeg processes and can:
- 🔍 **Detect running streams** using process pattern matching
- 🛑 **Gracefully terminate** current streams (SIGTERM → SIGKILL)
- 🔄 **Immediately start** priority content
- 📊 **Show real-time** ffmpeg output in console

## 🔐 Security Features

### Authentication
- **Token-based**: Simple bearer token authentication
- **Flexible**: Support for query parameters or HTTP headers
- **Optional**: Backward compatible when no token is set
- **Granular**: Only write operations require authentication

### Best Practices
```bash
# Generate a secure token
export MYCHANNEL_AUTH_TOKEN=$(openssl rand -hex 32)

# Use in API calls
curl -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
     -X POST "http://localhost:8080/queue/add?url=..."
```

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

Before running the application, you need to set up your YouTube Live streaming credentials and optionally configure authentication:

```bash
# Required streaming credentials
export YOUTUBE_RTMP_URL="your_youtube_rtmp_url"
export YOUTUBE_STREAM_KEY="your_stream_key"

# Optional API authentication (recommended for production)
export MYCHANNEL_AUTH_TOKEN="your_secure_token_here"
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

- [x] **Dynamic playlist management** - ✅ HTTP API implemented
- [x] **Web interface for remote control** - ✅ `test_client.html` available
- [x] **Priority queue system** - ✅ Stream interruption support
- [x] **Authentication system** - ✅ Token-based security
- [ ] Multiple streaming destinations
- [ ] Configurable streaming parameters via API
- [ ] Support for different video formats
- [ ] Scheduling and time-based playlist changes
- [ ] WebSocket support for real-time updates
- [ ] User management and role-based access
- [ ] Streaming analytics and monitoring

## License

This project is open source. Feel free to modify and distribute according to your needs.

## Troubleshooting

### Common Issues

1. **Missing Environment Variables**: Ensure `YOUTUBE_RTMP_URL` and `YOUTUBE_STREAM_KEY` are set
2. **Authentication Errors**: Check `MYCHANNEL_AUTH_TOKEN` if API returns 401 errors
3. **FFmpeg Path**: Update the hardcoded Nix store paths if not using Nix
4. **Process Termination**: If streams don't interrupt properly, check process permissions
5. **Video Format Issues**: Ensure video files are in supported formats
6. **Network Issues**: Check internet connection and YouTube Live stream status

### Error Messages

- "Environment variables not set": Set the required YouTube streaming credentials
- "Authentication required": Provide valid token via query param or Authorization header
- "No current process to terminate": Normal when no stream is running
- "popen() failed": Check FFmpeg installation and paths
- "Error parsing duration": Verify video file integrity and format support

### Debug Tips

1. **Console Output**: Use "Run MyChannel (No Debug)" configuration to see all output
2. **Process Monitoring**: Watch for PID messages to verify process tracking
3. **Authentication Testing**: Use the web interface to test token validity
4. **Network Debugging**: Monitor ffmpeg output for streaming issues

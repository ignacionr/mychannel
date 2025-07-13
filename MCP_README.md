# MyChannel MCP Server

A Model Context Protocol (MCP) server for managing live streaming operations. This allows LLMs to programmatically control video streaming, queue management, and content scheduling without requiring additional dependencies.

## 🎯 Overview

MyChannel now includes MCP (Model Context Protocol) support, enabling LLMs to:
- **Manage streaming queues** dynamically
- **Interrupt streams** for urgent content
- **Validate video sources** before streaming
- **Get real-time status** of streaming operations
- **Schedule content** with priority controls

## 🚀 Quick Start

1. **Start the server:**
```bash
nix develop -c ./build/mychannel
```

2. **Server runs on:** `http://localhost:8080`

3. **MCP endpoints:**
   - `GET /mcp/tools` - Discover available tools
   - `POST /mcp/call` - Execute tools

## 🔧 Available MCP Tools

### Queue Management

#### `add_video_to_queue`
Add videos to the streaming queue.

```json
{
  "tool": "add_video_to_queue",
  "params": {
    "source": "videos/my_video.mp4",
    "position": "back"
  }
}
```

**Parameters:**
- `source` (required): Video file path or YouTube URL
- `position` (optional): "front" or "back" (default: "back")

#### `add_priority_video`
Add high-priority video that immediately interrupts current stream.

```json
{
  "tool": "add_priority_video", 
  "params": {
    "source": "videos/breaking_news.mp4",
    "reason": "Emergency broadcast"
  }
}
```

**Parameters:**
- `source` (required): Video file path or YouTube URL  
- `reason` (optional): Reason for interruption

#### `get_streaming_queue`
Get current queue status and contents.

```json
{
  "tool": "get_streaming_queue",
  "params": {}
}
```

**Returns:**
```json
{
  "status": "success",
  "result": {
    "queue": ["video1.mp4", "video2.mp4"],
    "size": 2,
    "is_streaming": true
  }
}
```

#### `clear_streaming_queue`
Clear the entire streaming queue.

```json
{
  "tool": "clear_streaming_queue",
  "params": {}
}
```

### Stream Control

#### `get_stream_status`
Get comprehensive streaming status.

```json
{
  "tool": "get_stream_status",
  "params": {}
}
```

**Returns:**
```json
{
  "status": "success",
  "result": {
    "is_streaming": true,
    "queue_size": 3,
    "fallback_video": "videos/News_Intro.mp4",
    "server_status": "running"
  }
}
```

#### `interrupt_current_stream`
Immediately interrupt the current stream.

```json
{
  "tool": "interrupt_current_stream",
  "params": {
    "reason": "Technical difficulties"
  }
}
```

**Parameters:**
- `reason` (optional): Reason for interruption

### Media Analysis

#### `get_video_duration`
Get duration of video files or YouTube URLs.

```json
{
  "tool": "get_video_duration",
  "params": {
    "source": "videos/my_video.mp4"
  }
}
```

**Returns:**
```json
{
  "status": "success", 
  "result": {
    "duration": 120.5,
    "source": "videos/my_video.mp4"
  }
}
```

#### `validate_video_source`
Check if video source is accessible and valid.

```json
{
  "tool": "validate_video_source",
  "params": {
    "source": "https://youtube.com/watch?v=abc123"
  }
}
```

**Returns:**
```json
{
  "status": "success",
  "result": {
    "is_valid": true,
    "source_type": "youtube", 
    "source": "https://youtube.com/watch?v=abc123"
  }
}
```

## 🔐 Authentication

All MCP tool calls require authentication. Use one of these methods:

### Method 1: Authorization Header
```bash
curl -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_stream_status","params":{}}'
```

### Method 2: Query Parameter
```bash
curl -X POST "http://localhost:8080/mcp/call?token=YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_stream_status","params":{}}'
```

**Set your token:**
```bash
export MYCHANNEL_AUTH_TOKEN="your-secure-token-here"
```

## 🎬 Usage Examples

### Basic Video Management
```bash
# Add a video to queue
curl -s -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"add_video_to_queue","params":{"source":"videos/promo.mp4"}}'

# Check queue status
curl -s -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_streaming_queue","params":{}}'
```

### Emergency Broadcasting
```bash
# Interrupt current stream with breaking news
curl -s -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"add_priority_video","params":{"source":"videos/breaking_news.mp4","reason":"Emergency Alert"}}'
```

### YouTube Integration
```bash
# Add YouTube video to queue
curl -s -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"add_video_to_queue","params":{"source":"https://youtube.com/watch?v=dQw4w9WgXcQ"}}'

# Validate YouTube URL
curl -s -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"tool":"validate_video_source","params":{"source":"https://youtube.com/watch?v=dQw4w9WgXcQ"}}'
```

## 🤖 LLM Integration Patterns

### Automated News Channel
```python
# LLM can manage a news channel automatically
def schedule_news_content(stories):
    for story in stories:
        if story.priority == "breaking":
            # Interrupt current stream
            call_mcp_tool("add_priority_video", {
                "source": story.video_path,
                "reason": f"Breaking: {story.headline}"
            })
        else:
            # Add to regular queue  
            call_mcp_tool("add_video_to_queue", {
                "source": story.video_path
            })
```

### Content Validation Pipeline
```python
def validate_and_queue_content(video_sources):
    for source in video_sources:
        # Validate source first
        result = call_mcp_tool("validate_video_source", {
            "source": source
        })
        
        if result["is_valid"]:
            # Get duration for scheduling
            duration = call_mcp_tool("get_video_duration", {
                "source": source  
            })
            
            # Add to queue based on duration
            call_mcp_tool("add_video_to_queue", {
                "source": source,
                "position": "back" if duration > 300 else "front"
            })
```

### Real-time Event Response
```python
def handle_live_event(event):
    if event.urgency == "critical":
        # Immediately interrupt for critical events
        call_mcp_tool("interrupt_current_stream", {
            "reason": f"Critical event: {event.description}"
        })
        
        call_mcp_tool("add_priority_video", {
            "source": event.live_feed_url,
            "reason": "Live coverage"
        })
    else:
        # Queue for later
        call_mcp_tool("add_video_to_queue", {
            "source": event.video_url
        })
```

## 🛠️ Technical Details

### No Additional Dependencies
- Uses existing `httplib` HTTP server
- JSON parsing implemented without external libraries
- Reuses all existing MyChannel functionality
- Zero additional build dependencies

### Response Format
All MCP tools return standardized responses:

**Success:**
```json
{
  "status": "success",
  "result": { /* tool-specific data */ }
}
```

**Error:**
```json
{
  "status": "error", 
  "message": "Error description"
}
```

### Error Handling
- **401**: Authentication required
- **400**: Missing or invalid parameters
- **500**: Server errors (streaming issues, file not found, etc.)

## 🔍 Monitoring & Debugging

### Check Server Status
```bash
curl -s http://localhost:8080/status | python3 -m json.tool
```

### View Available Tools
```bash
curl -s http://localhost:8080/mcp/tools | python3 -m json.tool
```

### Monitor Queue Changes
```bash
# Watch queue in real-time
watch -n 2 'curl -s -X POST http://localhost:8080/mcp/call \
  -H "Authorization: Bearer $MYCHANNEL_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"tool\":\"get_streaming_queue\",\"params\":{}}" | python3 -m json.tool'
```

## 🎯 Use Cases for LLMs

1. **Automated Broadcasting**: LLMs can manage 24/7 channels with scheduled content
2. **Emergency Alerts**: Interrupt streams instantly for critical announcements  
3. **Content Curation**: Validate and organize video content automatically
4. **Event-driven Streaming**: Respond to external events with appropriate content
5. **Quality Control**: Pre-validate video sources before streaming
6. **Dynamic Scheduling**: Adjust content based on duration and priority
7. **Multi-source Management**: Handle both local files and YouTube content
8. **Real-time Analytics**: Monitor streaming status and queue metrics

This MCP implementation transforms MyChannel from a standalone streaming application into a powerful tool that LLMs can control programmatically, enabling sophisticated automated broadcasting scenarios without requiring any additional dependencies.

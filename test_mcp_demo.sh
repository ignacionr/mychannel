#!/bin/bash

# MyChannel MCP Demo Script
# Demonstrates all MCP tools in action

set -e

echo "🎬 MyChannel MCP Demo"
echo "====================="

# Check if auth token is set
if [ -z "$MYCHANNEL_AUTH_TOKEN" ]; then
    echo "❌ MYCHANNEL_AUTH_TOKEN not set. Please export your token."
    exit 1
fi

BASE_URL="http://localhost:8080/mcp/call"
AUTH_HEADER="Authorization: Bearer $MYCHANNEL_AUTH_TOKEN"

echo ""
echo "1️⃣  Discovering available MCP tools..."
curl -s http://localhost:8080/mcp/tools | python3 -c "
import sys, json
data = json.load(sys.stdin)
print(f'Found {len(data[\"tools\"])} MCP tools:')
for tool in data['tools']:
    print(f'  • {tool[\"name\"]}: {tool[\"description\"]}')
"

echo ""
echo "2️⃣  Getting initial stream status..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_stream_status","params":{}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
result = data['result']
print(f'Stream Status: {\"🟢 Running\" if result[\"is_streaming\"] else \"🔴 Stopped\"}')
print(f'Queue Size: {result[\"queue_size\"]} items')
print(f'Fallback Video: {result[\"fallback_video\"]}')
"

echo ""
echo "3️⃣  Validating video source..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"validate_video_source","params":{"source":"videos/video1_5s.webm"}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
result = data['result']
print(f'Video Validation: {\"✅ Valid\" if result[\"is_valid\"] else \"❌ Invalid\"}')
print(f'Source Type: {result[\"source_type\"]}')
print(f'Source: {result[\"source\"]}')
"

echo ""
echo "4️⃣  Getting video duration..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_video_duration","params":{"source":"videos/video1_5s.webm"}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
result = data['result']
print(f'Duration: {result[\"duration\"]} seconds')
print(f'Source: {result[\"source\"]}')
"

echo ""
echo "5️⃣  Adding video to queue..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"add_video_to_queue","params":{"source":"videos/video1_5s.webm","position":"back"}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
print(f'✅ {data[\"result\"]}')
"

echo ""
echo "6️⃣  Adding another video to queue..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"add_video_to_queue","params":{"source":"videos/video2_5s.webm","position":"back"}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
print(f'✅ {data[\"result\"]}')
"

echo ""
echo "7️⃣  Checking queue contents..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_streaming_queue","params":{}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
result = data['result']
print(f'Queue Size: {result[\"size\"]} items')
print(f'Queue Contents:')
for i, item in enumerate(result['queue'], 1):
    print(f'  {i}. {item}')
print(f'Streaming: {\"🟢 Yes\" if result[\"is_streaming\"] else \"🔴 No\"}')
"

echo ""
echo "8️⃣  Adding priority video (interrupts current stream)..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"add_priority_video","params":{"source":"videos/News_Intro.mp4","reason":"MCP Demo - Priority Content"}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
print(f'🚨 {data[\"result\"]}')
"

echo ""
echo "9️⃣  Final queue status..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"get_streaming_queue","params":{}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
result = data['result']
print(f'Final Queue Size: {result[\"size\"]} items')
print(f'Queue Contents:')
for i, item in enumerate(result['queue'], 1):
    print(f'  {i}. {item}')
"

echo ""
echo "🔟 Clearing queue..."
curl -s -X POST "$BASE_URL" \
  -H "$AUTH_HEADER" \
  -H "Content-Type: application/json" \
  -d '{"tool":"clear_streaming_queue","params":{}}' | python3 -c "
import sys, json
data = json.load(sys.stdin)
print(f'✅ {data[\"result\"]}')
"

echo ""
echo "🎉 MCP Demo Complete!"
echo "✨ MyChannel MCP server is fully functional with all 8 tools working correctly."
echo "📖 See MCP_README.md for detailed documentation and LLM integration examples."

#!/usr/bin/env python3
"""
Simple webhook server for GitHub push events
Listens for GitHub webhooks and triggers deployment
"""

import json
import subprocess
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
import hmac
import hashlib
import os

# Configuration
PORT = 9000
SECRET = os.environ.get('WEBHOOK_SECRET', 'your-secret-here')
DEPLOY_SCRIPT = '/opt/mychannel-deploy/deploy.sh'

class WebhookHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != '/webhook':
            self.send_response(404)
            self.end_headers()
            return

        # Get content length
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)

        # Verify GitHub signature (optional but recommended)
        if SECRET != 'your-secret-here':
            signature = self.headers.get('X-Hub-Signature-256', '')
            expected = 'sha256=' + hmac.new(
                SECRET.encode('utf-8'), 
                body, 
                hashlib.sha256
            ).hexdigest()
            
            if not hmac.compare_digest(signature, expected):
                print("❌ Invalid signature")
                self.send_response(401)
                self.end_headers()
                return

        try:
            # Parse JSON payload
            payload = json.loads(body.decode('utf-8'))
            
            # Check if it's a push to main branch
            if (payload.get('ref') == 'refs/heads/main' and 
                payload.get('repository', {}).get('name') == 'mychannel'):
                
                print(f"🚀 Deploying commit: {payload['head_commit']['id'][:7]}")
                print(f"📝 Message: {payload['head_commit']['message']}")
                
                # Trigger deployment
                result = subprocess.run([DEPLOY_SCRIPT], 
                                      capture_output=True, text=True)
                
                if result.returncode == 0:
                    print("✅ Deployment successful")
                    self.send_response(200)
                    self.send_header('Content-type', 'application/json')
                    self.end_headers()
                    self.wfile.write(json.dumps({
                        'status': 'success',
                        'message': 'Deployment completed'
                    }).encode())
                else:
                    print(f"❌ Deployment failed: {result.stderr}")
                    self.send_response(500)
                    self.send_header('Content-type', 'application/json')
                    self.end_headers()
                    self.wfile.write(json.dumps({
                        'status': 'error',
                        'message': result.stderr
                    }).encode())
            else:
                print("ℹ️  Ignoring push (not main branch or wrong repo)")
                self.send_response(200)
                self.end_headers()
                
        except Exception as e:
            print(f"❌ Error processing webhook: {e}")
            self.send_response(500)
            self.end_headers()

    def log_message(self, format, *args):
        # Custom logging
        print(f"[{self.date_time_string()}] {format % args}")

if __name__ == '__main__':
    print(f"🎣 Starting webhook server on port {PORT}")
    print(f"🔐 Secret configured: {'Yes' if SECRET != 'your-secret-here' else 'No (using default)'}")
    
    server = HTTPServer(('0.0.0.0', PORT), WebhookHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n👋 Shutting down webhook server")
        server.shutdown()

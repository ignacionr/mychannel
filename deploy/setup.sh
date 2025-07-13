#!/bin/bash

# MyChannel Deployment Setup Script
# Run this on dev.alquilacuida to set up continuous deployment

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

error() {
    echo -e "${RED}❌ $1${NC}"
    exit 1
}

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   error "This script must be run as root"
fi

log "🚀 Setting up MyChannel Continuous Deployment..."

# Install dependencies
log "📦 Installing dependencies..."
apt-get update
apt-get install -y python3 git curl

# Install Nix if not present
if ! command -v nix &> /dev/null; then
    log "📥 Installing Nix..."
    curl -L https://nixos.org/nix/install | sh
    source ~/.nix-profile/etc/profile.d/nix.sh
    
    # Enable flakes
    mkdir -p ~/.config/nix
    echo "experimental-features = nix-command flakes" > ~/.config/nix/nix.conf
else
    success "Nix already installed"
fi

# Create deployment directory
log "📁 Creating deployment directories..."
mkdir -p /opt/mychannel-deploy
mkdir -p /var/log/mychannel

# Copy deployment files
log "📋 Copying deployment scripts..."
cp webhook-server.py /opt/mychannel-deploy/
cp deploy.sh /opt/mychannel-deploy/
chmod +x /opt/mychannel-deploy/deploy.sh

# Install systemd service
log "⚙️  Installing webhook service..."
cp webhook.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable webhook.service

# Generate a random webhook secret if not set
WEBHOOK_SECRET=$(openssl rand -hex 32)
sed -i "s/your-webhook-secret-here/$WEBHOOK_SECRET/" /etc/systemd/system/webhook.service

# Start webhook service
log "🎣 Starting webhook service..."
systemctl start webhook.service

# Wait and check status
sleep 2
if systemctl is-active --quiet webhook.service; then
    success "Webhook service started successfully"
else
    error "Webhook service failed to start"
fi

# Show status
log "📊 Service status:"
systemctl status webhook.service --no-pager -l

# Get server IP
SERVER_IP=$(curl -s http://ipinfo.io/ip || hostname -I | awk '{print $1}')

success "🎉 Setup completed!"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}📡 WEBHOOK CONFIGURATION${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${BLUE}Webhook URL:${NC} http://$SERVER_IP:9000/webhook"
echo -e "${BLUE}Secret:${NC} $WEBHOOK_SECRET"
echo ""
echo "🔧 Add this webhook to your GitHub repository:"
echo "   Settings → Webhooks → Add webhook"
echo "   Payload URL: http://$SERVER_IP:9000/webhook"
echo "   Content type: application/json"
echo "   Secret: $WEBHOOK_SECRET"
echo "   Events: Just the push event"
echo ""
echo "🎯 Or run this command to test deployment manually:"
echo "   /opt/mychannel-deploy/deploy.sh"
echo ""
echo "📊 Monitor services:"
echo "   systemctl status webhook.service"
echo "   systemctl status mychannel.service"
echo "   journalctl -f -u webhook.service"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

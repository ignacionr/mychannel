#!/bin/bash
set -e

# Configuration
REPO_URL="https://github.com/ignacionr/mychannel.git"
DEPLOY_DIR="/opt/mychannel"
TEMP_DIR="/tmp/mychannel-deploy-$(date +%s)"
SERVICE_NAME="mychannel"
BUILD_USER="deploy"  # Non-root user for building

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Ensure we're running as root
if [[ $EUID -ne 0 ]]; then
   error "This script must be run as root"
fi

log "🚀 Starting MyChannel deployment..."

# Stop the service if it's running
if systemctl is-active --quiet $SERVICE_NAME; then
    log "🛑 Stopping $SERVICE_NAME service..."
    systemctl stop $SERVICE_NAME
fi

# Create deploy user if it doesn't exist
if ! id "$BUILD_USER" &>/dev/null; then
    log "👤 Creating $BUILD_USER user..."
    useradd -r -s /bin/bash -d /home/$BUILD_USER -m $BUILD_USER
fi

# Clone latest code
log "📥 Cloning latest code to $TEMP_DIR..."
git clone $REPO_URL $TEMP_DIR
cd $TEMP_DIR

# Get commit info
COMMIT_HASH=$(git rev-parse HEAD)
COMMIT_MSG=$(git log -1 --pretty=%B)
log "📝 Deploying commit: ${COMMIT_HASH:0:7}"
log "💬 Message: $COMMIT_MSG"

# Check if nix is available
log "🔍 Checking Nix installation..."
if [ -f /etc/profile.d/nix.sh ]; then
    source /etc/profile.d/nix.sh
fi

if ! command -v nix &> /dev/null; then
    error "Nix is not installed or not in PATH. Please install Nix first."
fi

success "Nix found: $(nix --version)"

# Build as deploy user (safer than root)
log "🔨 Building application..."
chown -R $BUILD_USER:$BUILD_USER $TEMP_DIR

# Source Nix environment and enable flakes
export NIX_CONFIG="experimental-features = nix-command flakes"
if [ -f /etc/profile.d/nix.sh ]; then
    source /etc/profile.d/nix.sh
fi

# Build with proper Nix environment
sudo -u $BUILD_USER bash -c "
    cd $TEMP_DIR
    if [ -f /etc/profile.d/nix.sh ]; then
        source /etc/profile.d/nix.sh
    fi
    export NIX_CONFIG='experimental-features = nix-command flakes'
    export PATH=/nix/var/nix/profiles/default/bin:\$PATH
    nix build --no-sandbox
"

if [ ! -f "$TEMP_DIR/result/bin/mychannel" ]; then
    error "Build failed - executable not found"
fi

success "Build completed successfully"

# Backup current deployment if it exists
if [ -d "$DEPLOY_DIR" ]; then
    BACKUP_DIR="/opt/mychannel-backup-$(date +%s)"
    log "💾 Backing up current deployment to $BACKUP_DIR..."
    cp -r "$DEPLOY_DIR" "$BACKUP_DIR"
fi

# Deploy new version
log "📦 Deploying new version..."
mkdir -p $DEPLOY_DIR
cp -r $TEMP_DIR/result/* $DEPLOY_DIR/
chown -R root:root $DEPLOY_DIR
chmod +x $DEPLOY_DIR/bin/mychannel

# Copy videos directory if it exists
if [ -d "$TEMP_DIR/videos" ]; then
    log "🎥 Copying videos directory..."
    cp -r $TEMP_DIR/videos $DEPLOY_DIR/
fi

# Create systemd service if it doesn't exist
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME.service"
if [ ! -f "$SERVICE_FILE" ]; then
    log "⚙️  Creating systemd service..."
    cat > $SERVICE_FILE << EOF
[Unit]
Description=MyChannel Streaming Service
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$DEPLOY_DIR
ExecStart=$DEPLOY_DIR/bin/mychannel
Restart=always
RestartSec=10
Environment=PATH=/usr/bin:/bin
Environment=HOME=/root

# Optional: Set environment variables
# Environment=STREAM_KEY=your_youtube_stream_key
# Environment=API_TOKEN=your_api_token

[Install]
WantedBy=multi-user.target
EOF
    systemctl daemon-reload
    systemctl enable $SERVICE_NAME
fi

# Start the service
log "🚀 Starting $SERVICE_NAME service..."
systemctl start $SERVICE_NAME

# Wait a moment and check if it's running
sleep 2
if systemctl is-active --quiet $SERVICE_NAME; then
    success "Service started successfully"
    
    # Show service status
    log "📊 Service status:"
    systemctl status $SERVICE_NAME --no-pager -l
else
    error "Service failed to start"
fi

# Cleanup
log "🧹 Cleaning up temporary files..."
rm -rf $TEMP_DIR

# Remove old backups (keep last 3)
log "🗂️  Cleaning old backups..."
ls -t /opt/mychannel-backup-* 2>/dev/null | tail -n +4 | xargs rm -rf 2>/dev/null || true

success "🎉 Deployment completed successfully!"
log "🔗 Commit: ${COMMIT_HASH:0:7}"
log "📱 Service: systemctl status $SERVICE_NAME"

# MyChannel Continuous Deployment Setup

## 🎉 Setup Complete!

Your MyChannel application now has **Continuous Deployment** configured with minimal GitHub setup required.

## 📡 Webhook Configuration

**Server:** `164.90.227.49`  
**Webhook URL:** `http://164.90.227.49:9000/webhook`  
**Secret:** `c206df039138e47050e54b61d92c75d60e8ea420d25aeb1329921f428b30c0b5`

## 🔧 GitHub Repository Setup

To enable automatic deployments, add this webhook to your GitHub repository:

1. Go to your repository: `https://github.com/ignacionr/mychannel`
2. Navigate to **Settings** → **Webhooks** → **Add webhook**
3. Configure:
   - **Payload URL:** `http://164.90.227.49:9000/webhook`
   - **Content type:** `application/json`
   - **Secret:** `c206df039138e47050e54b61d92c75d60e8ea420d25aeb1329921f428b30c0b5`
   - **Which events:** Just the push event
   - **Active:** ✅ Checked

## 🚀 How It Works

1. **Push to main** → GitHub sends webhook to your server
2. **Webhook receives** → Triggers deployment script
3. **Clone & Build** → Uses Nix to build the latest code
4. **Deploy** → Updates the running service
5. **Restart** → MyChannel service restarts with new version

## 📊 Monitoring

### Check Service Status
```bash
ssh dev.alquilacuida "systemctl status mychannel.service"
ssh dev.alquilacuida "systemctl status webhook.service"
```

### View Logs
```bash
ssh dev.alquilacuida "journalctl -u mychannel.service -f"
ssh dev.alquilacuida "journalctl -u webhook.service -f"
```

### Manual Deployment
```bash
ssh dev.alquilacuida "/opt/mychannel-deploy/deploy.sh"
```

## ⚙️ Configuration

### Environment Variables
The MyChannel service requires these environment variables (configured in `/etc/systemd/system/mychannel.service`):

```bash
Environment=YOUTUBE_RTMP_URL=rtmp://a.rtmp.youtube.com/live2
Environment=YOUTUBE_STREAM_KEY=your-stream-key-here
Environment=API_TOKEN=your-api-token-here
```

**⚠️ Important:** Update the `YOUTUBE_STREAM_KEY` and `API_TOKEN` with your actual values.

### Update Environment Variables
```bash
ssh dev.alquilacuida "sudo nano /etc/systemd/system/mychannel.service"
ssh dev.alquilacuida "sudo systemctl daemon-reload && sudo systemctl restart mychannel.service"
```

## 🔄 Deployment Process

Each deployment:
1. ✅ Stops the current service
2. ✅ Clones the latest code from GitHub
3. ✅ Builds using Nix (hermetic builds)
4. ✅ Creates backup of previous version
5. ✅ Deploys new version
6. ✅ Starts the service
7. ✅ Validates deployment

## 🛠️ Troubleshooting

### Deployment Failed
```bash
ssh dev.alquilacuida "journalctl -u webhook.service -n 50"
ssh dev.alquilacuida "/opt/mychannel-deploy/deploy.sh"
```

### Service Won't Start
```bash
ssh dev.alquilacuida "systemctl status mychannel.service"
ssh dev.alquilacuida "journalctl -u mychannel.service -n 20"
```

### Webhook Not Receiving
1. Check GitHub webhook deliveries in repository settings
2. Verify webhook is active and URL is correct
3. Check webhook service: `systemctl status webhook.service`

## 🏗️ Architecture

```
GitHub Push → Webhook → Deploy Script → Nix Build → Service Restart
     ↓            ↓           ↓            ↓            ↓
  main branch   Port 9000   /opt/deploy  Hermetic    New Version
```

## 🔐 Security Features

- ✅ **Webhook Secret Verification** - GitHub signatures validated
- ✅ **Branch Protection** - Only `main` branch deploys
- ✅ **Repository Validation** - Only `mychannel` repo
- ✅ **Non-root Build User** - Safer build process
- ✅ **Backup System** - Previous versions preserved

## 📁 File Structure

```
/opt/mychannel/              # Application deployment
├── bin/mychannel           # Main executable
└── videos/                 # Video assets

/opt/mychannel-deploy/      # Deployment system
├── webhook-server.py       # Webhook receiver
├── deploy.sh              # Deployment script
└── setup.sh              # Initial setup

/etc/systemd/system/       # Services
├── mychannel.service      # Application service
└── webhook.service       # Webhook service
```

---

## ✨ Next Steps

1. **Configure YouTube Stream Key** in the service environment variables
2. **Add the webhook** to your GitHub repository
3. **Push a commit** to test the deployment
4. **Monitor the logs** to see it working!

Your Continuous Deployment is ready! 🎊

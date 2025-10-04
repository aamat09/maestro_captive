let currentStep = 0;
let selectedNetwork = null;
let connectionCheckInterval = null;

const steps = [
    'step-welcome',
    'step-wifi-scan',
    'step-connection',
    'step-services',
    'step-complete'
];

function updateProgress() {
    const progress = ((currentStep + 1) / steps.length) * 100;
    document.getElementById('progress').style.width = progress + '%';
}

function showStep(stepIndex) {
    document.querySelectorAll('.step').forEach(step => step.classList.remove('active'));
    if (stepIndex >= 0 && stepIndex < steps.length) {
        document.getElementById(steps[stepIndex]).classList.add('active');
        currentStep = stepIndex;
        updateProgress();
    }
}

function nextStep() {
    if (currentStep < steps.length - 1) {
        showStep(currentStep + 1);
        if (currentStep === 1) {
            document.getElementById('scan-status').textContent = 'Ready to scan for networks';
        }
    }
}

function prevStep() {
    if (currentStep > 0) showStep(currentStep - 1);
}

async function scanWiFi(fullScan = false) {
    const wifiList = document.getElementById('wifi-list');
    const scanBtn = document.getElementById('scan-btn');
    const scanStatus = document.getElementById('scan-status');

    scanBtn.classList.add('loading');
    scanBtn.disabled = true;

    if (fullScan) {
        scanStatus.textContent = 'Scanning all channels...';
        wifiList.innerHTML = '<div class="loading">Full scan in progress...<br>Hotspot will briefly restart</div>';
    } else {
        scanStatus.textContent = 'Scanning nearby networks...';
        wifiList.innerHTML = '<div class="loading">Scanning nearby networks...</div>';
    }

    try {
        const url = fullScan ? '/api/wifi/scan?full_scan=true' : '/api/wifi/scan';
        const response = await fetch(url);
        const data = await response.json();

        if (data.status === 'success') {
            wifiList.innerHTML = '';
            scanStatus.textContent = `Found ${data.networks.length} networks`;

            if (data.networks.length === 0) {
                if (!fullScan) {
                    wifiList.innerHTML = `
                        <div class="loading">
                            No networks found on this channel.<br>
                            <strong>Try a full rescan to search all WiFi channels.</strong>
                        </div>
                    `;
                    // Show rescan button for empty results too
                    if (!document.getElementById('rescan-btn')) {
                        const rescanContainer = document.createElement('div');
                        rescanContainer.className = 'rescan-container';
                        rescanContainer.innerHTML = `
                            <p class="rescan-hint">⚠️ Full rescan will briefly disconnect the hotspot</p>
                            <button id="rescan-btn" class="btn btn-secondary" onclick="fullRescan()">
                                🔄 Full Rescan (All Channels)
                            </button>
                        `;
                        wifiList.parentElement.insertBefore(rescanContainer, wifiList);
                    }
                } else {
                    wifiList.innerHTML = '<div class="loading">No networks found. Try scanning again.</div>';
                }
                return;
            }

            // Show rescan button if not doing full scan
            if (!fullScan && !document.getElementById('rescan-btn')) {
                const rescanContainer = document.createElement('div');
                rescanContainer.className = 'rescan-container';
                rescanContainer.innerHTML = `
                    <p class="rescan-hint">Can't find your network? Try a full rescan (all channels)</p>
                    <button id="rescan-btn" class="btn btn-secondary" onclick="fullRescan()">
                        🔄 Full Rescan (All Channels)
                    </button>
                `;
                wifiList.parentElement.insertBefore(rescanContainer, wifiList);
            }

            data.networks.forEach(network => {
                const wifiItem = document.createElement('div');
                wifiItem.className = 'wifi-item';
                wifiItem.onclick = () => selectNetwork(network);

                const signalBars = getSignalBars(network.signal);
                const securityIcon = network.security ? '🔒' : '🔓';

                wifiItem.innerHTML = `
                    <div class="wifi-info">
                        <h4>${securityIcon} ${network.ssid}</h4>
                        <p>${network.security || 'Open Network'}</p>
                    </div>
                    <div class="signal-strength">
                        ${signalBars} <span>${network.signal}%</span>
                    </div>
                `;

                wifiList.appendChild(wifiItem);
            });
        } else {
            wifiList.innerHTML = '<div class="loading">Error scanning networks. Please try again.</div>';
            scanStatus.textContent = 'Scan failed';
        }
    } catch (error) {
        console.error('Error scanning WiFi:', error);
        wifiList.innerHTML = '<div class="loading">Network error. Please try again.</div>';
        scanStatus.textContent = 'Network error';
    } finally {
        scanBtn.classList.remove('loading');
        scanBtn.disabled = false;
    }
}

async function fullRescan() {
    await scanWiFi(true);
}

function getSignalBars(signal) {
    if (signal >= 80) return '📶📶📶📶';
    if (signal >= 60) return '📶📶📶';
    if (signal >= 40) return '📶📶';
    if (signal >= 20) return '📶';
    return '📶';
}

function selectNetwork(network) {
    selectedNetwork = network;

    // Update selected state
    document.querySelectorAll('.wifi-item').forEach(item => {
        item.classList.remove('selected');
    });
    event.currentTarget.classList.add('selected');

    // Show connection panel
    const panel = document.getElementById('wifi-connect-panel');
    const networkInfo = document.getElementById('selected-network-info');

    networkInfo.innerHTML = `
        <h4>${network.security ? '🔒' : '🔓'} ${network.ssid}</h4>
        <p>Signal: ${network.signal}% | Security: ${network.security || 'Open'}</p>
    `;

    // Clear previous password and status
    document.getElementById('wifi-password').value = '';
    document.getElementById('connection-status').innerHTML = '';

    // Show/hide password field based on security
    const passwordGroup = document.querySelector('.form-group');
    if (network.security && network.security !== 'Open') {
        passwordGroup.style.display = 'block';
        document.getElementById('wifi-password').focus();
    } else {
        passwordGroup.style.display = 'none';
    }

    panel.style.display = 'block';
}

function cancelConnection() {
    document.getElementById('wifi-connect-panel').style.display = 'none';
    document.querySelectorAll('.wifi-item').forEach(item => {
        item.classList.remove('selected');
    });
    selectedNetwork = null;
}

function togglePassword() {
    const passwordField = document.getElementById('wifi-password');
    const toggleBtn = document.querySelector('.password-toggle');

    if (passwordField.type === 'password') {
        passwordField.type = 'text';
        toggleBtn.textContent = '🙈';
    } else {
        passwordField.type = 'password';
        toggleBtn.textContent = '👁️';
    }
}

async function joinNetwork() {
    if (!selectedNetwork) return;

    const password = document.getElementById('wifi-password').value;
    const joinBtn = document.getElementById('join-btn');
    const statusDiv = document.getElementById('connection-status');

    // Validate password for secured networks
    if (selectedNetwork.security && selectedNetwork.security !== 'Open' && !password.trim()) {
        statusDiv.innerHTML = '<div class="connection-status error">Please enter the network password</div>';
        document.getElementById('wifi-password').focus();
        return;
    }

    joinBtn.classList.add('loading');
    joinBtn.disabled = true;
    statusDiv.innerHTML = '<div class="connection-status info">Connecting to network...</div>';

    try {
        const response = await fetch('/api/wifi/connect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                ssid: selectedNetwork.ssid,
                password: password
            })
        });

        const data = await response.json();

        if (data.status === 'success') {
            statusDiv.innerHTML = '<div class="connection-status success">Connection initiated! Validating...</div>';

            // Move to connection validation step
            setTimeout(() => {
                document.getElementById('final-network-name').textContent = selectedNetwork.ssid;
                showStep(2); // Connection validation step
                startConnectionValidation();
            }, 1000);
        } else {
            statusDiv.innerHTML = `<div class="connection-status error">Failed to connect: ${data.message || 'Unknown error'}</div>`;
            joinBtn.classList.remove('loading');
            joinBtn.disabled = false;
        }
    } catch (error) {
        console.error('Error connecting to WiFi:', error);
        statusDiv.innerHTML = '<div class="connection-status error">Network error. Please try again.</div>';
        joinBtn.classList.remove('loading');
        joinBtn.disabled = false;
    }
}

function startConnectionValidation() {
    const steps = ['step-connecting', 'step-validating', 'step-transitioning'];
    let currentValidationStep = 0;

    function updateValidationStep(stepIndex, status = 'active') {
        steps.forEach((step, index) => {
            const element = document.getElementById(step);
            element.classList.remove('active', 'completed', 'error');

            if (index < stepIndex) {
                element.classList.add('completed');
            } else if (index === stepIndex) {
                element.classList.add(status);
            }
        });
    }

    // Step 1: Connecting
    updateValidationStep(0);
    document.getElementById('connection-details').innerHTML = 'Establishing connection to ' + selectedNetwork.ssid + '...';

    // Check connection status
    connectionCheckInterval = setInterval(async () => {
        try {
            const response = await fetch('/api/wifi/status');
            const data = await response.json();

            if (data.connected && data.ssid === selectedNetwork.ssid) {
                clearInterval(connectionCheckInterval);

                // Step 2: Validating
                updateValidationStep(1);
                document.getElementById('connection-details').innerHTML =
                    `Connected to: ${data.ssid}\nSignal: ${data.signal}%\nValidating internet connectivity...`;

                // Validate internet connection
                setTimeout(async () => {
                    try {
                        // Step 3: Transitioning services
                        updateValidationStep(2);
                        document.getElementById('connection-details').innerHTML =
                            'Connection validated!\nSwitching off setup mode...\nTransitioning to home network...';

                        // Shutdown hotspot and DHCP services
                        await fetch('/api/services/hotspot/shutdown', { method: 'POST' });

                        setTimeout(() => {
                            updateValidationStep(3, 'completed');
                            document.getElementById('connection-details').innerHTML =
                                'Successfully connected to home network!\nSetup mode disabled.\nReady to start Home Assistant.';

                            // Move to services step
                            setTimeout(() => showStep(3), 2000);
                        }, 3000);

                    } catch (error) {
                        updateValidationStep(currentValidationStep, 'error');
                        document.getElementById('connection-details').innerHTML =
                            'Error during service transition: ' + error.message;
                    }
                }, 2000);

            } else if (currentValidationStep === 0) {
                document.getElementById('connection-details').innerHTML =
                    'Still connecting to ' + selectedNetwork.ssid + '...\nThis may take a moment.';
            }
        } catch (error) {
            clearInterval(connectionCheckInterval);
            updateValidationStep(currentValidationStep, 'error');
            document.getElementById('connection-details').innerHTML = 'Connection failed: ' + error.message;
        }
    }, 2000);

    // Timeout after 30 seconds
    setTimeout(() => {
        if (connectionCheckInterval) {
            clearInterval(connectionCheckInterval);
            updateValidationStep(currentValidationStep, 'error');
            document.getElementById('connection-details').innerHTML =
                'Connection timeout. Please check your password and try again.';
        }
    }, 30000);
}

async function startHomeAssistant() {
    const button = document.getElementById('start-ha-btn');
    const statusElement = document.getElementById('ha-status');

    button.style.display = 'none';
    statusElement.textContent = '⏳ Starting Home Assistant...';

    try {
        const response = await fetch('/api/services/homeassistant/start', { method: 'POST' });
        const data = await response.json();

        if (data.status === 'success') {
            statusElement.textContent = '✓ Running';
            statusElement.className = 'status connected';
            setTimeout(() => showStep(4), 2000);
        } else {
            statusElement.textContent = '❌ Failed to start';
            statusElement.className = 'status error';
            button.style.display = 'block';
        }
    } catch (error) {
        console.error('Error starting Home Assistant:', error);
        statusElement.textContent = '❌ Error occurred';
        statusElement.className = 'status error';
        button.style.display = 'block';
    }
}

async function disconnectAndReset() {
    const button = event.target;
    button.disabled = true;
    button.textContent = 'Disconnecting...';

    try {
        const response = await fetch('/api/wifi/reset', { method: 'POST' });
        const data = await response.json();

        if (data.status === 'success') {
            // Wait a moment for services to restart
            setTimeout(() => {
                // Redirect to setup mode
                showStep(0);
                button.disabled = false;
                button.textContent = 'Disconnect & Return to Setup';
            }, 3000);
        } else {
            alert('Failed to disconnect: ' + (data.message || 'Unknown error'));
            button.disabled = false;
            button.textContent = 'Disconnect & Return to Setup';
        }
    } catch (error) {
        console.error('Error disconnecting:', error);
        alert('Error disconnecting from network');
        button.disabled = false;
        button.textContent = 'Disconnect & Return to Setup';
    }
}

async function checkInitialWiFiStatus() {
    const wizard = document.querySelector('.setup-wizard');

    try {
        const response = await fetch('/api/wifi/status');
        const data = await response.json();

        if (data.status === 'success' && data.connected && data.ssid) {
            // WiFi is connected - show connected view
            document.getElementById('connected-ssid').textContent = data.ssid;
            document.getElementById('connected-signal').textContent = data.signal + '%';

            // Show the connected step instead of welcome
            document.querySelectorAll('.step').forEach(step => step.classList.remove('active'));
            document.getElementById('step-connected').classList.add('active');
        } else {
            // Not connected - show normal setup wizard
            showStep(0);
        }
    } catch (error) {
        console.error('Error checking WiFi status:', error);
        // On error, show normal setup
        showStep(0);
    } finally {
        // Show the wizard with fade-in effect
        wizard.style.opacity = '1';
    }
}

// Initialize the wizard
document.addEventListener('DOMContentLoaded', function() {
    checkInitialWiFiStatus();
});
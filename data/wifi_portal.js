class WifiManager {
    constructor() {
        // Add initialization check
        if (!document.getElementById('wifiModal')) return;

        // Main elements
        this.loadPortalHTML().then(() => {
            this.modal = document.getElementById('wifiModal');
            this.networkList = document.getElementById('networkListContainer');
            this.connectForm = document.getElementById('networkConnectForm');
            this.statusMessage = document.getElementById('wifiStatusMessage');
            this.connectionProgress = document.getElementById('connectionProgress');
            this.connectionResult = document.getElementById('connectionResult');
            this.successResult = document.getElementById('successResult');
            this.failureResult = document.getElementById('failureResult');

            // Form elements
            this.selectedNetworkInput = document.getElementById('selectedNetwork');
            this.passwordInput = document.getElementById('networkPassword');
            this.togglePasswordBtn = document.getElementById('togglePassword');

            // Result elements
            this.connectingNetworkSpan = document.getElementById('connectingNetwork');
            this.connectedNetworkSpan = document.getElementById('connectedNetwork');
            this.failedNetworkSpan = document.getElementById('failedNetwork');
            this.deviceIPSpan = document.getElementById('deviceIP');

            // Button elements
            this.refreshBtn = document.getElementById('refreshNetworks');
            this.cancelBtn = document.getElementById('cancelConnect');
            this.connectBtn = document.getElementById('connectNetwork');
            this.closeSuccessBtn = document.getElementById('closeSuccess');
            this.tryAgainBtn = document.getElementById('tryAgain');

            this.selectedSSID = '';
            this.scanTimer = null;
            this.checkConnectionTimer = null;

            this.isAPMode = false;
            this.setupEventListeners();
            this.checkWifiMode();
        });
    }

    async loadPortalHTML() {
        const response = await fetch('/wifi_portal.html');
        document.getElementById('wifiModal').innerHTML = await response.text();
    }

    setupEventListeners() {
        // Refresh button
        this.refreshBtn.addEventListener('click', () => this.scanNetworks());

        // Connect form buttons
        this.cancelBtn.addEventListener('click', () => this.showNetworkList());
        this.connectBtn.addEventListener('click', () => this.connectToNetwork());

        // Password visibility toggle
        this.togglePasswordBtn.addEventListener('click', () => this.togglePasswordVisibility());

        // Result buttons
        this.closeSuccessBtn.addEventListener('click', () => this.closeModal());
        this.tryAgainBtn.addEventListener('click', () => this.showNetworkList());
    }

    checkWifiMode() {
        fetch('/wifi/mode')
            .then(response => response.json())
            .then(data => {
                this.isAPMode = data.apMode;
                if (this.isAPMode) {
                    this.openModal();
                    this.scanNetworks();
                }
            })
            .catch(error => {
                console.error('Error checking WiFi mode:', error);
                // If we can't determine the mode, assume we're in AP mode if we're on a specific IP
                if (window.location.hostname === '192.168.4.1') {
                    this.isAPMode = true;
                    this.openModal();
                    this.scanNetworks();
                }
            });
    }

    openModal() {
        this.modal.style.display = 'block';
    }

    closeModal() {
        this.modal.style.display = 'none';
        // Clear any pending timers
        if (this.scanTimer) clearTimeout(this.scanTimer);
        if (this.checkConnectionTimer) clearTimeout(this.checkConnectionTimer);
    }

    scanNetworks() {
        // Show loading state
        this.networkList.innerHTML = `
            <div class="network-loading">
                <i class="fas fa-spinner fa-spin"></i>
                <span>Scanning for networks...</span>
            </div>
        `;

        // Animate refresh button
        this.refreshBtn.classList.add('refreshing');

        // Hide the connection form
        this.connectForm.style.display = 'none';

        // Request network scan
        fetch('/wifi/scan')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network scan failed');
                }

                // Wait for scan to complete (it takes a few seconds)
                this.scanTimer = setTimeout(() => this.fetchNetworks(), 5000);
            })
            .catch(error => {
                console.error('Scan error:', error);
                this.showScanError();
            });
    }

    fetchNetworks() {
        fetch('/wifi/networks')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Failed to fetch networks');
                }
                return response.json();
            })
            .then(networks => {
                this.displayNetworks(networks);
            })
            .catch(error => {
                console.error('Fetch networks error:', error);
                this.showScanError();
            })
            .finally(() => {
                // Stop refresh button animation
                this.refreshBtn.classList.remove('refreshing');
            });
    }

    displayNetworks(networks) {
        if (!networks || networks.length === 0) {
            this.networkList.innerHTML = `
                <div class="no-networks">
                    <p>No WiFi networks found</p>
                    <p>Please try scanning again</p>
                </div>
            `;
            return;
        }

        // Create network list HTML
        let html = '';
        networks.forEach(network => {
            html += `
                <div class="network-item" data-ssid="${network}">
                    <div class="network-icon">
                        <i class="fas fa-wifi"></i>
                    </div>
                    <div class="network-name">${network}</div>
                </div>
            `;
        });

        this.networkList.innerHTML = html;

        // Add click events to network items
        const networkItems = this.networkList.querySelectorAll('.network-item');
        networkItems.forEach(item => {
            item.addEventListener('click', () => {
                const ssid = item.getAttribute('data-ssid');
                this.selectNetwork(ssid);
            });
        });
    }

    showScanError() {
        this.networkList.innerHTML = `
            <div class="no-networks">
                <p>Error scanning for networks</p>
                <p>Please try again</p>
            </div>
        `;
        this.refreshBtn.classList.remove('refreshing');
    }

    selectNetwork(ssid) {
        this.selectedSSID = ssid;
        this.selectedNetworkInput.value = ssid;
        this.passwordInput.value = '';

        // Show the connection form
        this.connectForm.style.display = 'block';

        // Focus the password input
        this.passwordInput.focus();
    }

    showNetworkList() {
        // Hide forms and show network list
        this.connectionProgress.style.display = 'none';
        this.connectionResult.style.display = 'none';
        this.successResult.style.display = 'none';
        this.failureResult.style.display = 'none';
        this.connectForm.style.display = 'none';

        // Clear status message
        this.statusMessage.className = 'wifi-status-message';
        this.statusMessage.style.display = 'none';

        // Reset password field
        this.passwordInput.value = '';
        this.passwordInput.type = 'password';
        this.togglePasswordBtn.innerHTML = '<i class="fas fa-eye"></i>';
    }

    togglePasswordVisibility() {
        if (this.passwordInput.type === 'password') {
            this.passwordInput.type = 'text';
            this.togglePasswordBtn.innerHTML = '<i class="fas fa-eye-slash"></i>';
        } else {
            this.passwordInput.type = 'password';
            this.togglePasswordBtn.innerHTML = '<i class="fas fa-eye"></i>';
        }
    }

    connectToNetwork() {
        const ssid = this.selectedSSID;
        const password = this.passwordInput.value;

        if (!ssid) {
            this.showStatusMessage('Please select a network', 'error');
            return;
        }

        // Show connecting progress
        this.connectForm.style.display = 'none';
        this.connectionProgress.style.display = 'block';
        this.connectingNetworkSpan.textContent = ssid;

        // Send connection request
        const data = new URLSearchParams();
        data.append('ssid', ssid);
        data.append('password', password);

        fetch('/wifi/connect', {
            method: 'POST',
            body: data,
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            }
        })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Connection request failed');
                }

                // Poll for connection status
                this.checkConnectionStatus(ssid);
            })
            .catch(error => {
                console.error('Connection error:', error);
                this.showConnectionFailure(ssid);
            });
    }

    checkConnectionStatus(ssid) {
        // Clear any existing timer
        if (this.checkConnectionTimer) {
            clearTimeout(this.checkConnectionTimer);
        }

        fetch('/wifi/status')
            .then(response => response.json())
            .then(data => {
                if (data.connected) {
                    // Success!
                    this.showConnectionSuccess(ssid, data.ip);
                } else if (data.connecting) {
                    // Still connecting, check again in 1 second
                    this.checkConnectionTimer = setTimeout(() => {
                        this.checkConnectionStatus(ssid);
                    }, 1000);
                } else {
                    // Connection failed
                    this.showConnectionFailure(ssid);
                }
            })
            .catch(error => {
                console.error('Status check error:', error);
                // If we can't reach the device, it might be changing IP
                // Wait a bit and check if we have internet connection
                this.checkConnectionTimer = setTimeout(() => {
                    this.checkInternetConnection(ssid);
                }, 2000);
            });
    }

    checkInternetConnection(ssid) {
        // If the device successfully connected to WiFi, it might have a new IP
        // This function checks if we now have internet access (which would indicate success)

        fetch('https://www.google.com', { mode: 'no-cors', cache: 'no-store' })
            .then(() => {
                // If we can reach Google, we probably have internet now
                this.showConnectionSuccess(ssid, 'unknown (check router)');
            })
            .catch(() => {
                // Still can't reach the internet, connection probably failed
                this.showConnectionFailure(ssid);
            });
    }

    showConnectionSuccess(ssid, ip) {
        this.connectionProgress.style.display = 'none';
        this.connectionResult.style.display = 'block';
        this.successResult.style.display = 'block';

        this.connectedNetworkSpan.textContent = ssid;
        this.deviceIPSpan.textContent = ip;
    }

    showConnectionFailure(ssid) {
        this.connectionProgress.style.display = 'none';
        this.connectionResult.style.display = 'block';
        this.failureResult.style.display = 'block';

        this.failedNetworkSpan.textContent = ssid;
    }

    showStatusMessage(message, type) {
        this.statusMessage.textContent = message;
        this.statusMessage.className = 'wifi-status-message ' + type;
        this.statusMessage.style.display = 'block';
    }
}

// Initialize WiFi manager when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    // Check if WiFi modal elements exist before initializing
    if (document.getElementById('wifiModal')) {
        window.wifiManager = new WifiManager();
    }
});
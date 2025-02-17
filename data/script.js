class EdgeImpulseIntegration {
    constructor() {
        this.apiKey = '';
        this.projectId = '';
        this.deviceName = '';
        this.category = 'training';
        this.label = '';

        // Load saved configuration
        this.loadConfig();
        this.setupEventListeners();

        // Setup toggle functionality
        this.setupConfigToggle();
    }

    async loadConfig() {
        try {
            const response = await fetch('/loadConfig');
            if (response.ok) {
                const config = await response.json();
                this.apiKey = config.apiKey || '';
                this.projectId = config.projectId || '';
                this.deviceName = config.deviceName || '';

                // Populate UI
                document.getElementById('apiKey').value = this.apiKey;
                document.getElementById('projectID').value = this.projectId;
                document.getElementById('deviceName').value = this.deviceName;
            }
        } catch (error) {
            console.error('Error loading configuration:', error);
        }
    }

    setupEventListeners() {
        // document.getElementById('saveConfig').addEventListener('click', () => this.saveConfig());
        document.getElementById('saveConfig').addEventListener('click', () => this.saveConfig());
        document.getElementById('uploadToEI').addEventListener('click', () => this.uploadImages());
    }

    setupConfigToggle() {
        const toggleBtn = document.getElementById('toggleConfig');
        const configPanel = document.querySelector('.config-panel');

        toggleBtn.addEventListener('click', () => {
            const isVisible = configPanel.style.display !== 'none';
            configPanel.style.display = isVisible ? 'none' : 'block';
            toggleBtn.classList.toggle('active');
        });
    }

    async saveConfig() {
        const config = {
            apiKey: document.getElementById('apiKey').value,
            projectId: document.getElementById('projectID').value,
            deviceName: document.getElementById('deviceName').value
        };

        try {
            const response = await fetch('/saveConfig', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `config=${JSON.stringify(config)}`
            });

            if (response.ok) {
                this.apiKey = config.apiKey;
                this.projectId = config.projectId;
                this.deviceName = config.deviceName;
                alert('Configuration saved!');
            } else {
                alert('Failed to save configuration');
            }
        } catch (error) {
            console.error('Error saving configuration:', error);
            alert('Error saving configuration');
        }
    }

    // Simple encryption/decryption functions
    encrypt(text) {
        return btoa(text);  // For demo - replace with more secure encryption
    }

    decrypt(encrypted) {
        return atob(encrypted);  // For demo - replace with more secure decryption
    }

    async uploadImages() {
        const images = document.querySelectorAll('.preview-image');
        const category = document.getElementById('category').value;
        const label = document.getElementById('label').value;

        if (!this.apiKey || !this.projectId) {
            alert('Please configure Edge Impulse settings first');
            return;
        }
        if (!label) {
            alert('Please enter a label for the images');
            return;
        }

        // Show upload modal
        const modal = document.getElementById('uploadModal');
        const progressBar = document.getElementById('uploadProgress');
        const uploadCount = document.getElementById('uploadCount');
        const uploadComplete = document.getElementById('uploadComplete');

        modal.style.display = 'block';
        progressBar.style.width = '0%';
        uploadComplete.style.display = 'none';
        uploadCount.textContent = `0/${images.length}`;

        let successCount = 0;
        for (let i = 0; i < images.length; i++) {
            const img = images[i];
            try {
                const response = await fetch(img.src);
                const blob = await response.blob();
                await this.uploadToEdgeImpulse(blob, label, category, i);

                successCount++;
                progressBar.style.width = `${(successCount / images.length) * 100}%`;
                uploadCount.textContent = `${successCount}/${images.length}`;
            } catch (error) {
                console.error(`Error uploading image ${i + 1}:`, error);
            }
        }

        // Show completion message
        uploadComplete.style.display = 'block';
    }

    async uploadToEdgeImpulse(blob, label, category, index) {
        const filename = `${label}_${index}.jpg`;

        const formData = new FormData();
        formData.append('data', blob, filename);

        try {
            const endpoint = category === 'training' ? 'training' : 'testing';
            const response = await fetch(`https://ingestion.edgeimpulse.com/api/${endpoint}/files`, {
                method: 'POST',
                headers: {
                    'x-api-key': this.apiKey,
                    'x-label': label.trim().toLowerCase(), // Ensure label is clean
                    'x-add-date-id': '1',  // Let EI handle unique filenames: https://docs.edgeimpulse.com/reference/data-ingestion/ingestion-api#header-parameters
                },
                body: formData
            });

            // console.log(label.trim().toLowerCase());

            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(`HTTP error! status: ${response.status}, message: ${errorText}`);
            }

            return response.json();
        } catch (error) {
            console.log('Upload error details:', error);
            throw error;
        }
    }
}


class CameraInterface {
    constructor() {
        // Stream frame
        this.streamImg = document.getElementById('streamImg');

        // Buttons
        this.startButton = document.getElementById('startCollecting');
        this.clearButton = document.getElementById('clearAll');
        this.downloadButton = document.getElementById('download');
        this.uploadButton = document.getElementById('uploadToEI')

        // Auto capture elements
        this.startAutoCaptureBtn = document.getElementById('startAutoCapture');
        this.totalCapturesInput = document.getElementById('totalCaptures');

        // saved image table
        this.imageTableBody = document.getElementById('imageTableBody');

        // Image preview
        this.imageModal = document.getElementById('imageModal');
        this.modalImage = document.getElementById('modalImage');

        this.imageCount = 0;
        this.isAutoCapturing = false;
        this.autoCapturePause = 2000; // 2 seconds  

        this.initializeStream();
        this.setupEventListeners();

        // Initialize Edge Impulse integration
        this.edgeImpulse = new EdgeImpulseIntegration();

        // Show upload button when images are present
        this.updateButtonVisibility = function () {
            if (this.imageCount > 0) {
                this.uploadButton.style.display = 'inline-block';
                this.clearButton.style.display = 'inline-block';
                this.downloadButton.style.display = 'inline-block';

            } else {
                this.updateButton.style.display = 'none';
                this.clearButton.style.display = 'none';
                this.downloadButton.style.display = 'none';
            }
        };
    }

    initializeStream() {
        this.streamImg.src = "/stream"
    }

    setupEventListeners() {
        this.startButton.addEventListener('click', () => this.captureImage());
        this.clearButton.addEventListener('click', () => this.clearAllImages());
        this.imageModal.addEventListener('click', (e) => this.handleModalClick(e));
        this.downloadButton.addEventListener('click', () => this.downloadImages());
        document.addEventListener('keydown', (e) => this.handleKeyPress(e));
        this.startAutoCaptureBtn.addEventListener('click', () => this.toggleAutoCapture());
    }

    async captureImage() {
        try {
            const response = await fetch('/capture');
            const imageBlob = await response.blob();
            const imageUrl = URL.createObjectURL(imageBlob);

            this.addImageToTable(imageUrl);
            this.updateButtonVisibility();
        } catch (error) {
            console.error('Error capturing image:', error);
        }
    }

    async toggleAutoCapture() {
        if (this.isAutoCapturing) {
            this.isAutoCapturing = false;
            this.startAutoCaptureBtn.innerHTML = '<i class="fas fa-camera"></i> Start Auto Capture';
            return;
        }

        const totalCaptures = parseInt(this.totalCapturesInput.value);
        if (!totalCaptures || totalCaptures < 1 || totalCaptures > 50) {
            alert('Please enter a number between 1 and 50');
            return;
        }

        this.isAutoCapturing = true;
        this.startAutoCaptureBtn.innerHTML = '<i class="fas fa-stop"></i> Stop Auto Capture';

        let captureCount = 0;
        while (this.isAutoCapturing && captureCount < totalCaptures) {
            await this.captureImage();
            captureCount++;

            if (captureCount < totalCaptures) {
                await new Promise(resolve => setTimeout(resolve, this.autoCapturePause));
            }
        }

        this.isAutoCapturing = false;
        this.startAutoCaptureBtn.innerHTML = '<i class="fas fa-camera"></i> Start Auto Capture';
    }

    addImageToTable(imageUrl) {
        const row = this.imageTableBody.insertRow();

        // Index cell
        const indexCell = row.insertCell();
        indexCell.textContent = this.imageCount + 1;

        // Image cell
        const imgCell = row.insertCell();
        const img = document.createElement('img');
        img.src = imageUrl;
        img.className = 'preview-image';
        img.addEventListener('click', () => this.showModal(imageUrl));
        imgCell.appendChild(img);

        // Filename cell
        const filenameCell = row.insertCell();
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const filename = `IMG_${timestamp}_96x96.jpg`;
        filenameCell.textContent = filename;

        // Delete button cell
        const deleteCell = row.insertCell();
        const deleteBtn = document.createElement('button');
        deleteBtn.innerHTML = '<i class="fas fa-trash"></i>';
        deleteBtn.className = 'delete-btn';
        deleteBtn.addEventListener('click', () => this.deleteRow(row));
        deleteCell.appendChild(deleteBtn);

        this.imageCount++;
        this.updateButtonVisibility();
    }

    updateRowNumbers() {
        const rows = this.imageTableBody.getElementsByTagName('tr');
        for (let i = 0; i < rows.length; i++) {
            rows[i].cells[0].textContent = i + 1;
        }
    }

    deleteRow(row) {
        if (confirm('Are you sure you want to delete this image?')) {
            row.remove();
            this.imageCount--;
            this.updateRowNumbers();
            if (this.imageCount <= 1) {
                this.clearButton.style.display = 'none';
                this.downloadButton.style.display = 'none';
            }
        }
    }

    updateButtonVisibility() {
        if (this.imageCount > 1) {
            this.clearButton.style.display = 'inline-block';
            this.downloadButton.style.display = 'inline-block';
        }
    }

    async clearAllImages() {
        try {
            const response = await fetch('/clear', {
                method: 'POST', // ** Use POST instead of GET
                headers: {
                    'Content-Type': 'application/json'
                }
            });
            if (!response.ok) {
                throw new Error('Failed to clear images');
            }
            this.imageTableBody.innerHTML = '';
            this.imageCount = 0;
            this.clearButton.style.display = 'none';
            this.downloadButton.style.display = 'none';
        } catch (error) {
            console.error('Error clearing images:', error);
        }
    }

    async downloadImages() {
        const label = prompt("Enter Label");
        if (!label) return;

        const zip = new JSZip();
        const images = document.querySelectorAll('.preview-image');
        const timestamp = new Date().toISOString().split('T')[0];

        for (let i = 0; i < images.length; i++) {
            const img = images[i];
            const response = await fetch(img.src);
            const blob = await response.blob();
            const filename = `${label}_${timestamp}_${i + 1}.jpg`;
            zip.file(filename, blob);
        }

        const content = await zip.generateAsync({ type: "blob" });
        const link = document.createElement('a');
        link.href = URL.createObjectURL(content);
        link.download = `${label}.zip`;
        link.click();
    }

    showModal(imageUrl) {
        this.modalImage.src = imageUrl;
        this.imageModal.style.display = 'block';
    }

    handleModalClick(e) {
        if (e.target === this.imageModal) {
            this.imageModal.style.display = 'none';
        }
    }

    handleKeyPress(e) {
        if (e.key === 'Escape') {
            this.imageModal.style.display = 'none';
        }
    }
}

function setupThemeToggle() {
    const themeToggle = document.createElement('button');
    themeToggle.className = 'theme-toggle';
    themeToggle.innerHTML = '<i class="fas fa-moon"></i>';
    document.body.appendChild(themeToggle);

    // Check for saved theme preference
    const savedTheme = localStorage.getItem('theme') || 'light';
    document.documentElement.setAttribute('data-theme', savedTheme);
    updateThemeIcon(savedTheme);

    themeToggle.addEventListener('click', () => {
        const currentTheme = document.documentElement.getAttribute('data-theme');
        const newTheme = currentTheme === 'dark' ? 'light' : 'dark';

        document.documentElement.setAttribute('data-theme', newTheme);
        localStorage.setItem('theme', newTheme);
        updateThemeIcon(newTheme);
    });
}

function updateThemeIcon(theme) {
    const icon = document.querySelector('.theme-toggle i');
    icon.className = theme === 'dark' ? 'fas fa-sun' : 'fas fa-moon';
}

document.addEventListener('DOMContentLoaded', () => {
    new CameraInterface();
    setupThemeToggle();
});
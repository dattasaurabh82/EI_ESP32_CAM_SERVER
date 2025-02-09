class CameraInterface {
    constructor() {
        this.streamImg = document.getElementById('streamImg');
        this.startButton = document.getElementById('startCollecting');
        this.clearButton = document.getElementById('clearAll');
        this.downloadButton = document.getElementById('download');
        this.imageTableBody = document.getElementById('imageTableBody');
        this.imageModal = document.getElementById('imageModal');
        this.modalImage = document.getElementById('modalImage');
        this.imageCount = 0;

        this.initializeStream();
        this.setupEventListeners();
    }

    initializeStream() {
        this.streamImg.src = "/stream"
    }

    setupEventListeners() {
        this.startButton.addEventListener('click', () => this.captureImage());
        this.clearButton.addEventListener('click', () => this.clearAllImages());
        this.imageModal.addEventListener('click', (e) => this.handleModalClick(e));
        document.addEventListener('keydown', (e) => this.handleKeyPress(e));
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

    addImageToTable(imageUrl) {
        const row = this.imageTableBody.insertRow();
        const cell = row.insertCell();
        const img = document.createElement('img');
        img.src = imageUrl;
        img.className = 'preview-image';
        img.addEventListener('click', () => this.showModal(imageUrl));
        cell.appendChild(img);
        
        this.imageCount++;
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
            method: 'POST', // Use POST instead of GET
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

// Initialize the camera interface when the DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    new CameraInterface();
});
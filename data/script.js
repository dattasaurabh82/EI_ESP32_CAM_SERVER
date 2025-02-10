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
        this.downloadButton.addEventListener('click', () => this.downloadImages());
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

    // addImageToTable(imageUrl) {
    //     const row = this.imageTableBody.insertRow();
    //     const cell = row.insertCell();

    //     // Create image element
    //     const img = document.createElement('img');
    //     img.src = imageUrl;
    //     img.className = 'preview-image';
    //     img.addEventListener('click', () => this.showModal(imageUrl));

    //     // Create filename element
    //     const now = new Date();
    //     const filename = `IMG_${now.toISOString().replace(/[:.]/g, '-')}_${img.width}x${img.height}.jpg`;
    //     const filenameElement = document.createElement('div');
    //     filenameElement.textContent = filename;
    //     filenameElement.className = 'filename';

    //     // Append to table
    //     cell.appendChild(img);
    //     cell.appendChild(filenameElement);

    //     this.imageCount++;
    // }

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

// Initialize the camera interface when the DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    new CameraInterface();
});
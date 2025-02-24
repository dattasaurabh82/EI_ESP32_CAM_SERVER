document.addEventListener('DOMContentLoaded', () => {
    const button = document.querySelector('esp-web-install-button');
    // button.manifest = `manifest.json`;

    // For GitHub Pages, use the full path to your manifest
    button.manifest = `${window.location.origin}/EI_ESP32_CAM_SERVER/manifest.json`;
  });
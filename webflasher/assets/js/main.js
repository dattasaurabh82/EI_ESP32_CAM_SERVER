// document.addEventListener('DOMContentLoaded', () => {
//     const button = document.querySelector('esp-web-install-button');
//     // button.manifest = `manifest.json`;

//     // For GitHub Pages, use the full path to your manifest
//     button.manifest = `${window.location.origin}/EI_ESP32_CAM_SERVER/manifest.json`;
// });

// document.querySelectorAll('input[name="type"]').forEach(radio =>
//     radio.addEventListener("change", () => {
//         const button = document.querySelector('esp-web-install-button');

//         // For GitHub Pages, use the full path to your manifest
//         button.manifest = `${window.location.origin}/EI_ESP32_CAM_SERVER/manifest_${radio.value}.json`;
//         button.classList.remove('invisible');
//     }
//     ));


document.addEventListener('DOMContentLoaded', () => {
    const button = document.querySelector('esp-web-install-button');
    const radioButtons = document.querySelectorAll('input[name="type"]');
    const manifestDisplay = document.getElementById('current-manifest');

    // Map radio values to manifest filenames
    const manifestMap = {
        'xiaoesp32s3': 'manifest_xiao.json',
        'aithinkercam': 'manifest_aithinker.json'
    };

    // Determine if we're running locally or on GitHub Pages
    const isLocal = window.location.hostname === 'localhost' ||
        window.location.hostname === '127.0.0.1' ||
        window.location.hostname.startsWith('192.168.');

    // Get the base path for the manifests based on deployment environment
    function getBasePath() {
        if (isLocal) {
            return '';  // Local environment, use relative paths
        } else {
            // For GitHub Pages, we need to detect the repository name or path
            const isGitHubPages = window.location.hostname.endsWith('github.io');

            if (isGitHubPages) {
                // Extract repo path from github.io URL
                const pathSegments = window.location.pathname.split('/').filter(Boolean);
                if (pathSegments.length > 0) {
                    // Join all path segments up to the site root
                    return `${window.location.origin}/${pathSegments.join('/')}`;
                }
            }

            // Default: assume we're at the site root
            return window.location.origin;
        }
    }

    // Get the base path once
    const basePath = getBasePath();
    console.log(`Running in ${isLocal ? 'local' : 'remote'} environment`);
    console.log(`Base path detected as: ${basePath}`);

    // Function to get the correct manifest path
    function getManifestPath(radioValue) {
        const manifestFile = manifestMap[radioValue];
        if (isLocal) {
            return manifestFile;
        } else {
            // For GitHub Pages or other remote hosting
            return `${basePath}/${manifestFile}`;
        }
    }

    // Function to update debug info display
    window.updateDebugInfo = function () {
        const button = document.querySelector('esp-web-install-button');
        const manifestDisplay = document.getElementById('current-manifest');

        if (manifestDisplay) {
            manifestDisplay.textContent = button.manifest || 'Not set';
            console.log('Current manifest:', button.manifest);

            // Fetch and display manifest contents
            if (button.manifest) {
                fetch(button.manifest)
                    .then(response => {
                        if (!response.ok) {
                            throw new Error(`HTTP error! Status: ${response.status}`);
                        }
                        return response.json();
                    })
                    .then(manifestData => {
                        // Create a formatted display of the manifest contents
                        const manifestContent = document.getElementById('manifest-content');
                        if (manifestContent) {
                            manifestContent.textContent = JSON.stringify(manifestData, null, 2);
                        }

                        console.log('Manifest contents:', manifestData);
                    })
                    .catch(error => {
                        console.error('Error fetching manifest:', error);
                        const manifestContent = document.getElementById('manifest-content');
                        if (manifestContent) {
                            manifestContent.textContent = `Error fetching manifest: ${error.message}`;
                        }
                    });
            }
        }
    };

    // Add event listeners to radio buttons
    radioButtons.forEach(radio => {
        radio.addEventListener('change', () => {
            if (radio.checked) {
                button.manifest = getManifestPath(radio.value);
                console.log(`Selected board: ${radio.value}`);
                console.log(`Manifest path set to: ${button.manifest}`);

                // Update the debug display if it exists
                updateDebugInfo();
            }
        });
    });

    // Set default selection (first radio button)
    if (radioButtons.length > 0) {
        radioButtons[0].checked = true;
        button.manifest = getManifestPath(radioButtons[0].value);
        console.log(`Default selection: ${radioButtons[0].value}`);
        console.log(`Initial manifest path: ${button.manifest}`);

        // Update debug display
        setTimeout(updateDebugInfo, 500);
    }
});
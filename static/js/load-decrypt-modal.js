// Dynamically load the "Decryption" modal
async function loadDecryptionModal() {
    const response = await fetch('/static/html/partial.decrypt-modal.html'); // Fetch the modal HTML snippet
    if (response.ok) {
        const modalHtml = await response.text();
        document.getElementById('decrypt-modal-section').innerHTML = modalHtml; // Inject the modal into the DOM

        // Initialize modal-related JavaScript after loading
        initializeDecryptionModal();
    } else {
        showNotification('Failed to load decryption modal', 'error');
    }
}

// Initialize the decryption modal
function initializeDecryptionModal() {
    const modal = document.getElementById('decrypt-modal');
    const openModalButton = document.getElementById('open-decrypt-modal');
    const closeButton = modal.querySelector('.close-button');
    const decryptionForm = document.getElementById('decrypt-form');
    const decryptionRootInput = document.getElementById('decrypt-location-input');

    // Open the modal when the "Decrypt" button is clicked
    openModalButton.addEventListener('click', () => {
        modal.style.display = 'block';
    });

    // Close the modal when the close button is clicked
    closeButton.addEventListener('click', () => {
        modal.style.display = 'none';
    });

    // Close the modal when clicking outside of it
    window.addEventListener('click', (event) => {
        if (event.target === modal) {
            modal.style.display = 'none';
        }
    });

    // Handle the form submission
    decryptionForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const botId = document.getElementById('client-id').value; // Hidden input for bot ID
        const decryptionRoot = decryptionRootInput.value.trim();

        if (!botId) {
            showNotification('Bot ID is missing.', 'error');
            return;
        }

        if (!decryptionRoot) {
            showNotification('Please enter a root location.', 'error');
            return;
        }

        try {
            const response = await fetch('/api/decrypt', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `bot_id=${encodeURIComponent(botId)}&file_path=${encodeURIComponent(decryptionRoot)}`,
            });

            if (response.ok) {
                const result = await response.json();
                showNotification(`Decryption request successfully sent to client.`, 'success');
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.error}`, 'error');
            }
        } catch (error) {
            console.error('Error requesting decryption:', error);
            showNotification('Failed to send decryption request.', 'error');
        }

        // Close the modal
        modal.style.display = 'none';
    });
}

// Call the function to load the decryption modal
loadDecryptionModal();
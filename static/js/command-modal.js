// Helper function to get query parameters from the URL
function getQueryParam(param) {
    const urlParams = new URLSearchParams(window.location.search);
    return urlParams.get(param);
}

// Get modal elements
const modal = document.getElementById('command-modal');
const openModalButton = document.getElementById('open-command-modal');
const closeButton = document.querySelector('.close-button');
const commandForm = document.getElementById('command-form');
const clientIdInput = document.getElementById('client-id'); // Hidden input for bot_id

// Open the modal
openModalButton.addEventListener('click', () => {
    // Check for Bot ID in the URL first, then fallback to the input field
    const botId = getQueryParam('id');

    if (!botId) {
        alert('Bot ID is missing. Please ensure the bot ID is available in the URL.');
        return;
    }

    // Prefill the hidden input field with the bot ID
    clientIdInput.value = botId;

    // Open the modal
    modal.style.display = 'block';
});

// Close the modal
closeButton.addEventListener('click', () => {
    modal.style.display = 'none';
});

// Close the modal when clicking outside of it
window.addEventListener('click', (event) => {
    if (event.target === modal) {
        modal.style.display = 'none';
    }
});

// Handle form submission
commandForm.addEventListener('submit', async (event) => {
    event.preventDefault();

    const botId = clientIdInput.value.trim(); // Get the bot ID from the hidden input
    const cmdId = document.getElementById('cmd-id-input').value.trim();
    const program = document.getElementById('program-input').value.trim();
    const params = document.getElementById('params-input').value.trim();
    const delay = document.getElementById('delay-input').value.trim() || '0';
    const expectedCode = document.getElementById('expected-code-input').value.trim() || '0';

    if (!botId) {
        alert('Bot ID is missing. Please ensure the bot ID is available.');
        return;
    }

    if (!program) {
        alert('Please enter a program.');
        return;
    }

    try {
        const response = await fetch('/api/command', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `bot_id=${encodeURIComponent(botId)}&cmd_id=${encodeURIComponent(cmdId)}&program=${encodeURIComponent(program)}&params=${encodeURIComponent(params)}&delay=${encodeURIComponent(delay)}&expected_code=${encodeURIComponent(expectedCode)}`,
        });

        if (response.ok) {
            alert('Command sent successfully!');
        } else {
            const error = await response.json();
            alert(`Error: ${error.message}`);
        }
    } catch (err) {
        alert(`Failed to send command: ${err.message}`);
    }

    // Close the modal
    modal.style.display = 'none';
});
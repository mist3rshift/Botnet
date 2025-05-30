document.addEventListener('DOMContentLoaded', () => {
    const sysinfoBtn = document.getElementById('sysinfo-btn');

    if (!sysinfoBtn) {
        console.error('Button not found.');
        return;
    }

    sysinfoBtn.addEventListener('click', async () => {
        // Get the bot ID from the page (adjust selector as needed)
        const botId = document.getElementById('client-id').value; // Or wherever you store the bot ID

        if (!botId) {
            showNotification('Bot ID is missing.', 'error');
            return;
        }

        try {
            const response = await fetch('/api/sysinfo', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `bot_ids=${encodeURIComponent(botId)}`,
            });

            if (response.ok) {
                showNotification('Sysinfo request sent to client.', 'success');
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.error}`, 'error');
            }
        } catch (error) {
            console.error('Error sending sysinfo request:', error);
            showNotification('Failed to send sysinfo request.', 'error');
        }
    });
});

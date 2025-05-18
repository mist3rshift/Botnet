document.addEventListener('DOMContentLoaded', () => {
    const updateBtn = document.getElementById('update-client-btn');
    if (!updateBtn) return;

    updateBtn.addEventListener('click', async () => {
        // Get the bot ID from the page (adjust selector as needed)
        const botId = document.getElementById('client-id').value; // Or wherever you store the bot ID

        if (!botId) {
            showNotification('Bot ID is missing.', 'error');
            return;
        }

        try {
            const response = await fetch('/api/update', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `bot_ids=${encodeURIComponent(botId)}`,
            });

            if (response.ok) {
                showNotification('Update request sent to client.', 'success');
            } else {
                const error = await response.json();
                showNotification(`Error: ${error.error}`, 'error');
            }
        } catch (error) {
            console.error('Error sending update request:', error);
            showNotification('Failed to send update request.', 'error');
        }
    });
});
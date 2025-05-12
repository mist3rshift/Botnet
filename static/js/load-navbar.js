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

// Dynamically load the navbar
async function loadNavbar() {
    const response = await fetch('/static/html/partial.navbar.html');
    if (response.ok) {
        const navbarHtml = await response.text();
        document.getElementById('navbar').innerHTML = navbarHtml;
    } else {
        // Log the error to the console for debugging
        console.error('Failed to load navbar:', response.status);

        // Display a notification to the user
        showNotification('Failed to load the navbar. Please try refreshing the page.', 'error');
    }
}

// Call the function to load the navbar
window.onload = async () => {
    await loadNotificationSystem(); // Load the notification system
    loadNavbar();
}
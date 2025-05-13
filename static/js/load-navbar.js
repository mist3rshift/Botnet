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
    try {
        const response = await fetch('/static/html/partial.navbar.html');
        if (!response.ok) {
            console.error('Failed to load navbar:', response.status, response.statusText);
            showNotification('Failed to load the navbar. Please try refreshing the page.', 'error');
            return;
        }

        const navbarHtml = await response.text();
        console.log('Loaded navbar HTML:', navbarHtml);

        const navbarElement = document.getElementById('navbar');
        if (navbarElement) {
            navbarElement.innerHTML = navbarHtml;
        } else {
            console.error('Navbar container not found in the DOM.');
        }
    } catch (error) {
        console.error('Error loading navbar:', error);
        showNotification('Failed to load the navbar. Please try refreshing the page.', 'error');
    }
}

// Load the navbar and notification system when the DOM is fully loaded
document.addEventListener("DOMContentLoaded", async () => {
    await loadNotificationSystem();
    await loadNavbar();
});
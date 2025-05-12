// Fetch bots and populate the table
async function fetchBots() {
    try {
        const response = await fetch('/api/bots');
        if (!response.ok) {
            console.error('Failed to fetch bots:', response.status, response.statusText);
            return;
        }

        const bots = await response.json();
        console.log('Fetched bots:', bots);

        const tbody = document.querySelector('#bots-table tbody');
        if (!tbody) {
            console.error('Bots table body not found in the DOM.');
            return;
        }

        tbody.innerHTML = ''; // Clear existing rows

        bots.forEach(bot => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${bot.socket}</td>
                <td><a href="/static/html/bot.html?id=${bot.id}">${bot.id}</a></td>
                <td>${bot.status}</td>
            `;
            tbody.appendChild(row);
        });
    } catch (error) {
        console.error('Error fetching bots:', error);
    }
}

// Fetch bots when the DOM is fully loaded
document.addEventListener("DOMContentLoaded", fetchBots);
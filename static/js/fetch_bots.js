// Fetch bots and populate the table
async function fetchBots() {
    const response = await fetch('/api/bots');
    const bots = await response.json();
    const tbody = document.querySelector('#bots-table tbody');
    tbody.innerHTML = ''; // Clear existing rows

    bots.forEach(bot => {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${bot.socket}</td>
            <td>${bot.id}</td>
            <td>${bot.status}</td>
        `;
        tbody.appendChild(row);
    });
}

// Load bots on page load
window.onload = fetchBots;
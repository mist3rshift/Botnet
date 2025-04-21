let timerInterval;
let countdown = 30; // Refresh timer, seconds
let offset = 0; // "Load More" offset
let refreshOffset = 0; // Refresh only most recent lines
const linesPerPage = 50; // Number of lines to fetch per request

function formatLogsWithLineNumbers(logs, startingLineNumber = 1) {
    // Add line numbers to file!
    const lines = logs.split('\n');
    const formattedLines = lines.map((line, index) => {
        if (line.trim() === '') return ''; // Skip empty lines
        const lineNumber = startingLineNumber + index;
        return `<div class="log-line">
                <span class="line-number">${lineNumber}</span>
                <span class="line-content">${line}</span>
            </div>`;
    });
    return formattedLines.join('');
}

// Function to fetch bot logs
async function fetchBotLogs(botId, prepend = false, isRefresh = false) {
    const fileContentElement = document.getElementById('file-content');

    try {
        // Use the appropriate offset for refresh or load more
        const currentOffset = isRefresh ? refreshOffset : offset;
        const response = await fetch(`/api/botfile?id=${botId}&lines=${linesPerPage}&offset=${currentOffset}`);
        if (response.ok) {
            const data = await response.text();

            if (prepend) {
                // Calculate the starting line number for prepended lines
                const startingLineNumber = offset - linesPerPage + 1;
                // Prepend new lines to the top
                fileContentElement.innerHTML =
                    formatLogsWithLineNumbers(data, startingLineNumber) + fileContentElement.innerHTML;
            } else if (isRefresh) {
                // Replace only the most recent lines during refresh
                const preservedContent = Array.from(fileContentElement.querySelectorAll('.log-line'))
                    .slice(0, -linesPerPage)
                    .map(line => line.outerHTML)
                    .join('');
                const existingLines = fileContentElement.querySelectorAll('.log-line').length;
                fileContentElement.innerHTML =
                    preservedContent + formatLogsWithLineNumbers(data, existingLines - linesPerPage + 1);
            } else {
                // Replace content for initial load
                fileContentElement.innerHTML = formatLogsWithLineNumbers(data, 1);
            }
        } else {
            const error = await response.json();
            fileContentElement.textContent = `Error: ${error.error}`;
        }
    } catch (err) {
        fileContentElement.textContent = `Error: Failed to fetch bot logs. ${err.message}`;
    }
}

// Function to start the timer
function startTimer(botId) {
    const timerElement = document.getElementById('timer');
    countdown = 30; // Reset countdown

    clearInterval(timerInterval); // Clear any existing timer
    timerInterval = setInterval(() => {
        countdown--;
        timerElement.textContent = countdown;

        if (countdown <= 0) {
            clearInterval(timerInterval);
            fetchBotLogs(botId, false, true); // Refresh the most recent lines
            startTimer(botId); // Restart the timer
        }
    }, 1000);
}

// Function to get query parameters from the URL
function getQueryParam(param) {
    const urlParams = new URLSearchParams(window.location.search);
    return urlParams.get(param);
}

// Function to set the bot ID in the hidden input field
function setBotId(botId) {
    const clientIdInput = document.getElementById('client-id'); // Hidden input for bot_id
    if (clientIdInput) {
        clientIdInput.value = botId;
    }
}

// On page load, fetch the last `linesPerPage` lines and start the timer
window.onload = () => {
    const botId = getQueryParam('id');
    if (botId) {
        setBotId(botId); // Populate the hidden input field with the bot ID
        offset = 0; // Reset offset for "Load More"
        refreshOffset = 0; // Reset offset for refresh
        fetchBotLogs(botId); // Fetch the logs for the specified bot ID
        startTimer(botId); // Start the timer for regular updates
    } else {
        document.getElementById('file-content').textContent = 'Error: No bot ID provided in the URL.';
    }
};

// Event listener for the "Load Client" button
document.getElementById('load-client').addEventListener('click', () => {
    const clientId = document.getElementById('client-id').value.trim();
    if (clientId) {
        setBotId(clientId); // Update the hidden input field with the new bot ID
        offset = 0; // New line n° offset
        refreshOffset = 0; // New line n° offset
        fetchBotLogs(clientId); // Get file data
        startTimer(clientId); // Auto-update file content
    } else {
        alert('Please enter a valid Client ID.');
    }
});

// Event listener for the "Refresh Now" button
document.getElementById('force-refresh').addEventListener('click', () => {
    forcerefresh();
});

function forcerefresh() {
    const botId = getQueryParam('id') || document.getElementById('client-id').value.trim();
    if (botId) {
        setBotId(botId); // Ensure the hidden input field is updated
        fetchBotLogs(botId, false, true); // Refresh the most recent lines
        countdown = 30; // Reset the countdown
    } else {
        alert('Please enter a valid Bot ID.');
    }
}

// Event listener for the "Load More" button
document.getElementById('load-more').addEventListener('click', async () => {
    const botId = getQueryParam('id') || document.getElementById('client-id').value.trim();
    if (botId) {
        setBotId(botId); // Ensure the hidden input field is updated
        offset += linesPerPage; // Increase the offset for "Load More"
        fetchBotLogs(botId, true); // Fetch more lines and prepend
        await new Promise(r => setTimeout(r, 1)); // For some reason, we need to wait for just a little (1ms)
        forcerefresh(); // Allows to reset the line numbers :)
    } else {
        alert('Please enter a valid Bot ID.');
    }
});
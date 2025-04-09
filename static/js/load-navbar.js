// Dynamically load the navbar
async function loadNavbar() {
    const response = await fetch('/static/html/navbar.html');
    if (response.ok) {
        const navbarHtml = await response.text();
        document.getElementById('navbar').innerHTML = navbarHtml;
    } else {
        console.error('Failed to load navbar:', response.status);
    }
}

// Call the function to load the navbar
loadNavbar();
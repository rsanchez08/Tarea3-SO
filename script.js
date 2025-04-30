document.getElementById('myForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const message = document.getElementById('mensaje').value;
    
    const response = await fetch('/api/post', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({ mensaje: message })
    });
    
    alert(await response.text());
});
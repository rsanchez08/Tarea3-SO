document.querySelector('form').addEventListener('submit', async (e) => {
    e.preventDefault();
    const response = await fetch('/api/post', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({message: e.target.mensaje.value})
    });
    alert(await response.text());
});
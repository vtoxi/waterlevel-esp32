let autoScroll = true;
const logsDiv = document.getElementById('logs');
const autoScrollBtn = document.getElementById('autoScrollBtn');
const eventSource = new EventSource('/logs/stream');

function toggleAutoScroll() {
    autoScroll = !autoScroll;
    autoScrollBtn.textContent = 'Auto-scroll: ' + (autoScroll ? 'ON' : 'OFF');
}

function clearLogs() {
    if (confirm('Are you sure you want to clear all logs?')) {
        fetch('/logs/clear', { method: 'POST' })
            .then(response => {
                if (response.ok) {
                    logsDiv.innerHTML = '';
                }
            });
    }
}

function viewFullLogFile() {
    const pre = document.getElementById('full-log-file');
    if (pre.style.display === 'block') { pre.style.display = 'none'; return; }
    fetch('/logs/file').then(r => r.text()).then(txt => {
        pre.textContent = txt;
        pre.style.display = 'block';
        pre.scrollTop = pre.scrollHeight;
    });
}

function appendLog(message) {
    const logEntry = document.createElement('div');
    logEntry.className = 'log-entry';
    logEntry.textContent = message;
    logsDiv.appendChild(logEntry);
    if (autoScroll) {
        logsDiv.scrollTop = logsDiv.scrollHeight;
    }
}

eventSource.onmessage = function (event) {
    appendLog(event.data);
};

eventSource.onerror = function (error) {
    console.error('EventSource failed:', error);
    eventSource.close();
    appendLog('Connection lost. Please refresh the page.');
}; 
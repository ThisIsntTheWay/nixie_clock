var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
    console.log('WS: Connection opened.');
    websocket.send('WS client message');
}

function onClose(event) {
    console.log('WS: Connection closed.');
    setTimeout(initWebSocket, 2000);
}

// Message handling
function onMessage(event) {
    var state;
    if (event.data == "1") { state = "ON"; }
    else { state = "OFF"; }

    // Manipulate 
    document.getElementById('state').innerHTML = state;
}

// Start websockets connection on page load
function onLoad(event) {
    initWebSocket();
    initButton();
}

window.addEventListener('load', onLoad);

function toggle(){
    websocket.send('toggle');
}

function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
}


var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);

    // Determine functions to call on specific events
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
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
    let evData = event.data;

    
    
    // Manipulate DOM, but first check if element exists
    if (!!document.getElementById('state'))     document.getElementById('state').innerHTML = state;
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


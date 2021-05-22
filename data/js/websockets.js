var gateway = `ws://${window.location.hostname}/ws`;
var wsQueryInterval = null;
var websocket;

var wsQueryIntervalCycle = 1500;

function initWebSocket() {
    websocket = new WebSocket(gateway);

    // Determine functions to call on specific events
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.info('WS: Connection opened.');
}

function onClose(event) {
    console.warn('WS: Connection closed.');
    setTimeout(initWebSocket, 2000);
}

// Message handling
function onMessage(event) {
    let evData = event.data;
    console.debug("Got ws server message: " + evData);
    
    // Manipulate DOM based on message
    let regex = new RegExp("^\\S+\\s+");
    if (evData.startsWith("SYS_TIME")) document.getElementById('rtc_time').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_MODE")) document.getElementById('rtc_mode').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_NTP")) document.getElementById('rtc_ntp').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_GMT")) document.getElementById('rtc_gmt').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_DST")) document.getElementById('rtc_dst').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("HUE_IP")) document.getElementById('hue_ip').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("HUE_ON_SCHED")) document.getElementById('hue_on_time').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("HUE_OFF_SCHED")) document.getElementById('hue_off_time').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_RSSI")) document.getElementById('wifi_rssi').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("NIXIE_DISPLAY")) document.getElementById('tubes_display').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("NIXIE_DEP_INTERVAL")) document.getElementById('depoison_interval').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("NIXIE_DEP_TIME")) document.getElementById('depoison_schedule').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("NIXIE_DEP_MODE")) document.getElementById('depoison_mode').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("NIXIE_MODE")) document.getElementById('tubes_mode').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_CRYPTO")) document.getElementById('crypto_ticker').innerHTML = evData.replace(regex, '');
    if (evData.startsWith("SYS_MSG")) document.getElementById('sys_msg').innerHTML = evData.replace(regex, '');
}

function wsQuery() {
    //console.debug("wsQuery fired.");
    if (websocket.readyState == 1) {
        // Send specific text to websocket server based on existing DOM
        if (!!document.getElementById('rtc_time')) websocket.send("getTime");
        if (!!document.getElementById('rtc_mode')) websocket.send("getRTCMode");
        if (!!document.getElementById('rtc_ntp')) websocket.send("getNTPsource");
        if (!!document.getElementById('rtc_gmt') && xhr.state == 200) websocket.send("getGMTval");
        if (!!document.getElementById('rtc_dst') && xhr.state == 200) websocket.send("getDSTval");
        if (!!document.getElementById('wifi_rssi')) websocket.send("getWIFIrssi");
        if (!!document.getElementById('tubes_display')) websocket.send("getNixieDisplay");
        if (!!document.getElementById('tubes_mode')) websocket.send("getNixieMode");
        if (!!document.getElementById('crypto_ticker')) websocket.send("getCryptoTicker");
        if (!!document.getElementById('sys_msg')) websocket.send("getSysMsg");
        if (!!document.getElementById('hue_on_time')) websocket.send("getHUEon");
        if (!!document.getElementById('hue_off_time')) websocket.send("getHUEoff");
        if (!!document.getElementById('hue_ip')) websocket.send("getHUEip");
        if (!!document.getElementById('depoison_interval')) websocket.send("getDepoisonInt");
        if (!!document.getElementById('depoison_schedule')) websocket.send("getDepoisonTime");
        if (!!document.getElementById('depoison_mode')) websocket.send("getDepoisonMode");
    } else {
        console.warn("Cannot send msg to WS endpoint. Connection is in state " + websocket.readyState + ".");
    }
}

// Start websockets connection on page load
window.addEventListener('load', onLoad);
function onLoad(event) {
    initWebSocket();

    if (wsQueryInterval) clearInterval(wsQueryInterval);
    wsQueryInterval = setInterval(() => {wsQuery()}, wsQueryIntervalCycle);
}
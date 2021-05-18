var spinnerIntervalSet = null;
var spinnerCount = 0;

function spinner() {
    // Destroy timer if it exists already
    if (spinnerIntervalSet) clearInterval(spinnerIntervalSet); spinnerCount = 0;

    //&#x23F1; -> ⏱
    //&#x1f449; -> 👉
    spinnerFrames = ['&#x1f449;&#x23F1;', '&#x1f449; &#x23F1;', '&#x1f449;  &#x23F1;', '&#x1f449; &#x23F1;', '&#x1f449;&#x23F1;'];
    currFrame = 0;

    // Iterate through spinner
    function nextFrame() {
        if (xhr.readyState != 4 && xhr.status != 400) {
            spinnerCount++;

            if (spinnerCount < 50) {
                responseText.innerHTML = spinnerFrames[currFrame];
                currFrame = (currFrame == spinnerFrames.length - 1) ? 0 : currFrame + 1;
            } else {
                responseText.innerHTML = "&#x274C;&#x23F1;"
            }
        } else {
            if (spinnerIntervalSet) clearInterval(spinnerIntervalSet);
        }
    }

    spinnerIntervalSet = setInterval(nextFrame, 250);
}

function tableFromJson(jsonTable, mode) {
    data = JSON.parse(jsonTable);
    var table = "<table>";

    if (mode == 0) {
        table += 
            `<tr>
                <th>SSID</th>
                <th>MAC</th>
                <th>Channel</th>
                <th>RSSI</th>
                <th>Security</th>
            </tr>`;

        for (var key in data) {
            console.log("Key" + key);
            table += `
                <tr>
                    <td>${data[key]["ssid"]}</td>
                    <td>${data[key]["mac"]}</td>
                    <td>${data[key]["channel"]}</td>
                    <td>${data[key]["rssi"]}</td>
                    <td>${data[key]["security"]}</td>
                </tr>`;
        }
        table += "</table>";
    } else if (mode == 1) {
        for (var key in data) {
            table += 
            `<tr>
                <td>${key}</td>
                <td>${data[key]}</td>
            </tr>`;
        }
        table += "</table>";
    }

    // Now, add the newly created table with json data, to a container.
    responseText.innerHTML = table;
}
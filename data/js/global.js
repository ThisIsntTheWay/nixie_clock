var spinnerIntervalSet = null;

function spinner() {
    // Destroy timer if it exists already
    if (spinnerIntervalSet) clearInterval(spinnerIntervalSet);

    //&#x23F1; -> ⏱
    //&#x1f449; -> 👉
    spinnerFrames = ['&#x1f449;&#x23F1;', '&#x1f449; &#x23F1;', '&#x1f449;  &#x23F1;', '&#x1f449; &#x23F1;', '&#x1f449;&#x23F1;'];
    currFrame = 0;

    // Iterate through spinner
    function nextFrame() {
        if (xhr.readyState != 4 && xhr.status != 400) {
            responseText.innerHTML = spinnerFrames[currFrame];
            currFrame = (currFrame == spinnerFrames.length - 1) ? 0 : currFrame + 1;
        } else {
            if (spinnerIntervalSet) clearInterval(spinnerIntervalSet);
        }
    }

    spinnerIntervalSet = setInterval(nextFrame, 250);
}

function tableFromJson(jsonTable) {
    data = JSON.parse(jsonTable);
    var table = "<table>";

    for (let key in data) {
        table += `<tr><td>${key}</td><td>${data[key]}</td></tr>`;
    }
    table += "</table>";

    // Now, add the newly created table with json data, to a container.
    responseText.innerHTML = table;
}
var spinnerIntervalSet = null;

function spinner() {
    // Destroy timer if it exists already
    if (spinnerIntervalSet) clearInterval(spinnerIntervalSet);

    spinnerFrames = ['.', 'o', 'O', 'o', '.'];
    currFrame = 0;

    // Iterate through spinner
    function nextFrame() {
        if (xhr.readyState != 4) {
            responseText.innerHTML = spinnerFrames[currFrame];
            currFrame = (currFrame == spinnerFrames.length - 1) ? 0 : currFrame + 1;
        }
    }

    spinnerIntervalSet = setInterval(nextFrame, 100);
}

function tableFromJson(jsonIngress) {
    // Extract value from table header. 
    var col = [];
    for (var i = 0; i < jsonIngress.length; i++) {
        for (var key in jsonIngress[i]) {
            if (col.indexOf(key) === -1) {
                col.push(key);
            }
        }
    }

    // Create a table.
    var table = document.createElement("table");

    // Create table header row using the extracted headers above.
    var tr = table.insertRow(-1);                   // table row.

    for (var i = 0; i < col.length; i++) {
        var th = document.createElement("th");      // table header.
        th.innerHTML = col[i];
        tr.appendChild(th);
    }

    // add json data to the table as rows.
    for (var i = 0; i < jsonIngress.length; i++) {

        tr = table.insertRow(-1);

        for (var j = 0; j < col.length; j++) {
            var tabCell = tr.insertCell(-1);
            tabCell.innerHTML = jsonIngress[i][col[j]];
        }
    }

    // Now, add the newly created table with json data, to a container.
    responseText.innerHTML = "";
    responseText.appendChild(table);
}
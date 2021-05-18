function spinner() {
    setTimeout(() => {if (xhr.readyState != 4) responseText.innerHTML = ".";}, 250);
    setTimeout(() => {if (xhr.readyState != 4) responseText.innerHTML = "o";}, 500);
    setTimeout(() => {if (xhr.readyState != 4) responseText.innerHTML = "O";}, 750);
    setTimeout(() => {if (xhr.readyState != 4) responseText.innerHTML = "o";}, 1000);
    setTimeout(() => {if (xhr.readyState != 4) responseText.innerHTML = ".";}, 1250);
}
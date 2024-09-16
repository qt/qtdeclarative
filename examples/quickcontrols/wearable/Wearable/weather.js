// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


function formatTemp(farenheit) {
    let celsius = Math.round((farenheit -32) * 5/9)
    return celsius + "°C"
}

function requestWeatherData(cntr) {
    var xhr = new XMLHttpRequest;
    xhr.open("GET", "weather.json");
    xhr.onreadystatechange = function () {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            cntr.weatherData = JSON.parse(xhr.responseText)
        }
    }
    xhr.send();
}

function getTimeHMS(utcTime) {
    var date = new Date(utcTime * 1000);
    // Hours part from the timestamp
    var hours = "0" + date.getHours();
    // Minutes part from the timestamp
    var minutes = "0" + date.getMinutes();

    // Will display time in 10:30:23 format
    return hours.substr(-2) + ':' + minutes.substr(-2);
}

function iconSelect(code) {
    if (code === "01d" || code === "01n")
        return "sunny";
    else if (code === "02d" || code === "02n")
        return "sunny-very-few-clouds";
    else if (code === "03d" || code === "03n")
        return "few-clouds";
    else if (code === "04d" || code === "04n")
        return "overcast";
    else if (code === "09d" || code === "09n" || code === "10d" || code === "10n")
        return "showers";
    else if (code === "11d" || code === "11n")
        return "thundershower";
    else if (code === "13d" || code === "13n")
        return "snow";
    else if (code === "50d" || code === "50n")
        return "fog";

    return "sunny"; // default choice
}

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    var hours = date.getHours();
    var ampm = Math.floor((hours / 12)) ? " PM" : " AM";
    hours = (hours % 12);

    // Minutes part from the timestamp
    var minutes = "0" + date.getMinutes();
    // Seconds part from the timestamp
    var seconds = "0" + date.getSeconds();

    // Will display time in 10:30:23 format
    return hours % 12 + ':' + minutes.substr(-2) + ':' + seconds.substr(-2)
            + ampm;
}

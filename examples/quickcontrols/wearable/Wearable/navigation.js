// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

function requestNavigationRoute(rModel) {
    var xhr = new XMLHttpRequest;
    xhr.open("GET", "fallbackroute.json");
    xhr.onreadystatechange = function () {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            var a = JSON.parse(xhr.responseText);
            var steps = a.route.steps;

            for (var step in steps) {
                var direction = steps[step].direction;
                var instructionText = steps[step].instructionText;
                var distanceToNextInstruction = steps[step].distanceToNextInstruction;

                rModel.append({
                    icon: formatIconInstruction(direction),
                    shortInfo: formatShortInstruction(direction),
                    instruction: instructionText,
                    distance: qsTr("in") + " " + formatDistance(distanceToNextInstruction)
                });
            }
        }
    }
    xhr.send();
}

function formatIconInstruction(direction)
{
    switch (direction){
    case 0:
        return "";
    case 1:
        return "forward";
    case 2:
        return "bearright";
    case 3:
    case 4:
        return "right";
    case 5:
        return "lightright";
    case 6:
        return "uturnright";
    case 7:
        return "uturleft";
    case 8:
    case 9:
        return "left";
    case 10:
        return "lightleft";
    case 11:
        return "bearleft";
    }
}

function formatShortInstruction(direction)
{
    switch (direction){
    case 0:
        return "";
    case 1:
        return "Move forward";
    case 2:
        return "Bear right";
    case 3:
        return "Turn right";
    case 4:
        return "Turn hard right";
    case 5:
        return "Turn slightly right";
    case 6:
        return "Uturn right";
    case 7:
        return "Uturn left";
    case 8:
        return "Turn hard left";
    case 9:
        return "Turn left";
    case 10:
        return "Turn slightly left";
    case 11:
        return "Bear left";
    }
}

function formatTime(sec)
{
    var value = sec
    var seconds = value % 60
    value /= 60
    value = (value > 1) ? Math.round(value) : 0
    var minutes = value % 60
    value /= 60
    value = (value > 1) ? Math.round(value) : 0
    var hours = value
    if (hours > 0) value = hours + "h:"+ minutes + "m"
    else value = minutes + "min"
    return value
}

function formatDistance(meters)
{
    var dist = Math.round(meters)
    if (dist > 1000 ){
        if (dist > 100000){
            dist = Math.round(dist / 1000)
        }
        else{
            dist = Math.round(dist / 100)
            dist = dist / 10
        }
        dist = dist + " km"
    }
    else{
        dist = dist + " m"
    }
    return dist
}

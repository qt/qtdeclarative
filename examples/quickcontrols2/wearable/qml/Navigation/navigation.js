// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

function requestNavigationRoute(rModel) {
    var xhr = new XMLHttpRequest;
    xhr.open("GET", "walk_route.json");
    xhr.onreadystatechange = function () {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            var a = JSON.parse(xhr.responseText);
            var steps = a.routes[0].legs[0].steps;

            for (var step in steps) {
                var maneuver = steps[step].maneuver;
                var duration = steps[step].duration;
                var distance = steps[step].distance;

                rModel.append({
                                      navInstruction: maneuver.instruction,
                                      navImage: getNavigationImage(
                                                    maneuver.type,
                                                    maneuver.modifier,
                                                    routeView.imageList),
                                      navAuxInfo: getAuxInfo(distance,
                                                             duration)
                                  });
            }
        }
    }
    xhr.send();
}

function getNavigationImage(maneuverType, maneuverModifier, imageList) {
    var imageToReturn;
    if (maneuverType === "depart") {
        imageToReturn = imageList[4];
    } else if (maneuverType === "arrive") {
        imageToReturn = imageList[5];
    } else if (maneuverType === "turn") {
        if (maneuverModifier.search("left") >= 0)
            imageToReturn = imageList[1];
        else if (maneuverModifier.search("right") >= 0)
            imageToReturn = imageList[2];
        else
            imageToReturn = imageList[0];
    } else {
        if (maneuverModifier === "uturn") {
            imageToReturn = imageList[3];
        } else {
            imageToReturn = imageList[0];
        }
    }

    return imageToReturn;
}

function getAuxInfo(distInMeters, timeInSecs) {
    var distance = convertDistance(distInMeters);
    if (distance.length > 0)
        return "Distance: " + distance + "\nTime: " + formatSeconds(
                    timeInSecs);
    else
        return "";
}

function convertDistance(meter) {
    var dist = "";
    var feet = (meter * 0.3048).toPrecision(6);
    var miles = (meter * 0.000621371).toPrecision(6);

    if (Math.floor(miles) > 1) {
        dist += Math.floor(miles) + " mi";
        feet = ((miles - Math.floor(miles)) * 0.3048).toPrecision(6);
    }
    if (Math.floor(feet) > 1)
        dist += Math.floor(feet) + " ft";

    return dist
}

function formatSeconds(seconds) {
    var date = new Date(1970, 0, 1);
    date.setSeconds(seconds);
    return date.toTimeString().replace(/.*(\d{2}:\d{2}:\d{2}).*/, "$1");
}

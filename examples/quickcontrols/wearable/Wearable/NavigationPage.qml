// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtLocation
import QtPositioning
import WearableStyle

Item {
    id: navigationPage

    MapView {
        id: view
        anchors.fill: parent

        map.plugin: Plugin {
            name: "osm"
        }
        map.zoomLevel: 16

        RouteModel {
            id: routeModel
            plugin : view.map.plugin
            query:  RouteQuery {
                id: routeQuery
            }
            onStatusChanged: {
                if (status == RouteModel.Ready) {
                    routeInfoModel.populate()
                }
            }
        }

        property real lat: 59.9485
        property real lon: 10.7686

        map.center: QtPositioning.coordinate(lat, lon)

        Behavior on lon { PropertyAnimation { duration: 500; easing.type: Easing.InOutQuad } }
        Behavior on lat { PropertyAnimation { duration: 500; easing.type: Easing.InOutQuad } }


        MapItemView {
            parent: view.map
            model: routeModel
            delegate: MapRoute {
                id: route

                required property var routeData
                route: routeData
                line.color: UIStyle.highlightColor
                line.width: 5
                smooth: true
                opacity: 1
            }
            autoFitViewport: true
        }

        Component.onCompleted: {
            var startCoordinate = QtPositioning.coordinate(59.9485, 10.7686);
            var endCoordinate = QtPositioning.coordinate(59.9645, 10.671);

            routeQuery.clearWaypoints();
            routeQuery.addWaypoint(startCoordinate)
            routeQuery.addWaypoint(endCoordinate)
            routeQuery.travelModes = RouteQuery.CarTravel
            routeQuery.routeOptimizations = RouteQuery.FastestRoute

            routeModel.update();
        }
    }

    ListModel {
        id: routeInfoModel

        function formatIconInstruction(direction)
        {
            switch (direction){
            case RouteManeuver.NoDirection:
                return "";
            case RouteManeuver.DirectionForward:
                return "forward";
            case RouteManeuver.DirectionBearRight:
                return "bearright";
            case RouteManeuver.DirectionRight:
            case RouteManeuver.DirectionHardRight:
                return "right";
            case RouteManeuver.DirectionLightRight:
                return "lightright";
            case RouteManeuver.DirectionUTurnRight:
                return "uturnright";
            case RouteManeuver.DirectionUTurnLeft:
                return "uturleft";
            case RouteManeuver.DirectionHardLeft:
            case RouteManeuver.DirectionLeft:
                return "left";
            case RouteManeuver.DirectionLightLeft:
                return "lightleft";
            case RouteManeuver.DirectionBearLeft:
                return "bearleft";
            }
        }

        function formatShortInstruction(direction)
        {
            switch (direction){
            case RouteManeuver.NoDirection:
                return "";
            case RouteManeuver.DirectionForward:
                return "Move forward";
            case RouteManeuver.DirectionBearRight:
                return "Bear right";
            case RouteManeuver.DirectionRight:
                return "Turn right";
            case RouteManeuver.DirectionHardRight:
                return "Turn hard right";
            case RouteManeuver.DirectionLightRight:
                return "Turn slightly right";
            case RouteManeuver.DirectionUTurnRight:
                return "Uturn right";
            case RouteManeuver.DirectionUTurnLeft:
                return "Uturn left";
            case RouteManeuver.DirectionHardLeft:
                return "Turn hard left";
            case RouteManeuver.DirectionLeft:
                return "Turn left";
            case RouteManeuver.DirectionLightLeft:
                return "Turn slightly left";
            case RouteManeuver.DirectionBearLeft:
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

        function populate()
        {
            routeInfoModel.clear()
            if (routeModel.count > 0) {
                for (var i = 0; i < routeModel.get(0).segments.length; i++) {
                    routeInfoModel.append({
                        "icon": formatIconInstruction(routeModel.get(0).segments[i].maneuver.direction),
                        "shortInfo": formatShortInstruction(routeModel.get(0).segments[i].maneuver.direction),
                        "instruction": routeModel.get(0).segments[i].maneuver.instructionText,
                        "distance": qsTr("in") + " " + formatDistance(routeModel.get(0).segments[i].maneuver.distanceToNextInstruction)
                    });
                }
            }
        }
    }

    ListView {
        id: routeView

        height: 90
        anchors.bottom: navigationPage.bottom
        anchors.left: navigationPage.left
        anchors.right: navigationPage.right
        anchors.rightMargin: 10
        anchors.leftMargin: 15
        anchors.bottomMargin: 20

        spacing: 12

        clip: true
        focus: true

        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem

        property int centeredIndex: 0

        model: routeInfoModel
        delegate: RouteElement {
            width: routeView.width - 10
            height: routeView.height - 10
        }

        onContentYChanged: {
            centeredIndex = indexAt(10, contentY)
            if (centeredIndex > -1) {
                view.lat = routeModel.get(0).segments[centeredIndex].maneuver.position.latitude
                view.lon = routeModel.get(0).segments[centeredIndex].maneuver.position.longitude
            }
        }
    }

    function incrementPoint() {
        routeView.centeredIndex++
        if (routeView.centeredIndex >= routeInfoModel.count)
            routeView.centeredIndex = 0
        routeView.positionViewAtIndex(routeView.centeredIndex, ListView.Center)
    }
}

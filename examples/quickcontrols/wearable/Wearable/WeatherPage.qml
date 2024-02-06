// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import WearableStyle
import "weather.js" as WeatherData

Item {
    Flickable {
        id: flick
        anchors.fill:parent
        anchors.margins: 15
        anchors.topMargin: 40 + 15
        contentWidth: width
        contentHeight: column.height

        Column {
            id: column
            spacing: 10

            Item {
                width: flick.contentWidth
                height: 100

                Row {
                    id: townName
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    width: childrenRect.width
                    height: childrenRect.height

                    spacing: 10
                    Image {
                        id: sunIcon
                        source: (wDataCntr.weatherData
                                 && wDataCntr.weatherData.weather
                                 && wDataCntr.weatherData.weather[0]
                                 && wDataCntr.weatherData.weather[0].icon) ?
                                UIStyle.iconPath("weather-" + WeatherData.iconSelect(wDataCntr.weatherData.weather[0].icon)) : ""
                        width: 64
                        height: width
                        sourceSize.width: width
                        sourceSize.height: height
                    }
                    Text {
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.name) ?
                              wDataCntr.weatherData.name : ""
                        anchors.verticalCenter: sunIcon.verticalCenter
                        color: UIStyle.textColor
                        font: UIStyle.h1
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: townName.bottom
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.weather
                           && wDataCntr.weatherData.weather[0]
                           && wDataCntr.weatherData.weather[0].main) ?
                          wDataCntr.weatherData.weather[0].main : ""
                    color: UIStyle.textColor
                    font: UIStyle.h3
                }
            }

            ListItem {
                width: flick.contentWidth
                height: 92

                Image {
                    id: thermoimage
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 20
                    anchors.topMargin: 17
                    source: UIStyle.iconPath("thermometer")
                }

                Text {
                    anchors.left: thermoimage.right
                    anchors.verticalCenter: thermoimage.verticalCenter
                    anchors.leftMargin: 5
                    text: qsTr("Temperature")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: thermoimage.verticalCenter
                    anchors.rightMargin: 20
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.main
                           && wDataCntr.weatherData.main.temp) ?
                          WeatherData.formatTemp(wDataCntr.weatherData.main.temp) : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }

                Text {
                    id: maxtxt
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 20
                    anchors.bottomMargin: 17
                    text: qsTr("Max")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    anchors.left: maxtxt.right
                    anchors.verticalCenter: maxtxt.verticalCenter
                    anchors.leftMargin: 10
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.main
                           && wDataCntr.weatherData.main.temp_max) ?
                          WeatherData.formatTemp(wDataCntr.weatherData.main.temp_max) : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }

                Text {
                    id: mintxt
                    anchors.horizontalCenter: maxtxt.horizontalCenter
                    anchors.verticalCenter: maxtxt.verticalCenter
                    anchors.horizontalCenterOffset: parent.width / 2
                    text: qsTr("Min")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    anchors.left: mintxt.right
                    anchors.verticalCenter: mintxt.verticalCenter
                    anchors.leftMargin: 10
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.main
                           && wDataCntr.weatherData.main.temp_min) ?
                          WeatherData.formatTemp(wDataCntr.weatherData.main.temp_min) : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }
            }

            ListItem {
                width: flick.contentWidth
                height: 50

                Image {
                    id: sunriseIcon
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 20
                    anchors.topMargin: 17
                    source: UIStyle.iconPath("sunrise")
                }

                Text {
                    id: sunriseText
                    anchors.left: sunriseIcon.right
                    anchors.verticalCenter: sunriseIcon.verticalCenter
                    anchors.leftMargin: 5
                    text: qsTr("Sunrise")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }
                Text {
                    anchors.left: sunriseText.right
                    anchors.verticalCenter: sunriseText.verticalCenter
                    anchors.leftMargin: 15
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.sys
                           && wDataCntr.weatherData.sys.sunrise) ?
                         WeatherData.getTimeHMS(wDataCntr.weatherData.sys.sunrise) : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }

                Image {
                    id: sunsetIcon
                    anchors.right: sunsetText.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 5
                    source: UIStyle.iconPath("sunset")
                }

                Text {
                    id: sunsetText
                    anchors.right: sunsetValue.left
                    anchors.verticalCenter: sunsetIcon.verticalCenter
                    anchors.rightMargin: 15
                    text: qsTr("Sunset")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    id: sunsetValue
                    anchors.right: parent.right
                    anchors.verticalCenter: sunsetText.verticalCenter
                    anchors.rightMargin: 20
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.sys
                           && wDataCntr.weatherData.sys.sunset) ?
                          WeatherData.getTimeHMS(wDataCntr.weatherData.sys.sunset) : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }
            }

            ListItem {
                width: flick.contentWidth
                height: 50

                Image {
                    id: windIcon
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 20
                    anchors.topMargin: 17
                    source: UIStyle.iconPath("wind")
                }

                Text {
                    id: windText
                    anchors.left: windIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 5
                    text: qsTr("Wind")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 20
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.wind
                           && wDataCntr.weatherData.wind.speed) ?
                          Math.round(wDataCntr.weatherData.wind.speed * 1.61) + " km/h" : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }
            }

            ListItem {
                width: flick.contentWidth
                height: 50

                Image {
                    id: humidityIcon
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 20
                    anchors.topMargin: 17
                    source: UIStyle.iconPath("drop")
                }

                Text {
                    id: humidityText
                    anchors.left: humidityIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 5
                    text: qsTr("Humidity")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 20
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.main
                           && wDataCntr.weatherData.main.humidity) ?
                         wDataCntr.weatherData.main.humidity + " %" : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }
            }

            ListItem {
                width: flick.contentWidth
                height: 50

                Image {
                    id: pressureIcon
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 20
                    anchors.topMargin: 17
                    source: UIStyle.iconPath("pressure")
                }

                Text {
                    id: pressureText
                    anchors.left: pressureIcon.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 5
                    text: qsTr("HPA")
                    font: UIStyle.h4
                    color: UIStyle.textColor
                }

                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 20
                    text: (wDataCntr.weatherData
                           && wDataCntr.weatherData.main
                           && wDataCntr.weatherData.main.pressure) ?
                          Math.round(wDataCntr.weatherData.main.pressure) + " hpa" : ""
                    font: UIStyle.h3
                    color: UIStyle.textColor
                }
            }
        }
    }

    QtObject {
        id: wDataCntr
        property var weatherData
    }
    Component.onCompleted: {
        WeatherData.requestWeatherData(wDataCntr)
    }
}

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import Wearable
import WearableStyle
import "weather.js" as WeatherData

Item {
    QQC2.SwipeView {
        id: svWeatherContainer

        anchors.fill: parent

        SwipeViewPage {
            id: weatherPage1

            Row {
                anchors.centerIn: parent
                spacing: 2

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: UIStyle.themeImagePath("weather-temperature")
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 40

                    Text {
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.temp) ?
                                  qsTr("Avg: ")
                                  + String(wDataCntr.weatherData.main.temp)
                                  + " °F" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }

                    Text {
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.temp_min) ?
                                  qsTr("Min: ")
                                  + String(wDataCntr.weatherData.main.temp_min)
                                  + " °F" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }

                    Text {
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.temp_max) ?
                                  qsTr("Max: ")
                                  + String(wDataCntr.weatherData.main.temp_max)
                                  + " °F" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }
                }
            }
        }

        SwipeViewPage {
            id: weatherPage2

            Column {
                spacing: 40
                anchors.centerIn: parent

                Row {
                    spacing: 20
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        id: wImg
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("weather-wind")
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.wind
                               && wDataCntr.weatherData.wind.speed) ?
                                  String(wDataCntr.weatherData.wind.speed)
                                  + " mph" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }
                }

                Row {
                    spacing: 20
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        id: hImg
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("weather-humidity")
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.humidity) ?
                                  String(wDataCntr.weatherData.main.humidity)
                                  + " %" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }
                }
            }
        }

        SwipeViewPage {
            id: weatherPage3

            Row {
                anchors.centerIn: parent
                spacing: 10

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: UIStyle.themeImagePath("weather-pressure")
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 40

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.pressure) ?
                                  String(wDataCntr.weatherData.main.pressure)
                                  + " hPa" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.sea_level) ?
                                  String(wDataCntr.weatherData.main.sea_level)
                                  + " hPa" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.main
                               && wDataCntr.weatherData.main.grnd_level) ?
                                  String(wDataCntr.weatherData.main.grnd_level)
                                  + " hPa" : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }
                }
            }
        }

        SwipeViewPage {
            id: weatherPage4

            Column {
                spacing: 40
                anchors.centerIn: parent

                Row {
                    spacing: 30
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("weather-sunrise")
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.sys
                               && wDataCntr.weatherData.sys.sunrise) ?
                                  WeatherData.getTimeHMS(wDataCntr.weatherData.sys.sunrise)
                                : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }
                }

                Row {
                    spacing: 30
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("weather-sunset")
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (wDataCntr.weatherData
                               && wDataCntr.weatherData.sys
                               && wDataCntr.weatherData.sys.sunset) ?
                                  WeatherData.getTimeHMS(wDataCntr.weatherData.sys.sunset)
                                : "N/A"
                        font.pixelSize: UIStyle.fontSizeM
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray1
                    }
                }
            }
        }
    }

    QtObject {
        id: wDataCntr
        property var weatherData
    }

    QQC2.PageIndicator {
        count: svWeatherContainer.count
        currentIndex: svWeatherContainer.currentIndex

        anchors.bottom: svWeatherContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Component.onCompleted: {
        WeatherData.requestWeatherData(wDataCntr)
    }
}

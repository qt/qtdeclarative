// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page
    enabled: !GalleryConfig.disabled

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: qsTr("MonthGrid presents a calendar month as a grid of days, "
                     + "calculated for a specific month, year, and locale.")
        }

        ColumnLayout {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Button {
                    implicitWidth: height
                    enabled: !GalleryConfig.disabled
                    flat: true
                    text: qsTr("<")
                    onClicked: {
                        const new_month = monthGrid.month - 1
                        if (new_month < 0) {
                            monthGrid.month = 11
                            --monthGrid.year
                        } else {
                            monthGrid.month = new_month
                        }
                    }
                }
                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("%1 %2").arg(monthGrid.locale.monthName(monthGrid.month))
                                           .arg(monthGrid.year)
                    }
                }
                Button {
                    implicitWidth: height
                    enabled: !GalleryConfig.disabled
                    flat: true
                    text: qsTr(">")
                    onClicked: {
                        const new_month = monthGrid.month + 1
                        if (new_month >= 12) {
                            monthGrid.month = 0
                            ++monthGrid.year
                        } else {
                            monthGrid.month = new_month
                        }
                    }
                }
            }

            GridLayout {
                columns: 2
                Layout.fillWidth: true
                Layout.fillHeight: true

                DayOfWeekRow {
                    locale: monthGrid.locale
                    Layout.fillWidth: true
                    Layout.column: 1
                    Accessible.name: qsTr("Week days")
                }

                WeekNumberColumn {
                    locale: monthGrid.locale
                    year: monthGrid.year
                    month: monthGrid.month
                    Layout.fillHeight: true
                    Accessible.name: qsTr("Week numbers")
                }

                MonthGrid {
                    id: monthGrid
                    locale: Qt.locale("en_US")
                    year: currentDate.getFullYear()
                    month: currentDate.getMonth()
                    readonly property date currentDate: new Date()
                    Layout.fillWidth: true
                    Accessible.name: qsTr("A grid displaying all the days in a month")
                }
            }
        }
    }
}

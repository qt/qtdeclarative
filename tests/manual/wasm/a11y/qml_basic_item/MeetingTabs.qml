
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    enum Types {
        Invitees,
        Scheduler,
        Summary
    }
    property alias setTime: invitees.dateAndTime
    TabBar {
        id: meetingTabs
        width: parent.width
        TabButton {
            text: qsTr("Meeting Invitees")
            Accessible.role: Accessible.PageTab
            Accessible.name: text
            Accessible.description: "Tab to add meeting invitees"
        }
        TabButton {
            text: qsTr("Meeting Scheduler")
            Accessible.role: Accessible.PageTab
            Accessible.name: text
            Accessible.description: "Tab to add a schedule"
        }
        TabButton {
            text: qsTr("Summary")
            Accessible.role: Accessible.PageTab
            Accessible.name: text
            Accessible.description: "Tab to add meeting summary"
        }

        Accessible.role: Accessible.PageTabList
        Accessible.name: "Meetings Tab Bar"
        Accessible.description: "A Tab list of tabs to setup a meeting"
    }

    StackLayout {
        width: 550
        currentIndex: meetingTabs.currentIndex
        anchors {
            left: parent.left
            leftMargin: 10
            top: meetingTabs.bottom
            topMargin: 20
        }

        MeetingInviteesPage {
            id: invitees
            nextButton.Accessible.onPressAction: {
                meetingTabs.currentIndex = MeetingTabs.Types.Scheduler
            }
            nextButton.onReleased: {
                meetingTabs.currentIndex = MeetingTabs.Types.Scheduler
            }
        }

        MeetingSchedulerPage {
            id: scheduler
            nextButton.Accessible.onPressAction: {
                meetingTabs.currentIndex = MeetingTabs.Types.Summary
            }
            nextButton.onReleased: {
                meetingTabs.currentIndex = MeetingTabs.Types.Summary
            }
        }

        MeetingSummary {
            id: activityTab
            meetingOccurrence: scheduler.meetingOccurrence
            onlineOfflineStatus: scheduler.onlineOfflineStatus
            roomNumber: scheduler.roomNumber
            calendarWeek: scheduler.calendarWeek
            meetingDescription: scheduler.meetingDescription
        }
    }
}

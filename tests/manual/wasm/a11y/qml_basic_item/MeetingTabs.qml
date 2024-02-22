// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    width: parent.width -10
    height:parent.height -10
    property color itemColor: "lightgrey"
        Rectangle {
            width: parent.width
            height: parent.height
            color: itemColor
        }
    enum Types {
        Invitees,
        Scheduler,
        Summary
    }
    property alias setTime: invitees.dateAndTime
    property alias textSummary: scheduler.description
    property alias currentIndex: meetingTabs.currentIndex
    TabBar {
        id: meetingTabs
        width: parent.width
        TabButton {
            text: qsTr("Meeting Invitees")
            width:meetingTabs.width/3
            Accessible.role: Accessible.PageTab
            Accessible.name: text
            Accessible.description: "Tab to add meeting invitees"
        }
        TabButton {
            text: qsTr("Meeting Scheduler")
            width:meetingTabs.width/3
            Accessible.role: Accessible.PageTab
            Accessible.name: text
            Accessible.description: "Tab to add a schedule"
        }
        TabButton {
            text: qsTr("Summary")
            width:meetingTabs.width/3
            Accessible.role: Accessible.PageTab
            Accessible.name: text
            Accessible.description: "Tab to add meeting summary"
        }

        Accessible.role: Accessible.PageTabList
        Accessible.name: "Meetings Tab Bar"
        Accessible.description: "A Tab list of tabs to setup a meeting"
    }

    StackLayout {
        width: parent.width - 20
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
            inviteesNameEmail: invitees.inviteesNameEmail
        }
    }
}

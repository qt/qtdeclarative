// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
Item {

     enum Types {Invitees, Scheduler, Summary}

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
        width: 500
        currentIndex: meetingTabs.currentIndex
        anchors {
            left: parent.left
            leftMargin: 10
            top: meetingTabs.bottom
            topMargin: 20
        }

        MeetingInviteesPage {
            id: invitees
            width: 500
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
        }
    }
}

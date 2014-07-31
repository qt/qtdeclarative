/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQml.StateMachine 1.0
import QtTest 1.0

TestCase {
    StateMachine {
        id: myStateMachine
        childMode: StateBase.ParallelStates
        StateBase {
            id: childState1
            childMode: StateBase.ParallelStates
            StateBase {
                id: childState11
            }
            StateBase {
                id: childState12
            }
        }
        StateBase {
            id: childState2
            initialState: childState21
            StateBase {
                id: childState21
            }
            StateBase {
                id: childState22
            }
        }
    }
    name: "nestedParallelMachineStates"

    function test_nestedInitalStates() {
        // uncomment me after vm problems are fixed.
        //            compare(myStateMachine.running, false);
        compare(childState1.active, false);
        compare(childState11.active, false);
        compare(childState12.active, false);
        compare(childState2.active, false);
        compare(childState21.active, false);
        compare(childState22.active, false);
        myStateMachine.start();
        tryCompare(myStateMachine, "running", true);
        tryCompare(childState1, "active", true);
        tryCompare(childState11, "active", true);
        tryCompare(childState12, "active", true);
        tryCompare(childState2, "active", true);
        tryCompare(childState21, "active", true);
        tryCompare(childState22, "active", false);
    }
}

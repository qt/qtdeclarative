// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

Item {
    id: root

    signal basicSignal()
    signal intSignal(value: int)
    signal boolSignal(value: bool)
    signal doubleSignal(value: double)
    signal stringSignal(value: string)

    readonly property int signalWaitTime: 1000

    TestActivityCommunicator {
        id: communicator
    }

    SignalSpy {
        id: basicSpy
        target: communicator
        signalName: "basicSignal"
    }
    SignalSpy {
        id: intSpy
        target: communicator
        signalName: "intSignal"
    }
    SignalSpy {
        id: boolSpy
        target: communicator
        signalName: "boolSignal"
    }
    SignalSpy {
        id: doubleSpy
        target: communicator
        signalName: "doubleSignal"
    }
    SignalSpy {
        id: stringSpy
        target: communicator
        signalName: "stringSignal"
    }

    function wipeSpies() {
        basicSpy.clear()
        intSpy.clear()
        boolSpy.clear()
        doubleSpy.clear()
        stringSpy.clear()
    }

    TestCase {
        name: "basicSignaling"
        when: windowShown

        function init() { root.wipeSpies() }

        function test_basic() {
            compare(basicSpy.count, 0)
            root.basicSignal()
            basicSpy.wait(root.signalWaitTime)
            compare(basicSpy.count, 1)
        }

        function test_int() {
            let testVal = 123123123
            compare(intSpy.count, 0)

            root.intSignal(testVal)
            intSpy.wait(root.signalWaitTime)

            compare(intSpy.count, 1)
            compare(intSpy.signalArguments.length, 1)
            compare(intSpy.signalArguments[0][0], testVal)
        }

        function test_bool() {
            let testVal = false
            compare(boolSpy.count, 0)

            root.boolSignal(testVal)
            boolSpy.wait(root.signalWaitTime)

            compare(boolSpy.count, 1)
            compare(boolSpy.signalArguments.length, 1)
            compare(boolSpy.signalArguments[0][0], testVal)
        }

        function test_double() {
            let testVal = 123.123456
            compare(doubleSpy.count, 0)

            root.doubleSignal(testVal)
            doubleSpy.wait(root.signalWaitTime)

            compare(doubleSpy.count, 1)
            compare(doubleSpy.signalArguments.length, 1)
            compare(doubleSpy.signalArguments[0][0], testVal)
        }

        function test_string() {
            let testVal = "Testing testing, is this thing on?"
            compare(stringSpy.count, 0)

            root.stringSignal(testVal)
            stringSpy.wait(root.signalWaitTime)

            compare(stringSpy.count, 1)
            compare(stringSpy.signalArguments.length, 1)
            compare(stringSpy.signalArguments[0][0], testVal)
        }
    }

    TestCase {
        name: "spamSignaling"
        when: windowShown

        readonly property int spam_count: 100000

        function init() { root.wipeSpies() }

        function waitWhileSignalsArrive(spy, expected_signals, timeout = 100) {
            while (spy.count < expected_signals)
                spy.wait(timeout)
        }

        function test_basic() {
            compare(basicSpy.count, 0)
            for (let i = 0; i < spam_count; ++i) {
                root.basicSignal()
            }
            waitWhileSignalsArrive(basicSpy, spam_count)

            compare(basicSpy.count, spam_count)
        }

        function test_int() {
            const testVal = 123
            compare(intSpy.count, 0)
            for (let i = 0; i < spam_count; ++i) {
                root.intSignal(testVal)
            }
            waitWhileSignalsArrive(intSpy, spam_count)

            compare(intSpy.count, spam_count)
            for (const arg of intSpy.signalArguments) {
                compare(arg[0], testVal)
            }
        }

        function test_bool() {
            const testVal = false
            compare(boolSpy.count, 0)
            for (let i = 0; i < spam_count; ++i) {
                root.boolSignal(testVal)
            }
            waitWhileSignalsArrive(boolSpy, spam_count)

            compare(boolSpy.count, spam_count)
            for (const arg of boolSpy.signalArguments) {
                compare(arg[0], testVal)
            }
        }

        function test_double() {
            const testVal = 123.123
            compare(doubleSpy.count, 0)
            for (let i = 0; i < spam_count; ++i) {
                root.doubleSignal(testVal)
            }
            waitWhileSignalsArrive(doubleSpy, spam_count)

            compare(doubleSpy.count, spam_count)
            for (const arg of doubleSpy.signalArguments) {
                compare(arg[0], testVal)
            }
        }

        function test_string() {
            const testVal = "Testing testing, is this thing on?"
            compare(stringSpy.count, 0)
            for (let i = 0; i < spam_count; ++i) {
                root.stringSignal(testVal)
            }
            waitWhileSignalsArrive(stringSpy, spam_count)

            compare(stringSpy.count, spam_count)
            for (const arg of stringSpy.signalArguments) {
                compare(arg[0], testVal)
            }
        }
    }

    TestCase {
        name: "weirdValues"
        when: windowShown

        IntValidator {
            id: intValidator
        }

        DoubleValidator {
            id: doubleValidator
        }

        function init() { root.wipeSpies() }

        function test_intMin() {
            compare(intSpy.count, 0)
            root.intSignal(intValidator.bottom)
            intSpy.wait(root.signalWaitTime)

            compare(intSpy.count, 1)
            compare(intSpy.signalArguments.length, 1)
            compare(intSpy.signalArguments[0][0], intValidator.bottom)
        }

        function test_intMax() {
            compare(intSpy.count, 0)
            root.intSignal(intValidator.top)
            intSpy.wait(root.signalWaitTime)

            compare(intSpy.count, 1)
            compare(intSpy.signalArguments.length, 1)
            compare(intSpy.signalArguments[0][0], intValidator.top)
        }

        function test_doubleMin() {
            compare(doubleSpy.count, 0)
            root.doubleSignal(doubleValidator.bottom)
            doubleSpy.wait(root.signalWaitTime)

            compare(doubleSpy.count, 1)
            compare(doubleSpy.signalArguments.length, 1)
            compare(doubleSpy.signalArguments[0][0], doubleValidator.bottom)
        }

        function test_doubleMax() {
            compare(doubleSpy.count, 0)
            root.doubleSignal(doubleValidator.top)
            doubleSpy.wait(root.signalWaitTime)

            compare(doubleSpy.count, 1)
            compare(doubleSpy.signalArguments.length, 1)
            compare(doubleSpy.signalArguments[0][0], doubleValidator.top)
        }

        function test_stringLong() {
            compare(stringSpy.count, 0)
            root.stringSignal(veryLongString)
            stringSpy.wait(root.signalWaitTime)

            compare(stringSpy.count, 1)
            compare(stringSpy.signalArguments.length, 1)
            compare(stringSpy.signalArguments[0][0], veryLongString)
        }

        function test_stringEmpty() {
            compare(stringSpy.count, 0)
            root.stringSignal("")
            stringSpy.wait(root.signalWaitTime)

            compare(stringSpy.count, 1)
            compare(stringSpy.signalArguments.length, 1)
            compare(stringSpy.signalArguments[0][0], "")
        }
    }

    // Workaround for QTBUG-137025, allow the Android UI thread to loop.
    TestCase {
        name: "aDelayToAllowProcessing"
        when: windowShown
        function test_a() {
            wait(1000)
            verify(true)
        }
    }

    readonly property string veryLongString: "Lorem ipsum odor amet, consectetuer adipiscing elit. Finibus pretium lobortis nibh platea congue luctus eros eget. Nam neque senectus ligula tristique diam hac. Nam lorem imperdiet arcu morbi imperdiet nulla penatibus elit nibh. Fusce habitasse facilisi lacinia vivamus enim. Neque mauris etiam nulla inceptos cursus. Pharetra diam sapien amet urna dis. Quam nullam rhoncus ad et augue. Lectus hendrerit velit interdum sollicitudin facilisis suspendisse platea vivamus.

Nec euismod neque quis, hendrerit ultricies congue. Conubia habitant inceptos purus fermentum, lobortis vestibulum mus. Habitasse habitasse parturient nullam neque potenti; consequat quis turpis. Neque a enim litora lacus facilisis primis arcu justo? Nisl semper ultricies tortor tristique gravida mollis. Vehicula euismod iaculis ultricies ligula eros dictumst aliquet sagittis. Himenaeos rhoncus ac fusce proin ullamcorper fusce. Efficitur facilisi parturient est orci non pulvinar? Platea eu ante ex montes sociosqu ornare ligula nibh. Nascetur orci justo egestas dis turpis.

Sodales a nisi nisi nec; consectetur aptent gravida varius. Dis morbi consectetur molestie taciti duis mauris habitant vel fusce. Sit praesent convallis class interdum montes, elit urna dis. Lacinia facilisi ultrices efficitur ultricies nulla arcu felis lacinia mauris. Aptent odio sapien himenaeos sodales nunc fermentum tincidunt. Dignissim rhoncus finibus cursus fermentum ipsum est tellus. Vivamus auctor urna orci himenaeos felis nam bibendum finibus nullam. Consequat enim taciti maecenas leo tempus primis. Sit penatibus orci commodo hendrerit auctor lobortis dis.

Cubilia vitae venenatis nunc; turpis primis nunc vulputate. Mauris viverra taciti sagittis eleifend mollis sit etiam ultricies. Dis elementum conubia parturient eu nibh sagittis; dignissim est. Odio fames non imperdiet lobortis lacus facilisi justo. Augue aliquam eu augue duis mi nec dolor. Gravida torquent tempor, montes dictum fames nibh. Nisi elit vitae fringilla amet donec tempor ridiculus dictumst phasellus. Semper curabitur eleifend non hac turpis inceptos. Fames fusce nec feugiat gravida erat taciti.

Blandit ultricies elementum bibendum vulputate nascetur at at. Feugiat netus finibus habitasse, platea odio lectus. Natoque augue natoque sociosqu vitae diam vivamus mi quam. Tincidunt lacinia massa adipiscing nulla blandit laoreet gravida nulla fames. Magna placerat porta dis scelerisque sem. Ultricies pretium suscipit magnis blandit, convallis litora volutpat convallis. Risus leo efficitur sapien porttitor libero nisl metus nam."
}

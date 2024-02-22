// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtNetwork
import QmlTestUri

Item {
    id: root

    property sslKey keyFirst;
    property sslKey keySecond;

    TestCase {
        name: "qtSslKeyTest"

        function test_1initialization() {
            keyFirst.keyFile = ":/assets/key.pem"
            keyFirst.keyAlgorithm = Ssl.Rsa
            keyFirst.keyFormat = Ssl.Pem
            keyFirst.keyType = Ssl.PrivateKey

            keySecond = keyFirst;
        }

        function test_sslConfigurationFields_data() {
            return [
                        { tag: "key is creatable object",
                            field: typeof keyFirst, answer: "object" },
                        { tag: "keySecond is creatable object",
                            field: typeof keySecond, answer: "object" },

                        // key values
                        { tag: "keyFirst.keyFile == :/assets/key.pem",
                            field: keyFirst.keyFile, answer: ":/assets/key.pem" },
                        { tag: "keyFirst.keyAlgorithm == Ssl.Rsa",
                            field: keyFirst.keyAlgorithm, answer: Ssl.Rsa },
                        { tag: "keyFirst.keyFormat == Ssl.Pem",
                            field: keyFirst.keyFormat, answer: Ssl.Pem },
                        { tag: "keyFirst.keyType == Ssl.PrivateKey",
                            field: keyFirst.keyType, answer: Ssl.PrivateKey },

                        // keySecond values
                        { tag: "keySecond.keyFile == :/assets/key.pem",
                            field: keySecond.keyFile, answer: ":/assets/key.pem" },
                        { tag: "keySecond.keyAlgorithm == Ssl.Rsa",
                            field: keySecond.keyAlgorithm, answer: Ssl.Rsa },
                        { tag: "keySecond.keyFormat == Ssl.Pem",
                            field: keySecond.keyFormat, answer: Ssl.Pem },
                        { tag: "keySecond.keyType == Ssl.PrivateKey",
                            field: keySecond.keyType, answer: Ssl.PrivateKey }
                    ]
        }

        function test_sslConfigurationFields(data) {
            compare(data.field, data.answer)
        }
    }
}

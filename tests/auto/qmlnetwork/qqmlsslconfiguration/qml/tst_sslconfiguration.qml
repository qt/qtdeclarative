// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtNetwork
import QmlTestUri

Item {
    id: root

    property sslConfiguration userSslObject;
    property sslConfiguration defaultSslObject;
    property sslDtlsConfiguration dtlsDefaultSslObj;
    property sslKey key;

    TestCase {
        name: "qtSslConfigurationTest"

        function test_1initialization() {
            userSslObject.peerVerifyMode = SslSocket.QueryPeer
            userSslObject.peerVerifyDepth = 0
            userSslObject.protocol = Ssl.TlsV1_2
            userSslObject.sslOptions = [Ssl.SslOptionDisableEmptyFragments]

            defaultSslObject.peerVerifyMode = SslSocket.VerifyPeer;
            dtlsDefaultSslObj.peerVerifyMode = SslSocket.VerifyNone;

            key.keyFile = ":/data/key.pem"
            key.keyAlgorithm = Ssl.Rsa
            key.keyFormat = Ssl.Pem

            // Call invokable functions for ssl object
            userSslObject.setCertificateFiles([":/data/cert.pem"])
            userSslObject.setPrivateKey(key)
        }

        function test_sslEnumsFields_data() {
            return [
                        // enum SslProtocol
                        { tag: "SslProtocol.TlsV1_2OrLater",
                            field: Ssl.TlsV1_2OrLater, answer: 7 },
                        { tag: "SslProtocol.DtlsV1_2",
                            field: Ssl.DtlsV1_2, answer: 10 },
                        { tag: "SslProtocol.DtlsV1_2OrLater",
                            field: Ssl.DtlsV1_2OrLater, answer: 11},
                        { tag: "SslProtocol.TlsV1_3",
                            field: Ssl.TlsV1_3, answer: 12 },
                        { tag: "SslProtocol.TlsV1_3OrLater",
                            field: Ssl.TlsV1_3OrLater, answer: 13 },
                        { tag: "SslProtocol.UnknownProtocol",
                            field: Ssl.UnknownProtocol, answer: -1 },

                        // enum EncodingFormat
                        { tag: "EncodingFormat.Pem",
                            field: Ssl.Pem, answer: 0 },
                        { tag: "EncodingFormat.Der",
                            field: Ssl.Der, answer: 1 },

                        // enum KeyType
                        { tag: "KeyType.key",
                            field: Ssl.PrivateKey, answer: 0 },
                        { tag: "KeyType.PublicKey",
                            field: Ssl.PublicKey, answer: 1 },

                        //enum KeyAlgorithm
                        { tag: "KeyAlgorithm.Opaque",
                            field: Ssl.Opaque, answer: 0 },
                        { tag: "KeyAlgorithm.Rsa",
                            field: Ssl.Rsa, answer: 1 },
                        { tag: "KeyAlgorithm.Dsa",
                            field: Ssl.Dsa, answer: 2 },
                        { tag: "KeyAlgorithm.Ec",
                            field: Ssl.Ec, answer: 3 },
                        { tag: "KeyAlgorithm.Dh",
                            field: Ssl.Dh, answer: 4 },

                        // enum SslOption
                        { tag: "SslOption.SslOptionDisableEmptyFragments",
                            field: Ssl.SslOptionDisableEmptyFragments, answer: 0x01 },
                        { tag: "SslOption.SslOptionDisableSessionTickets",
                            field: Ssl.SslOptionDisableSessionTickets, answer: 0x02 },
                        { tag: "SslOption.SslOptionDisableCompression",
                            field: Ssl.SslOptionDisableCompression, answer: 0x04 },
                        { tag: "SslOption.SslOptionDisableServerNameIndication",
                            field: Ssl.SslOptionDisableServerNameIndication, answer: 0x08 },
                        { tag: "SslOption.SslOptionDisableLegacyRenegotiation",
                            field: Ssl.SslOptionDisableLegacyRenegotiation, answer: 0x10 },
                        { tag: "SslOption.SslOptionDisableSessionSharing",
                            field: Ssl.SslOptionDisableSessionSharing, answer: 0x20 },
                        { tag: "SslOption.SslOptionDisableSessionPersistence",
                            field: Ssl.SslOptionDisableSessionPersistence, answer: 0x40 },
                        { tag: "SslOption.SslOptionDisableServerCipherPreference",
                            field: Ssl.SslOptionDisableServerCipherPreference, answer: 0x80 },

                        // enum PeerVerifyMode
                        { tag: "PeerVerifyMode.VerifyNone",
                            field: SslSocket.VerifyNone, answer: 0 },
                        { tag: "PeerVerifyMode.QueryPeer",
                            field: SslSocket.QueryPeer, answer: 1 },
                        { tag: "PeerVerifyMode.VerifyPeer",
                            field: SslSocket.VerifyPeer, answer: 2 },
                        { tag: "PeerVerifyMode.AutoVerifyPeer",
                            field: SslSocket.AutoVerifyPeer, answer: 3 }
                    ]
        }

        function test_sslEnumsFields(data) {
            compare(data.field, data.answer)
        }

        function test_sslConfigurationFields_data() {
            return [
                        { tag: "userSslObject is creatable object",
                            field: typeof userSslObject, answer: "object" },
                        { tag: "defaultSslObject is creatable object",
                            field: typeof defaultSslObject, answer: "object" },
                        { tag: "dtlsDefaultSslObj is creatable object",
                            field: typeof dtlsDefaultSslObj, answer: "object" },
                        { tag: "key is creatable object",
                            field: typeof key, answer: "object" },
                        { tag: "userSslObject.sslOptions is creatable object",
                            field: typeof userSslObject.sslOptions,
                            answer: "object" },

                        // userSslObject
                        { tag: "userSslObject.peerVerifyMode == Ssl.QueryPeer",
                            field: userSslObject.peerVerifyMode, answer: SslSocket.QueryPeer },
                        // defaultSslObject
                        { tag: "defaultSslObject.peerVerifyMode == Ssl.VerifyPeer",
                            field: defaultSslObject.peerVerifyMode, answer: SslSocket.VerifyPeer },
                        // dtlsDefaultSslObj
                        { tag: "dtlsDefaultSslObj.peerVerifyMode == Ssl.VerifyNone",
                            field: dtlsDefaultSslObj.peerVerifyMode,
                            answer: SslSocket.VerifyNone },

                        // userSslObject
                        { tag: "userSslObject.peerVerifyDepth == 0",
                            field: userSslObject.peerVerifyDepth, answer: 0 },
                        { tag: "userSslObject.sslOptions == Ssl.SslOptionDisableEmptyFragments",
                            field: userSslObject.sslOptions[0],
                            answer: Ssl.SslOptionDisableEmptyFragments },
                        { tag: "SSL configuration protocol == SslProtocol.TlsV1_2",
                            field: userSslObject.protocol, answer: Ssl.TlsV1_2 },
                        { tag: "key.keyFile == :/data/key.pem",
                            field: key.keyFile, answer: ":/data/key.pem" },
                        { tag: "key.keyAlgorithm == Ssl.Rsa",
                            field: key.keyAlgorithm, answer: Ssl.Rsa },
                        { tag: "key.keyFormat == Ssl.Pem",
                            field: key.keyFormat, answer: Ssl.Pem }
                    ]
        }

        function test_sslConfigurationFields(data) {
            compare(data.field, data.answer)
        }
    }
}

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/* Disclaimer: This file is an "as is" copy of the C++ header generated for the
   accompanying HelloWorld.qml. Its pieces are used to:

   * verify that the generated code is similar to what is contained in this file
   * provide documentation snippets for the qmltc docs

   Note: all the stuff below MAGIC_QMLTC_TEST_DELIMITER_LINE comment
   participates in the aforementioned activities. Prefer to put arbitrary stuff
   before that comment.
*/

// MAGIC_QMLTC_TEST_DELIMITER_LINE

//! [qmltc-hello-world-generated]
class HelloWorld : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString hello WRITE setHello READ hello BINDABLE bindableHello)

public:
    HelloWorld(QQmlEngine* engine, QObject* parent = nullptr);

Q_SIGNALS:
    void created();

public:
    void setHello(const QString& hello_);
    QString hello();
    QBindable<QString> bindableHello();
    Q_INVOKABLE void printHello(passByConstRefOrValue<QString> prefix, passByConstRefOrValue<QString> suffix);

    // ...
};
//! [qmltc-hello-world-generated]

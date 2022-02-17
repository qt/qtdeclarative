/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Note: this file is published under a license that is different from a default
//       test sources license. This is intentional to comply with default
//       snippet license.

#include <QtTest/qtest.h>
#include <QtCore/qstring.h>
#include <QtCore/qtimer.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/qquickwindow.h>

//! [qqmlcomponent-include]
#include <QtQml/qqmlcomponent.h>
//! [qqmlcomponent-include]

//! [qmltc-include]
#include "myapp.h" // include generated C++ header
//! [qmltc-include]

#include <algorithm>

class tst_qmltc_examples : public QObject
{
    Q_OBJECT

    static constexpr int m_argc = 1;
    static constexpr char *m_argv[] = { const_cast<char *>("tst_qmltc_examples") };

public:
    tst_qmltc_examples(QObject *parent = nullptr) : QObject(parent) { }

private slots:
    void app();
    void appComponent();

    void helloWorld();
};

#define CREATE_DUMMY_ARGC_ARGV() \
    int argc = 1; \
    char *argv[] = { const_cast<char *>("tst_qmltc_examples") };

void tst_qmltc_examples::app()
{
    CREATE_DUMMY_ARGC_ARGV()

    //! [qmltc-app-code]
    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("This example is powered by qmltc!"));

    QQmlEngine e;
    QQuickWindow window;

    QScopedPointer<myApp> documentRoot(new myApp(&e));

    documentRoot->setParentItem(window.contentItem());
    window.setHeight(documentRoot->height());
    window.setWidth(documentRoot->width());
    // ...
    //! [qmltc-app-code]

    QTimer::singleShot(1000, &app, QGuiApplication::quit);

    //! [qmltc-app-exec]
    window.show();
    app.exec();
    //! [qmltc-app-exec]
}

void tst_qmltc_examples::appComponent()
{
    CREATE_DUMMY_ARGC_ARGV()

    //! [qqmlcomponent-app-code-0]
    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("This example is powered by QQmlComponent :("));

    QQmlEngine e;
    QQuickWindow window;

    QQmlComponent component(&e);
    component.loadUrl(QUrl(QStringLiteral("qrc:/QmltcExample/myApp.qml")));
    //! [qqmlcomponent-app-code-0]

    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    //! [qqmlcomponent-app-code-1]
    QScopedPointer<QObject> documentRoot(component.create());
    QQuickItem *documentRootItem = qobject_cast<QQuickItem *>(documentRoot.get());
    //! [qqmlcomponent-app-code-1]

    QVERIFY(documentRootItem);

    //! [qqmlcomponent-app-code-2]
    documentRootItem->setParentItem(window.contentItem());
    window.setHeight(documentRootItem->height());
    window.setWidth(documentRootItem->width());
    // ...
    //! [qqmlcomponent-app-code-2]

    QTimer::singleShot(1000, &app, QGuiApplication::quit);

    window.show();
    app.exec();
}

#if !defined(QMLTC_TESTS_SOURCE_DIR) || !defined(QMLTC_TESTS_BINARY_DIR)
#    error "Tests assume that QMLTC_TESTS_{SOURCE,BINARY}_DIR are specified (through CMake)"
#endif

// Note: QtTest macros need to be in void-returning function, so use output arg.
template<typename Predicate>
void readFileContent(QStringList *content, const QString &url, Predicate filter)
{
    QVERIFY(content);

    QFile file(url);
    QVERIFY(file.exists());
    QVERIFY(file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text));

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (filter(line))
            content->append(std::move(line));
    }
}

void tst_qmltc_examples::helloWorld()
{
#ifdef Q_OS_ANDROID
    QSKIP("expected C++ files are not bundled with Android tests.");
#endif
    QStringList generatedCode;
    readFileContent(&generatedCode,
                    QStringLiteral(QMLTC_TESTS_BINARY_DIR)
                            + u"/.qmltc/tst_qmltc_examples/helloworld.h",
                    [](const QString &) { return true; });
    if (QTest::currentTestFailed())
        QFAIL("Reading _generated_ C++ content for special/HelloWorld.qml failed");

    QStringList documentationCode;
    const auto filterDocumentationLines = [encounteredStart = false](QStringView line) mutable {
        if (line.startsWith(u"// MAGIC_QMLTC_TEST_DELIMITER_LINE")) {
            encounteredStart = true;
            return false; // we don't need this specific line
        }
        if (!encounteredStart)
            return false;
        line = line.trimmed();
        return !line.isEmpty() && !line.startsWith(u"//");
    };
    readFileContent(&documentationCode,
                    QStringLiteral(QMLTC_TESTS_SOURCE_DIR) + u"/special/HelloWorld.qml.cpp",
                    filterDocumentationLines);
    if (QTest::currentTestFailed())
        QFAIL("Reading special/HelloWorld.qml.cpp failed");

    QVERIFY(!generatedCode.isEmpty());
    QVERIFY(!documentationCode.isEmpty());

    auto begin = generatedCode.cbegin();
    for (const QString &existingString : qAsConst(documentationCode)) {
        auto pos = std::find(begin, generatedCode.cend(), existingString);
        QVERIFY2(pos != generatedCode.cend(), qPrintable(u"Could not find: " + existingString));
        begin = std::next(pos);
    }
}

#undef CREATE_DUMMY_ARGC_ARGV

QTEST_APPLESS_MAIN(tst_qmltc_examples)
#include "tst_qmltc_examples.moc"

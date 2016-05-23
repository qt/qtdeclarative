/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QtQuick>

typedef QPair<QString, QString> QStringPair;

class tst_Snippets : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void screenshots();
    void screenshots_data();

private:
    QMap<QString, QStringPair> filePaths;
    QStringList nonVisualSnippets;
};

void tst_Snippets::initTestCase()
{
    QDir outdir(QDir::current().filePath("screenshots"));
    QVERIFY(outdir.exists() || QDir::current().mkpath("screenshots"));

    QString datadir(QQC2_SNIPPETS_PATH);
    QVERIFY(!datadir.isEmpty());

    qInfo() << datadir;

    QDirIterator it(datadir, QStringList() << "qtquick*.qml" << "qtlabs*.qml", QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        filePaths.insert(fi.baseName(), qMakePair(fi.filePath(), outdir.filePath(fi.baseName() + ".png")));
    }
    QVERIFY(!filePaths.isEmpty());

    nonVisualSnippets << "qtquickcontrols2-stackview-custom.qml"
                      << "qtquickcontrols2-swipeview-custom.qml"
                      << "qtquickcontrols2-tooltip-custom.qml";
}

Q_DECLARE_METATYPE(QList<QQmlError>)

void tst_Snippets::screenshots()
{
    QFETCH(QString, input);
    QFETCH(QString, output);

    qRegisterMetaType<QList<QQmlError> >();

    QQuickView view;
    QSignalSpy warnings(view.engine(), SIGNAL(warnings(QList<QQmlError>)));
    QVERIFY(warnings.isValid());

    view.setSource(QUrl::fromLocalFile(input));
    QCOMPARE(view.status(), QQuickView::Ready);
    QVERIFY(view.errors().isEmpty());
    QVERIFY(view.rootObject());

    QVERIFY(warnings.isEmpty());

    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    bool generateScreenshot = true;
    foreach (const QString &baseName, nonVisualSnippets) {
        if (input.contains(baseName)) {
            generateScreenshot = false;
            break;
        }
    }

    if (generateScreenshot) {
        QSharedPointer<QQuickItemGrabResult> result = view.contentItem()->grabToImage();
        QSignalSpy spy(result.data(), SIGNAL(ready()));
        QVERIFY(spy.isValid());
        QVERIFY(spy.wait());
        QVERIFY(result->saveToFile(output));
    }

    QGuiApplication::processEvents();
}

void tst_Snippets::screenshots_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    QMap<QString, QStringPair>::const_iterator it;
    for (it = filePaths.constBegin(); it != filePaths.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.value().first << it.value().second;
}

QTEST_MAIN(tst_Snippets)

#include "tst_snippets.moc"

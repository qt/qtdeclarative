// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <qqml.h>

#include <private/qqmlglobal_p.h>

class tst_qqmlglobal : public QObject
{
    Q_OBJECT
public:
    tst_qqmlglobal() {}

private slots:
    void initTestCase();

    void colorProviderWarning();
    void noGuiProviderWarning();
};

void tst_qqmlglobal::initTestCase()
{
}

void tst_qqmlglobal::colorProviderWarning()
{
    const QLatin1String expected("Warning: QQml_colorProvider: no color provider has been set!");
    QTest::ignoreMessage(QtWarningMsg, expected.data());
    QQml_colorProvider();
}

void tst_qqmlglobal::noGuiProviderWarning()
{
    QVERIFY(QQml_guiProvider()); //No GUI provider, so a default non-zero application instance is returned.
}

QTEST_MAIN(tst_qqmlglobal)

#include "tst_qqmlglobal.moc"

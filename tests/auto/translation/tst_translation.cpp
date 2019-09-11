/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtTest/qtest.h>
#include "../shared/visualtestutil.h"

#include <QtCore/qtranslator.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

using namespace QQuickVisualTestUtil;

class tst_translation : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void dialogButtonBox();
};

void tst_translation::dialogButtonBox()
{
    QQuickView view(testFileUrl("dialogButtonBox.qml"));
    if (view.status() != QQuickView::Ready)
        QFAIL("Failed to load QML file");
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickDialog *dialog = view.rootObject()->property("dialog").value<QQuickDialog*>();
    QVERIFY(dialog);

    QQuickDialogButtonBox *dialogButtonBox = qobject_cast<QQuickDialogButtonBox*>(dialog->footer());
    QVERIFY(dialogButtonBox);

    QQuickAbstractButton *saveButton = dialogButtonBox->standardButton(QPlatformDialogHelper::Save);
    QVERIFY(saveButton);
    QString defaultSaveText = QGuiApplicationPrivate::platformTheme()->standardButtonText(QPlatformDialogHelper::Save);
    defaultSaveText = QPlatformTheme::removeMnemonics(defaultSaveText);
    QCOMPARE(saveButton->text(), defaultSaveText);

    QQuickAbstractButton *discardButton = dialogButtonBox->standardButton(QPlatformDialogHelper::Discard);
    QVERIFY(discardButton);
    QString defaultDiscardText = QGuiApplicationPrivate::platformTheme()->standardButtonText(QPlatformDialogHelper::Discard);
    defaultDiscardText = QPlatformTheme::removeMnemonics(defaultDiscardText);
    QCOMPARE(discardButton->text(), defaultDiscardText);

    QTranslator translator;
    QVERIFY(translator.load(":/i18n/qtbase_fr.qm"));
    QVERIFY(qApp->installTranslator(&translator));
    view.engine()->retranslate();

    QString translatedSaveText = QGuiApplicationPrivate::platformTheme()->standardButtonText(QPlatformDialogHelper::Save);
    translatedSaveText = QPlatformTheme::removeMnemonics(translatedSaveText);
    QCOMPARE(saveButton->text(), translatedSaveText);

    QString translatedDiscardText = QGuiApplicationPrivate::platformTheme()->standardButtonText(QPlatformDialogHelper::Discard);
    translatedDiscardText = QPlatformTheme::removeMnemonics(translatedDiscardText);
    QCOMPARE(discardButton->text(), translatedDiscardText);
}

QTEST_MAIN(tst_translation)

#include "tst_translation.moc"

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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtQuickTest/quicktest.h>

static const char* styles[] = { "Material", "Universal" };

int main(int argc, char *argv[])
{
    QByteArray style = qgetenv("QT_QUICK_CONTROLS_STYLE");
    if (!style.isEmpty())
        return quick_test_main(argc, argv, "tst_styles(" + style + ")", TST_CONTROLS_DATA);

    QCoreApplication app(argc, argv);

    int failures = 0;
    int count = sizeof(styles) / sizeof(styles[0]);

    for (int i = 0; i < count; ++i) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("QT_QUICK_CONTROLS_STYLE", styles[i]);

        QProcess process;
        process.setProcessEnvironment(env);
        process.setWorkingDirectory(QDir::currentPath());
        process.setProcessChannelMode(QProcess::ForwardedChannels);

        process.start(argv[0], app.arguments().mid(1));
        process.waitForFinished();
        if (process.exitStatus() != QProcess::NormalExit)
            return -1;

        failures += process.exitCode();
    }

    return failures;
}

/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuickTest/quicktest.h>

class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool bindingLoopDetected READ wasBindingLoopDetected FINAL)

public:
    Setup() {}

    bool wasBindingLoopDetected() const { return mBindingLoopDetected; }

public slots:
    void reset() { mBindingLoopDetected = false; }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        connect(engine, &QQmlEngine::warnings, this, &Setup::qmlWarnings);

        qmlRegisterSingletonInstance("org.qtproject.Test", 1, 0, "BindingLoopDetector", this);
    }

    void qmlWarnings(const QList<QQmlError> &warnings)
    {
        for (const auto error : warnings) {
            if (error.messageType() == QtWarningMsg && error.description().contains(QStringLiteral("Binding loop detected")))
                mBindingLoopDetected = true;
        }
    }

private:
    bool mBindingLoopDetected = false;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_qquickmaterialstyle, Setup)

#include "tst_qquickmaterialstyle.moc"

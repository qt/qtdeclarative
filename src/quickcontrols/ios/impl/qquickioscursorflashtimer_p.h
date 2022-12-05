// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKIOSCURSORFLASHTIMER_P_H
#define QQUICKIOSCURSORFLASHTIMER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickIOSCursorFlashTimer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged FINAL)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged FINAL)
    QML_NAMED_ELEMENT(CursorFlashTimer)

public:
    explicit QQuickIOSCursorFlashTimer(QObject *parent = nullptr);

    bool visible() const;
    void setVisible(bool visible);

    int cursorPosition() const;
    void setCursorPosition(int cursorPosition);

    bool running() const;
    void setRunning(bool running);

    void start();
    void stop();

    void timerEvent(QTimerEvent *event) override;

Q_SIGNALS:
    void visibleChanged();
    void cursorPositionChanged();
    void runningChanged(bool running);

private:
    bool m_visible = false;
    int m_cursorPosition = 0;
    int m_timer = 0;
    bool m_running = false;
};

QT_END_NAMESPACE

#endif // QQUICKIOSCURSORFLASHTIMER_P_H

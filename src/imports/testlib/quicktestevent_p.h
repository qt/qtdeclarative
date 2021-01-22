/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QUICKTESTEVENT_P_H
#define QUICKTESTEVENT_P_H

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

#include <QtCore/qobject.h>
#include <QtGui/QWindow>
#include <QtQml/qqml.h>
#include <QtTest/qtesttouch.h>

QT_BEGIN_NAMESPACE

class QuickTestEvent;
class QQuickTouchEventSequence : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit QQuickTouchEventSequence(QuickTestEvent *testEvent, QObject *item = nullptr);
public slots:
    QObject* press(int touchId, QObject *item, qreal x, qreal y);
    QObject* move(int touchId, QObject *item, qreal x, qreal y);
    QObject* release(int touchId, QObject *item, qreal x, qreal y);
    QObject* stationary(int touchId);
    QObject* commit();

private:
    QTest::QTouchEventSequence m_sequence;
    QuickTestEvent * const m_testEvent;
};

class QuickTestEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int defaultMouseDelay READ defaultMouseDelay FINAL)
    QML_NAMED_ELEMENT(TestEvent)
public:
    QuickTestEvent(QObject *parent = nullptr);
    ~QuickTestEvent() override;
    int defaultMouseDelay() const;

public Q_SLOTS:
    bool keyPress(int key, int modifiers, int delay);
    bool keyRelease(int key, int modifiers, int delay);
    bool keyClick(int key, int modifiers, int delay);

    bool keyPressChar(const QString &character, int modifiers, int delay);
    bool keyReleaseChar(const QString &character, int modifiers, int delay);
    bool keyClickChar(const QString &character, int modifiers, int delay);

    Q_REVISION(2) bool keySequence(const QVariant &keySequence);

    bool mousePress(QObject *item, qreal x, qreal y, int button,
                    int modifiers, int delay);
    bool mouseRelease(QObject *item, qreal x, qreal y, int button,
                      int modifiers, int delay);
    bool mouseClick(QObject *item, qreal x, qreal y, int button,
                    int modifiers, int delay);
    bool mouseDoubleClick(QObject *item, qreal x, qreal y, int button,
                          int modifiers, int delay);
    bool mouseDoubleClickSequence(QObject *item, qreal x, qreal y, int button,
                          int modifiers, int delay);
    bool mouseMove(QObject *item, qreal x, qreal y, int delay, int buttons);

#if QT_CONFIG(wheelevent)
    bool mouseWheel(QObject *item, qreal x, qreal y, int buttons,
               int modifiers, int xDelta, int yDelta, int delay);
#endif

    QQuickTouchEventSequence *touchEvent(QObject *item = nullptr);
private:
    QWindow *eventWindow(QObject *item = nullptr);
    QWindow *activeWindow();
    QTouchDevice *touchDevice();

    Qt::MouseButtons m_pressedButtons;

    friend class QQuickTouchEventSequence;
};

QT_END_NAMESPACE

#endif

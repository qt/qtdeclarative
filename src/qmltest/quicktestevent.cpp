/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quicktestevent_p.h"
#include <QtTest/qtestkeyboard.h>
#include <QtQml/qqml.h>
#if defined(QML_VERSION) && QML_VERSION >= 0x020000
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickcanvas.h>
#define QUICK_TEST_SCENEGRAPH 1
#endif
#include <QtWidgets/qgraphicsscene.h>

QT_BEGIN_NAMESPACE

QuickTestEvent::QuickTestEvent(QObject *parent)
    : QObject(parent)
{
}

QuickTestEvent::~QuickTestEvent()
{
}

bool QuickTestEvent::keyPress(int key, int modifiers, int delay)
{
    QWindow *window = eventWindow();
    if (!window)
        return false;
    QTest::keyPress(window, Qt::Key(key), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyRelease(int key, int modifiers, int delay)
{
    QWindow *window = eventWindow();
    if (!window)
        return false;
    QTest::keyRelease(window, Qt::Key(key), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyClick(int key, int modifiers, int delay)
{
    QWindow *window = eventWindow();
    if (!window)
        return false;
    QTest::keyClick(window, Qt::Key(key), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

namespace QTest {
    extern int Q_TESTLIB_EXPORT defaultMouseDelay();
};

namespace QtQuickTest
{
    enum MouseAction { MousePress, MouseRelease, MouseClick, MouseDoubleClick, MouseMove };

    static void mouseEvent(MouseAction action, QWindow *window,
                           QObject *item, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey, QPointF _pos, int delay=-1)
    {
        QTEST_ASSERT(window);
        QTEST_ASSERT(item);

        if (delay == -1 || delay < QTest::defaultMouseDelay())
            delay = QTest::defaultMouseDelay();
        if (delay > 0)
            QTest::qWait(delay);

        if (action == MouseClick) {
            mouseEvent(MousePress, window, item, button, stateKey, _pos);
            mouseEvent(MouseRelease, window, item, button, stateKey, _pos);
            return;
        }

        QPoint pos;
#ifdef QUICK_TEST_SCENEGRAPH
        QQuickItem *sgitem = qobject_cast<QQuickItem *>(item);
        if (sgitem) {
            pos = sgitem->mapToScene(_pos).toPoint();
        } else
#endif
        {
            qWarning("No suitable QtQuick1 implementation is available!");
        }
        QTEST_ASSERT(button == Qt::NoButton || button & Qt::MouseButtonMask);
        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);

        QMouseEvent me(QEvent::User, QPoint(), Qt::LeftButton, button, stateKey);
        switch (action)
        {
            case MousePress:
                me = QMouseEvent(QEvent::MouseButtonPress, pos, window->mapToGlobal(pos), button, button, stateKey);
                break;
            case MouseRelease:
                me = QMouseEvent(QEvent::MouseButtonRelease, pos, window->mapToGlobal(pos), button, 0, stateKey);
                break;
            case MouseDoubleClick:
                me = QMouseEvent(QEvent::MouseButtonDblClick, pos, window->mapToGlobal(pos), button, button, stateKey);
                break;
            case MouseMove:
                // with move event the button is NoButton, but 'buttons' holds the currently pressed buttons
                me = QMouseEvent(QEvent::MouseMove, pos, window->mapToGlobal(pos), Qt::NoButton, button, stateKey);
                break;
            default:
                QTEST_ASSERT(false);
        }
        QSpontaneKeyEvent::setSpontaneous(&me);
        if (!qApp->notify(window, &me)) {
            static const char *mouseActionNames[] =
                { "MousePress", "MouseRelease", "MouseClick", "MouseDoubleClick", "MouseMove" };
            QString warning = QString::fromLatin1("Mouse event \"%1\" not accepted by receiving window");
            QWARN(warning.arg(QString::fromLatin1(mouseActionNames[static_cast<int>(action)])).toAscii().data());
        }
    }

    static void mouseWheel(QWindow* window, QObject* item, Qt::MouseButtons buttons,
                                Qt::KeyboardModifiers stateKey,
                                QPointF _pos, int delta, int delay = -1, Qt::Orientation orientation = Qt::Vertical)
    {
        QTEST_ASSERT(window);
        QTEST_ASSERT(item);
        if (delay == -1 || delay < QTest::defaultMouseDelay())
            delay = QTest::defaultMouseDelay();
        if (delay > 0)
            QTest::qWait(delay);

        QPoint pos;
#ifdef QUICK_TEST_SCENEGRAPH
        QQuickItem *sgitem = qobject_cast<QQuickItem *>(item);
        if (sgitem) {
            pos = sgitem->mapToScene(_pos).toPoint();
        } else
#endif
        {
            qWarning("No suitable QtQuick1 implementation is available!");
        }
        QTEST_ASSERT(buttons == Qt::NoButton || buttons & Qt::MouseButtonMask);
        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);
        QWheelEvent we(pos, window->mapToGlobal(pos), delta, buttons, stateKey, orientation);

        QSpontaneKeyEvent::setSpontaneous(&we); // hmmmm
        if (!qApp->notify(window, &we))
            QTest::qWarn("Wheel event not accepted by receiving window");
    }
};

bool QuickTestEvent::mousePress
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow();
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MousePress, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseWheel(
    QObject *item, qreal x, qreal y, int buttons,
    int modifiers, int delta, int delay, int orientation)
{
    QWindow *view = eventWindow();
    if (!view)
        return false;
    QtQuickTest::mouseWheel(view, item, Qt::MouseButtons(buttons),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delta, delay, Qt::Orientation(orientation));
    return true;
}

bool QuickTestEvent::mouseRelease
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow();
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseRelease, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseClick
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow();
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseClick, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseDoubleClick
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow();
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseDoubleClick, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseMove
    (QObject *item, qreal x, qreal y, int delay, int buttons)
{
    QWindow *view = eventWindow();
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseMove, view, item,
                            Qt::MouseButton(buttons), Qt::NoModifier,
                            QPointF(x, y), delay);
    return true;
}

QWindow *QuickTestEvent::eventWindow()
{
#ifdef QUICK_TEST_SCENEGRAPH
    QQuickItem *sgitem = qobject_cast<QQuickItem *>(parent());
    if (sgitem)
        return sgitem->canvas();
#endif
    return 0;
    /*
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (!item)
        return 0;
    QGraphicsScene *s = item->scene();
    if (!s)
        return 0;
    QList<QGraphicsView *> views = s->views();
    if (views.isEmpty())
        return 0;
    return views.at(0)->windowHandle();
    */
}

QT_END_NAMESPACE

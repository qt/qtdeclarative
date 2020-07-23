/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "inputinspector.h"
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtCore/QSet>

static const int timerInterval = 100;

InputInspector::InputInspector(QObject *parent) : QObject(parent)
{
}

InputInspector::~InputInspector()
{
    m_window = nullptr;
    killTimer(m_timerId);
}

QString InputInspector::mouseGrabber() const
{
    QString name;
    if (m_window) {
        if (QQuickItem *grabber = m_window->mouseGrabberItem()) {
            name = objectIdentifier(grabber);
        } else {
            name = QLatin1String("no grabber");
        }
    } else {
        name = "ERROR: need a source window";
    }
    return name;
}

QString InputInspector::passiveGrabbers() const
{
    return vectorStringJoin(passiveGrabbers_helper());
}

QString InputInspector::exclusiveGrabbers() const
{
    return vectorStringJoin(exclusiveGrabbers_helper());
}

QString InputInspector::vectorStringJoin(const QVector<QObject*> &arr) const
{
    QString res;
    for (QObject* obj: arr) {
        if (!res.isEmpty())
            res += QLatin1String(" , ");
        res += objectIdentifier(obj);
    }
    res.prepend(QLatin1String("["));
    res += QLatin1String("]");
    return res;
}

QQuickWindow *InputInspector::source() const
{
    return m_window;
}

void InputInspector::setSource(QQuickWindow *window)
{
    m_window = window;
    if (m_window && !m_timerId)
        m_timerId = startTimer(timerInterval);
    emit sourceChanged();
}

void InputInspector::update()
{
    const QString mouseGrabberName = mouseGrabber();
    if (lastState.mouseGrabber != mouseGrabberName) {
        emit mouseGrabberChanged();
        lastState.mouseGrabber = mouseGrabberName;
    }

    const QString tempPassiveGrabbers = passiveGrabbers();
    if (lastState.passiveGrabbers != tempPassiveGrabbers) {
        emit passiveGrabbersChanged();
        lastState.passiveGrabbers = tempPassiveGrabbers;
    }

    const QString tempExclusiveGrabbers = exclusiveGrabbers();
    if (lastState.exclusiveGrabbers != tempExclusiveGrabbers) {
        emit exclusiveGrabbersChanged();
        lastState.exclusiveGrabbers = tempExclusiveGrabbers;
    }
}

void InputInspector::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timerId)
        update();
}

const QPointingDevice *InputInspector::pointerDevice() const
{
    const QPointingDevice *device = nullptr;
    for (const auto dev : QInputDevice::devices()) {
        if (dev->type() == QInputDevice::DeviceType::TouchScreen) {
            device = static_cast<const QPointingDevice *>(dev);
            break;
        }
    }
    if (!device)
        device = QPointingDevice::primaryPointingDevice();
    return device;
}

QVector<QObject*> InputInspector::passiveGrabbers_helper(int pointId /*= 0*/) const
{
    QVector<QObject*> result;
    const QPointingDevice *device = pointerDevice();
    if (device && source()) {
        for (auto eventPoint : QPointingDevicePrivate::get(device)->activePoints) {
            if (!pointId || eventPoint.id() == pointId) {
                for (auto pg : eventPoint.passiveGrabbers()) {
                    if (!result.contains(pg))
                        result << pg;
                }
            }
        }
    }
    return result;
}

QVector<QObject*> InputInspector::exclusiveGrabbers_helper(int pointId /*= 0*/) const
{
    QVector<QObject*> result;
    const QPointingDevice *device = pointerDevice();
    if (device && source()) {
        for (auto eventPoint : QPointingDevicePrivate::get(device)->activePoints) {
            if (!pointId || eventPoint.id() == pointId) {
                if (auto g = eventPoint.exclusiveGrabber()) {
                    if (!result.contains(g))
                        result << g;
                }
            }
        }
    }
    return result;
}

QString InputInspector::objectIdentifier(QObject *o)
{
    QString name;
    name = o->objectName();
    if (name.isEmpty())
        name = o->property("text").toString();
    if (name.isEmpty())
        name = o->metaObject()->className();
    if (name.isEmpty())
        name = QLatin1String("unknown");

    return name;
}

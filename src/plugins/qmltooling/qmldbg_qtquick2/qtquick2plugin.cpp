/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtquick2plugin.h"
#include "qquickviewinspector.h"

#include <QtCore/qplugin.h>
#include <QtDeclarative/private/qdeclarativeinspectorservice_p.h>
#include <QtQuick/QQuickView>

namespace QmlJSDebugger {
namespace QtQuick2 {

QtQuick2Plugin::QtQuick2Plugin() :
    m_inspector(0)
{
}

QtQuick2Plugin::~QtQuick2Plugin()
{
    delete m_inspector;
}

bool QtQuick2Plugin::canHandleView(QObject *view)
{
    return qobject_cast<QQuickView*>(view);
}

void QtQuick2Plugin::activate(QObject *view)
{
    QQuickView *qtQuickView = qobject_cast<QQuickView*>(view);
    Q_ASSERT(qtQuickView);
    m_inspector = new QQuickViewInspector(qtQuickView, qtQuickView);
}

void QtQuick2Plugin::deactivate()
{
    delete m_inspector;
}

void QtQuick2Plugin::clientMessage(const QByteArray &message)
{
    if (m_inspector)
        m_inspector->handleMessage(message);
}

} // namespace QtQuick2
} // namespace QmlJSDebugger

Q_EXPORT_PLUGIN2(qmldbg_qtquick2, QmlJSDebugger::QtQuick2::QtQuick2Plugin)

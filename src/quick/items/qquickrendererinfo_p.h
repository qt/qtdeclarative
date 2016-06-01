/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKRENDERERINFO_P_H
#define QQUICKRENDERERINFO_P_H

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
#include <QtCore/qpointer.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickWindow;

class QQuickRendererInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GraphicsApi api READ api NOTIFY apiChanged FINAL)

public:
    // must match QSGRendererInterface
    enum GraphicsApi {
        Unknown,
        Software,
        OpenGL,
        Direct3D12
    };
    Q_ENUM(GraphicsApi)

    QQuickRendererInfo(QQuickItem *item = 0);

    static QQuickRendererInfo *qmlAttachedProperties(QObject *object);

    GraphicsApi api() const { return m_api; }

Q_SIGNALS:
    void apiChanged();

private Q_SLOTS:
    void updateInfo();
    void setWindow(QQuickWindow *window);

private:
    QPointer<QQuickWindow> m_window;
    GraphicsApi m_api;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickRendererInfo, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKRENDERERINFO_P_H

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKRENDERCONTROL_P_H
#define QQUICKRENDERCONTROL_P_H

#include <QtGui/QImage>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QSGContext;
class QSGRenderContext;
class QAnimationDriver;
class QOpenGLContext;
class QQuickRenderControlPrivate;

class Q_QUICK_PRIVATE_EXPORT QQuickRenderControl : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickRenderControl)
public:
    QQuickRenderControl();
    ~QQuickRenderControl();

    QQuickWindow *window() const;
    QQuickWindow *createOffscreenWindow();
    virtual QWindow *renderWindow(QPoint *offset) { Q_UNUSED(offset); return 0; }
    static QWindow *renderWindowFor(QQuickWindow *win, QPoint *offset = 0);

    void windowDestroyed();

    void initialize(QOpenGLContext *gl);
    void invalidate();
    void polishItems();
    void render();
    bool sync();
    void stop();

    QImage grab();

Q_SIGNALS:
    void renderRequested();
    void sceneChanged();

private:
    friend class QQuickWindowPrivate;
    friend class QQuickWindow;
    void setWindow(QQuickWindow *window);
    inline void update() { /*emit*/ renderRequested(); }
    inline void maybeUpdate() { /*emit*/ sceneChanged(); }

    QSGContext *sceneGraphContext() const;
    QSGRenderContext *renderContext(QSGContext *) const;
};

QT_END_NAMESPACE

#endif // QQUICKRENDERCONTROL_P_H

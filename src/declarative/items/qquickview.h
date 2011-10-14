// Commit: 0b83a2161261be525f01359397ab1c8c34827749
/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKVIEW_H
#define QQUICKVIEW_H

#include <qquickcanvas.h>
#include <QtCore/qurl.h>
#include <QtDeclarative/qdeclarativedebug.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeEngine;
class QDeclarativeContext;
class QDeclarativeError;
class QQuickItem;

class QQuickViewPrivate;
class Q_DECLARATIVE_EXPORT QQuickView : public QQuickCanvas
{
    Q_OBJECT
    Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource DESIGNABLE true)
    Q_ENUMS(ResizeMode Status)
public:
    explicit QQuickView(QWindow *parent = 0, Qt::WindowFlags f = 0);
    QQuickView(const QUrl &source, QWindow *parent = 0, Qt::WindowFlags f = 0);
    virtual ~QQuickView();

    QUrl source() const;

    QDeclarativeEngine* engine() const;
    QDeclarativeContext* rootContext() const;

    QQuickItem *rootObject() const;

    enum ResizeMode { SizeViewToRootObject, SizeRootObjectToView };
    ResizeMode resizeMode() const;
    void setResizeMode(ResizeMode);

    enum Status { Null, Ready, Loading, Error };
    Status status() const;

    QList<QDeclarativeError> errors() const;

    QSize sizeHint() const;
    QSize initialSize() const;

public Q_SLOTS:
    void setSource(const QUrl&);

Q_SIGNALS:
    void statusChanged(QQuickView::Status);

private Q_SLOTS:
    void continueExecute();

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent(QTimerEvent*);

    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
private:
    Q_DISABLE_COPY(QQuickView)
    Q_DECLARE_PRIVATE(QQuickView)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKVIEW_H

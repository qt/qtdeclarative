/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKSCREEN_P_H
#define QQUICKSCREEN_P_H

#include <qqml.h>
#include <QRect>
#include <QSize>
#include <private/qqmlglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QQuickItem;
class QQuickCanvas;
class QScreen;

class Q_AUTOTEST_EXPORT QQuickScreenAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(Qt::ScreenOrientation primaryOrientation READ primaryOrientation NOTIFY primaryOrientationChanged)
    Q_PROPERTY(Qt::ScreenOrientation orientation READ orientation NOTIFY orientationChanged)

public:
    QQuickScreenAttached(QObject* attachee);

    int width() const;
    int height() const;
    Qt::ScreenOrientation primaryOrientation() const;
    Qt::ScreenOrientation orientation() const;

    //Treats int as Qt::ScreenOrientation, due to QTBUG-20639
    Q_INVOKABLE int angleBetween(int a, int b);

    void canvasChanged(QQuickCanvas*);

Q_SIGNALS:
    void widthChanged();
    void heightChanged();
    void primaryOrientationChanged();
    void orientationChanged();

private:
    QScreen* m_screen;
    QQuickItem* m_attachee;
};

class Q_AUTOTEST_EXPORT QQuickScreen : public QObject
{
    Q_OBJECT
public:
    static QQuickScreenAttached *qmlAttachedProperties(QObject *object){ return new QQuickScreenAttached(object); }
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickScreen, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER

#endif

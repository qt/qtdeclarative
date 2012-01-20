/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef QDECLARATIVETIMELINE_H
#define QDECLARATIVETIMELINE_H

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

#include <QtCore/QObject>
#include <QtCore/QAbstractAnimation>

QT_BEGIN_NAMESPACE

class QEasingCurve;
class QDeclarative1TimeLineValue;
class QDeclarative1TimeLineCallback;
struct QDeclarative1TimeLinePrivate;
class QDeclarative1TimeLineObject;
class Q_AUTOTEST_EXPORT QDeclarative1TimeLine : public QAbstractAnimation
{
Q_OBJECT
public:
    QDeclarative1TimeLine(QObject *parent = 0);
    ~QDeclarative1TimeLine();

    enum SyncMode { LocalSync, GlobalSync };
    SyncMode syncMode() const;
    void setSyncMode(SyncMode);

    void pause(QDeclarative1TimeLineObject &, int);
    void callback(const QDeclarative1TimeLineCallback &);
    void set(QDeclarative1TimeLineValue &, qreal);

    int accel(QDeclarative1TimeLineValue &, qreal velocity, qreal accel);
    int accel(QDeclarative1TimeLineValue &, qreal velocity, qreal accel, qreal maxDistance);
    int accelDistance(QDeclarative1TimeLineValue &, qreal velocity, qreal distance);

    void move(QDeclarative1TimeLineValue &, qreal destination, int time = 500);
    void move(QDeclarative1TimeLineValue &, qreal destination, const QEasingCurve &, int time = 500);
    void moveBy(QDeclarative1TimeLineValue &, qreal change, int time = 500);
    void moveBy(QDeclarative1TimeLineValue &, qreal change, const QEasingCurve &, int time = 500);

    void sync();
    void setSyncPoint(int);
    int syncPoint() const;

    void sync(QDeclarative1TimeLineValue &);
    void sync(QDeclarative1TimeLineValue &, QDeclarative1TimeLineValue &);

    void reset(QDeclarative1TimeLineValue &);

    void complete();
    void clear();
    bool isActive() const;

    int time() const;

    virtual int duration() const;
Q_SIGNALS:
    void updated();
    void completed();

protected:
    virtual void updateCurrentTime(int);

private:
    void remove(QDeclarative1TimeLineObject *);
    friend class QDeclarative1TimeLineObject;
    friend struct QDeclarative1TimeLinePrivate;
    QDeclarative1TimeLinePrivate *d;
};

class Q_AUTOTEST_EXPORT QDeclarative1TimeLineObject
{
public:
    QDeclarative1TimeLineObject();
    virtual ~QDeclarative1TimeLineObject();

protected:
    friend class QDeclarative1TimeLine;
    friend struct QDeclarative1TimeLinePrivate;
    QDeclarative1TimeLine *_t;
};

class Q_AUTOTEST_EXPORT QDeclarative1TimeLineValue : public QDeclarative1TimeLineObject
{
public:
    QDeclarative1TimeLineValue(qreal v = 0.) : _v(v) {}

    virtual qreal value() const { return _v; }
    virtual void setValue(qreal v) { _v = v; }

    QDeclarative1TimeLine *timeLine() const { return _t; }

    operator qreal() const { return _v; }
    QDeclarative1TimeLineValue &operator=(qreal v) { setValue(v); return *this; }
private:
    friend class QDeclarative1TimeLine;
    friend struct QDeclarative1TimeLinePrivate;
    qreal _v;
};

class Q_AUTOTEST_EXPORT QDeclarative1TimeLineCallback
{
public:
    typedef void (*Callback)(void *);

    QDeclarative1TimeLineCallback();
    QDeclarative1TimeLineCallback(QDeclarative1TimeLineObject *b, Callback, void * = 0);
    QDeclarative1TimeLineCallback(const QDeclarative1TimeLineCallback &o);

    QDeclarative1TimeLineCallback &operator=(const QDeclarative1TimeLineCallback &o);
    QDeclarative1TimeLineObject *callbackObject() const;

private:
    friend struct QDeclarative1TimeLinePrivate;
    Callback d0;
    void *d1;
    QDeclarative1TimeLineObject *d2;
};

template<class T>
class QDeclarative1TimeLineValueProxy : public QDeclarative1TimeLineValue
{
public:
    QDeclarative1TimeLineValueProxy(T *cls, void (T::*func)(qreal), qreal v = 0.)
    : QDeclarative1TimeLineValue(v), _class(cls), _setFunctionReal(func), _setFunctionInt(0)
    {
        Q_ASSERT(_class);
    }

    QDeclarative1TimeLineValueProxy(T *cls, void (T::*func)(int), qreal v = 0.)
    : QDeclarative1TimeLineValue(v), _class(cls), _setFunctionReal(0), _setFunctionInt(func)
    {
        Q_ASSERT(_class);
    }

    virtual void setValue(qreal v)
    {
        QDeclarative1TimeLineValue::setValue(v);
        if (_setFunctionReal) (_class->*_setFunctionReal)(v);
        else if (_setFunctionInt) (_class->*_setFunctionInt)((int)v);
    }

private:
    T *_class;
    void (T::*_setFunctionReal)(qreal);
    void (T::*_setFunctionInt)(int);
};

QT_END_NAMESPACE

#endif

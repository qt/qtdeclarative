/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QRECURSIONWATCHER_P_H
#define QRECURSIONWATCHER_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QRecursionNode;
class QRecursionNode {
public:
    inline QRecursionNode();
    bool *_r;
};

template<class T, QRecursionNode T::*Node>
class QRecursionWatcher {
public:
    inline QRecursionWatcher(T *);
    inline ~QRecursionWatcher();
    inline bool hasRecursed() const;
private:
    T *_t;
    bool _r;
};

QRecursionNode::QRecursionNode()
: _r(nullptr)
{
}

template<class T, QRecursionNode T::*Node>
QRecursionWatcher<T, Node>::QRecursionWatcher(T *t)
: _t(t), _r(false)
{
    if ((_t->*Node)._r) *(_t->*Node)._r = true;
    (_t->*Node)._r = &_r;
}

template<class T, QRecursionNode T::*Node>
QRecursionWatcher<T, Node>::~QRecursionWatcher()
{
    if ((_t->*Node)._r == &_r) (_t->*Node)._r = nullptr;
}

template<class T, QRecursionNode T::*Node>
bool QRecursionWatcher<T, Node>::hasRecursed() const
{
    return _r;
}

QT_END_NAMESPACE

#endif // QRECURSIONWATCHER_P_H

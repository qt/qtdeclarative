/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLABSTRACTBINDING_P_H
#define QQMLABSTRACTBINDING_P_H

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

#include <QtCore/qsharedpointer.h>
#include <private/qtqmlglobal_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qpointervaluepair_p.h>

QT_BEGIN_NAMESPACE

class QQmlObjectCreator;

class Q_QML_PRIVATE_EXPORT QQmlAbstractBinding
{
public:
    typedef QWeakPointer<QQmlAbstractBinding> Pointer;

    void destroy() {
        removeFromObject();
        clear();
        delete this;
    }

    virtual QString expression() const;

    virtual bool isValueTypeProxy() const;

    // Should return the encoded property index for the binding.  Should return this value
    // even if the binding is not enabled or added to an object.
    // Encoding is:  coreIndex | (valueTypeIndex << 16)
    int targetPropertyIndex() const { return m_targetIndex; }

    // Should return the object for the binding.  Should return this object even if the
    // binding is not enabled or added to the object.
    QObject *targetObject() const { return m_target.data(); }

    virtual void setEnabled(bool e, QQmlPropertyPrivate::WriteFlags f = QQmlPropertyPrivate::DontRemoveBinding) = 0;

    void addToObject();
    void removeFromObject();

    static inline Pointer getPointer(QQmlAbstractBinding *p);
    static void printBindingLoopError(QQmlProperty &prop);

    inline QQmlAbstractBinding *nextBinding() const;

protected:
    QQmlAbstractBinding();
    virtual ~QQmlAbstractBinding();
    void clear();

private:
    friend class QQmlData;
    friend class QQmlValueTypeProxyBinding;
    friend class QQmlObjectCreator;

    Pointer weakPointer();

    typedef QSharedPointer<QQmlAbstractBinding> SharedPointer;
    // To save memory, we also store the rarely used weakPointer() instance in here
    // We also use the flag bits:
    //    m_mePtr.flag1: added to object
    QPointerValuePair<QQmlAbstractBinding*, SharedPointer> m_mePtr;

    inline void setAddedToObject(bool v);
    inline bool isAddedToObject() const;

    inline void setNextBinding(QQmlAbstractBinding *);

    // Pointer to the next binding in the linked list of bindings.
    QQmlAbstractBinding *m_nextBinding;

protected:
    QFlagPointer<QObject> m_target;
    int m_targetIndex;
};

QQmlAbstractBinding::Pointer
QQmlAbstractBinding::getPointer(QQmlAbstractBinding *p)
{
    return p ? p->weakPointer() : Pointer();
}

void QQmlAbstractBinding::setAddedToObject(bool v)
{
    m_mePtr.setFlagValue(v);
}

bool QQmlAbstractBinding::isAddedToObject() const
{
    return m_mePtr.flag();
}

QQmlAbstractBinding *QQmlAbstractBinding::nextBinding() const
{
    return m_nextBinding;
}

void QQmlAbstractBinding::setNextBinding(QQmlAbstractBinding *b)
{
    m_nextBinding = b;
}

QT_END_NAMESPACE

#endif // QQMLABSTRACTBINDING_P_H

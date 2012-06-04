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

#ifndef QQMLPROPERTY_P_H
#define QQMLPROPERTY_P_H

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

#include "qqmlproperty.h"
#include "qqmlengine.h"

#include <private/qobject_p.h>
#include <private/qtqmlglobal_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlguard_p.h>
#include <private/qqmlboundsignalexpressionpointer_p.h>

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlEnginePrivate;
class QQmlJavaScriptExpression;
class Q_QML_PRIVATE_EXPORT QQmlPropertyPrivate : public QQmlRefCount
{
public:
    enum WriteFlag {
        BypassInterceptor = 0x01,
        DontRemoveBinding = 0x02,
        RemoveBindingOnAliasWrite = 0x04
    };
    Q_DECLARE_FLAGS(WriteFlags, WriteFlag)

    QQmlContextData *context;
    QQmlGuard<QQmlEngine> engine;
    QQmlGuard<QObject> object;

    QQmlPropertyData core;

    bool isNameCached:1;
    QString nameCache;

    QQmlPropertyPrivate();

    inline QQmlContextData *effectiveContext() const;

    void initProperty(QObject *obj, const QString &name);
    void initDefault(QObject *obj);

    bool isValueType() const;
    int propertyType() const;
    QQmlProperty::Type type() const;
    QQmlProperty::PropertyTypeCategory propertyTypeCategory() const;

    QVariant readValueProperty();
    bool writeValueProperty(const QVariant &, WriteFlags);

    static QQmlMetaObject rawMetaObjectForType(QQmlEnginePrivate *, int);
    static bool writeEnumProperty(const QMetaProperty &prop, int idx, QObject *object, 
                                  const QVariant &value, int flags);
    static bool writeValueProperty(QObject *, QQmlEngine *,
                                   const QQmlPropertyData &,
                                   const QVariant &, QQmlContextData *, 
                                   WriteFlags flags = 0);
    static bool write(QObject *, const QQmlPropertyData &, const QVariant &,
                      QQmlContextData *, WriteFlags flags = 0);
    static void findAliasTarget(QObject *, int, QObject **, int *);

    static QQmlAbstractBinding *setBinding(QObject *, int coreIndex,
                                                   int valueTypeIndex /* -1 */,
                                                   QQmlAbstractBinding *,
                                                   WriteFlags flags = DontRemoveBinding);
    static QQmlAbstractBinding *setBindingNoEnable(QObject *, int coreIndex,
                                                           int valueTypeIndex /* -1 */,
                                                           QQmlAbstractBinding *);
    static QQmlAbstractBinding *activateSharedBinding(QQmlContextData *context,
                                                      int sharedIdx, WriteFlags flags);
    static QQmlAbstractBinding *binding(QObject *, int coreIndex,
                                                int valueTypeIndex /* -1 */);

    static QQmlPropertyData saveValueType(const QQmlPropertyData &,
                                          const QMetaObject *, int,
                                          QQmlEngine *);
    static QQmlProperty restore(QObject *,
                                        const QQmlPropertyData &,
                                        QQmlContextData *);

    int signalIndex() const;

    static inline QQmlPropertyPrivate *get(const QQmlProperty &p) {
        return p.d;
    }

    // "Public" (to QML) methods
    static QQmlAbstractBinding *binding(const QQmlProperty &that);
    static QQmlAbstractBinding *setBinding(const QQmlProperty &that,
                                                   QQmlAbstractBinding *,
                                                   WriteFlags flags = DontRemoveBinding);
    static QQmlBoundSignalExpression *signalExpression(const QQmlProperty &that);
    static QQmlBoundSignalExpressionPointer setSignalExpression(const QQmlProperty &that,
                                                                QQmlBoundSignalExpression *);
    static QQmlBoundSignalExpressionPointer takeSignalExpression(const QQmlProperty &that,
                                                                 QQmlBoundSignalExpression *);
    static bool write(const QQmlProperty &that, const QVariant &, WriteFlags);
    static bool writeBinding(QObject *, const QQmlPropertyData &,
                             QQmlContextData *context,
                             QQmlJavaScriptExpression *expression, 
                             v8::Handle<v8::Value> result, bool isUndefined,
                             WriteFlags flags);
    static int valueTypeCoreIndex(const QQmlProperty &that);
    static int bindingIndex(const QQmlProperty &that);
    static int bindingIndex(const QQmlPropertyData &that);
    static QMetaMethod findSignalByName(const QMetaObject *mo, const QByteArray &);
    static bool connect(const QObject *sender, int signal_index,
                        const QObject *receiver, int method_index,
                        int type = 0, int *types = 0);
    static void flushSignal(const QObject *sender, int signal_index);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlPropertyPrivate::WriteFlags)

QT_END_NAMESPACE

#endif // QQMLPROPERTY_P_H

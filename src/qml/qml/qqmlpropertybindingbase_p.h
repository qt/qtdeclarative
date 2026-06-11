// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYBINDINGBASE_P_H
#define QQMLPROPERTYBINDINGBASE_P_H

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

#include <private/qproperty_p.h>
#include <private/qqmlpropertyindex_p.h>

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyBindingBase : public QPropertyBindingPrivate
{
public:
    enum class HasBoundFunction : bool {
        No = false,
        Yes = true,
    };

    enum class IsUndefined : bool {
        No = false,
        Yes = true,
    };

    enum class BindingKind : quint8 {
        JavaScript = 0,
        PropertyToProperty = 1,
    };

    BindingKind bindingKind() const { return targetData()->bindingKind; }

    bool isUndefined() const { return bool(targetData()->isUndefined); }

protected:
    QQmlPropertyBindingBase(QObject *target, QQmlPropertyIndex targetIndex,
                            QMetaType targetMetaType,
                            const QtPrivate::BindingFunctionVTable *vtable, BindingKind bindingKind,
                            HasBoundFunction hasBoundFunction = HasBoundFunction::No)
        : QPropertyBindingPrivate(targetMetaType, vtable, {}, true)
    {
        static_assert(std::is_trivially_destructible_v<TargetData>);
        static_assert(sizeof(TargetData) + sizeof(DeclarativeErrorCallback)
                      <= sizeof(QPropertyBindingSourceLocation));
        static_assert(alignof(TargetData) <= alignof(QPropertyBindingSourceLocation));
        new (&declarativeExtraData)
                TargetData{ target, targetIndex, hasBoundFunction, IsUndefined::No, bindingKind };
    }

    QObject *target() const { return targetData()->target; }

    QQmlPropertyIndex targetIndex() const { return targetData()->targetIndex; }

    bool hasBoundFunction() const
    {
        return targetData()->hasBoundFunction == HasBoundFunction::Yes;
    }

    void setIsUndefined(bool isUndefined)
    {
        targetData()->isUndefined = isUndefined ? IsUndefined::Yes : IsUndefined::No;
    }

private:
    struct TargetData
    {
        QObject *target = nullptr;
        QQmlPropertyIndex targetIndex;
        HasBoundFunction hasBoundFunction = HasBoundFunction::No;
        IsUndefined isUndefined = IsUndefined::No;
        BindingKind bindingKind = BindingKind::JavaScript;
    };

    static constexpr size_t kindOffset() { return sizeof(declarativeExtraData) - 2; }

    TargetData *targetData()
    {
        return std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData));
    }

    const TargetData *targetData() const
    {
        return std::launder(reinterpret_cast<const TargetData *>(&declarativeExtraData));
    }
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYBINDINGBASE_P_H

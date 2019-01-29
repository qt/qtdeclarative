/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLPROPERTYRAWDATA_P_H
#define QQMLPROPERTYRAWDATA_P_H

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

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

// We have this somewhat awful split between RawData and Data so that RawData can be
// used in unions.  In normal code, you should always use Data which initializes RawData
// to an invalid state on construction.
// ### We should be able to remove this split nowadays
class QQmlPropertyCacheMethodArguments;
class QQmlPropertyRawData
{
public:
    typedef QObjectPrivate::StaticMetaCallFunction StaticMetaCallFunction;

    struct Flags {
        enum Types {
            OtherType            = 0,
            FunctionType         = 1, // Is an invokable
            QObjectDerivedType   = 2, // Property type is a QObject* derived type
            EnumType             = 3, // Property type is an enum
            QListType            = 4, // Property type is a QML list
            QmlBindingType       = 5, // Property type is a QQmlBinding*
            QJSValueType         = 6, // Property type is a QScriptValue
            V4HandleType         = 7, // Property type is a QQmlV4Handle
            VarPropertyType      = 8, // Property type is a "var" property of VMEMO
            QVariantType         = 9  // Property is a QVariant
        };

        // The _otherBits (which "pad" the Flags struct to align it nicely) are used
        // to store the relative property index. It will only get used when said index fits. See
        // trySetStaticMetaCallFunction for details.
        // (Note: this padding is done here, because certain compilers have surprising behavior
        // when an enum is declared in-between two bit fields.)
        enum { BitsLeftInFlags = 10 };
        unsigned _otherBits       : BitsLeftInFlags; // align to 32 bits

        // Can apply to all properties, except IsFunction
        unsigned isConstant       : 1; // Has CONST flag
        unsigned isWritable       : 1; // Has WRITE function
        unsigned isResettable     : 1; // Has RESET function
        unsigned isAlias          : 1; // Is a QML alias to another property
        unsigned isFinal          : 1; // Has FINAL flag
        unsigned isOverridden     : 1; // Is overridden by a extension property
        unsigned isDirect         : 1; // Exists on a C++ QMetaObject

        unsigned type             : 4; // stores an entry of Types

        // Apply only to IsFunctions
        unsigned isVMEFunction    : 1; // Function was added by QML
        unsigned hasArguments     : 1; // Function takes arguments
        unsigned isSignal         : 1; // Function is a signal
        unsigned isVMESignal      : 1; // Signal was added by QML
        unsigned isV4Function     : 1; // Function takes QQmlV4Function* args
        unsigned isSignalHandler  : 1; // Function is a signal handler
        unsigned isOverload       : 1; // Function is an overload of another function
        unsigned isCloned         : 1; // The function was marked as cloned
        unsigned isConstructor    : 1; // The function was marked is a constructor

        // Internal QQmlPropertyCache flags
        unsigned notFullyResolved : 1; // True if the type data is to be lazily resolved
        unsigned overrideIndexIsProperty: 1;

        inline Flags();
        inline bool operator==(const Flags &other) const;
        inline void copyPropertyTypeFlags(Flags from);
    };

    Flags flags() const { return _flags; }
    void setFlags(Flags f)
    {
        unsigned otherBits = _flags._otherBits;
        _flags = f;
        _flags._otherBits = otherBits;
    }

    bool isValid() const { return coreIndex() != -1; }

    bool isConstant() const { return _flags.isConstant; }
    bool isWritable() const { return _flags.isWritable; }
    void setWritable(bool onoff) { _flags.isWritable = onoff; }
    bool isResettable() const { return _flags.isResettable; }
    bool isAlias() const { return _flags.isAlias; }
    bool isFinal() const { return _flags.isFinal; }
    bool isOverridden() const { return _flags.isOverridden; }
    bool isDirect() const { return _flags.isDirect; }
    bool hasStaticMetaCallFunction() const { return staticMetaCallFunction() != nullptr; }
    bool isFunction() const { return _flags.type == Flags::FunctionType; }
    bool isQObject() const { return _flags.type == Flags::QObjectDerivedType; }
    bool isEnum() const { return _flags.type == Flags::EnumType; }
    bool isQList() const { return _flags.type == Flags::QListType; }
    bool isQmlBinding() const { return _flags.type == Flags::QmlBindingType; }
    bool isQJSValue() const { return _flags.type == Flags::QJSValueType; }
    bool isV4Handle() const { return _flags.type == Flags::V4HandleType; }
    bool isVarProperty() const { return _flags.type == Flags::VarPropertyType; }
    bool isQVariant() const { return _flags.type == Flags::QVariantType; }
    bool isVMEFunction() const { return _flags.isVMEFunction; }
    bool hasArguments() const { return _flags.hasArguments; }
    bool isSignal() const { return _flags.isSignal; }
    bool isVMESignal() const { return _flags.isVMESignal; }
    bool isV4Function() const { return _flags.isV4Function; }
    bool isSignalHandler() const { return _flags.isSignalHandler; }
    bool isOverload() const { return _flags.isOverload; }
    void setOverload(bool onoff) { _flags.isOverload = onoff; }
    bool isCloned() const { return _flags.isCloned; }
    bool isConstructor() const { return _flags.isConstructor; }

    bool hasOverride() const { return overrideIndex() >= 0; }
    bool hasRevision() const { return revision() != 0; }

    bool isFullyResolved() const { return !_flags.notFullyResolved; }

    int propType() const { Q_ASSERT(isFullyResolved()); return _propType; }
    void setPropType(int pt)
    {
        Q_ASSERT(pt >= 0);
        Q_ASSERT(pt <= std::numeric_limits<qint16>::max());
        _propType = quint16(pt);
    }

    int notifyIndex() const { return _notifyIndex; }
    void setNotifyIndex(int idx)
    {
        Q_ASSERT(idx >= std::numeric_limits<qint16>::min());
        Q_ASSERT(idx <= std::numeric_limits<qint16>::max());
        _notifyIndex = qint16(idx);
    }

    bool overrideIndexIsProperty() const { return _flags.overrideIndexIsProperty; }
    void setOverrideIndexIsProperty(bool onoff) { _flags.overrideIndexIsProperty = onoff; }

    int overrideIndex() const { return _overrideIndex; }
    void setOverrideIndex(int idx)
    {
        Q_ASSERT(idx >= std::numeric_limits<qint16>::min());
        Q_ASSERT(idx <= std::numeric_limits<qint16>::max());
        _overrideIndex = qint16(idx);
    }

    int coreIndex() const { return _coreIndex; }
    void setCoreIndex(int idx)
    {
        Q_ASSERT(idx >= std::numeric_limits<qint16>::min());
        Q_ASSERT(idx <= std::numeric_limits<qint16>::max());
        _coreIndex = qint16(idx);
    }

    quint8 revision() const { return _revision; }
    void setRevision(quint8 rev)
    {
        Q_ASSERT(rev <= std::numeric_limits<quint8>::max());
        _revision = quint8(rev);
    }

    /* If a property is a C++ type, then we store the minor
     * version of this type.
     * This is required to resolve property or signal revisions
     * if this property is used as a grouped property.
     *
     * Test.qml
     * property TextEdit someTextEdit: TextEdit {}
     *
     * Test {
     *   someTextEdit.preeditText: "test" //revision 7
     *   someTextEdit.onEditingFinished: console.log("test") //revision 6
     * }
     *
     * To determine if these properties with revisions are available we need
     * the minor version of TextEdit as imported in Test.qml.
     *
     */

    quint8 typeMinorVersion() const { return _typeMinorVersion; }
    void setTypeMinorVersion(quint8 rev)
    {
        Q_ASSERT(rev <= std::numeric_limits<quint8>::max());
        _typeMinorVersion = quint8(rev);
    }

    QQmlPropertyCacheMethodArguments *arguments() const { return _arguments; }
    void setArguments(QQmlPropertyCacheMethodArguments *args) { _arguments = args; }

    int metaObjectOffset() const { return _metaObjectOffset; }
    void setMetaObjectOffset(int off)
    {
        Q_ASSERT(off >= std::numeric_limits<qint16>::min());
        Q_ASSERT(off <= std::numeric_limits<qint16>::max());
        _metaObjectOffset = qint16(off);
    }

    StaticMetaCallFunction staticMetaCallFunction() const { return _staticMetaCallFunction; }
    void trySetStaticMetaCallFunction(StaticMetaCallFunction f, unsigned relativePropertyIndex)
    {
        if (relativePropertyIndex < (1 << Flags::BitsLeftInFlags) - 1) {
            _flags._otherBits = relativePropertyIndex;
            _staticMetaCallFunction = f;
        }
    }
    quint16 relativePropertyIndex() const { Q_ASSERT(hasStaticMetaCallFunction()); return _flags._otherBits; }

private:
    Flags _flags;
    qint16 _coreIndex = 0;
    quint16 _propType = 0;

    // The notify index is in the range returned by QObjectPrivate::signalIndex().
    // This is different from QMetaMethod::methodIndex().
    qint16 _notifyIndex = 0;
    qint16 _overrideIndex = 0;

    quint8 _revision = 0;
    quint8 _typeMinorVersion = 0;
    qint16 _metaObjectOffset = 0;

    QQmlPropertyCacheMethodArguments *_arguments = nullptr;
    StaticMetaCallFunction _staticMetaCallFunction = nullptr;

    friend class QQmlPropertyData;
    friend class QQmlPropertyCache;
};

#if QT_POINTER_SIZE == 4
    Q_STATIC_ASSERT(sizeof(QQmlPropertyRawData) == 24);
#else // QT_POINTER_SIZE == 8
    Q_STATIC_ASSERT(sizeof(QQmlPropertyRawData) == 32);
#endif

QQmlPropertyRawData::Flags::Flags()
    : _otherBits(0)
    , isConstant(false)
    , isWritable(false)
    , isResettable(false)
    , isAlias(false)
    , isFinal(false)
    , isOverridden(false)
    , isDirect(false)
    , type(OtherType)
    , isVMEFunction(false)
    , hasArguments(false)
    , isSignal(false)
    , isVMESignal(false)
    , isV4Function(false)
    , isSignalHandler(false)
    , isOverload(false)
    , isCloned(false)
    , isConstructor(false)
    , notFullyResolved(false)
    , overrideIndexIsProperty(false)
{}

bool QQmlPropertyRawData::Flags::operator==(const QQmlPropertyRawData::Flags &other) const
{
    return isConstant == other.isConstant &&
            isWritable == other.isWritable &&
            isResettable == other.isResettable &&
            isAlias == other.isAlias &&
            isFinal == other.isFinal &&
            isOverridden == other.isOverridden &&
            type == other.type &&
            isVMEFunction == other.isVMEFunction &&
            hasArguments == other.hasArguments &&
            isSignal == other.isSignal &&
            isVMESignal == other.isVMESignal &&
            isV4Function == other.isV4Function &&
            isSignalHandler == other.isSignalHandler &&
            isOverload == other.isOverload &&
            isCloned == other.isCloned &&
            isConstructor == other.isConstructor &&
            notFullyResolved == other.notFullyResolved &&
            overrideIndexIsProperty == other.overrideIndexIsProperty;
}

void QQmlPropertyRawData::Flags::copyPropertyTypeFlags(QQmlPropertyRawData::Flags from)
{
    switch (from.type) {
    case QObjectDerivedType:
    case EnumType:
    case QListType:
    case QmlBindingType:
    case QJSValueType:
    case V4HandleType:
    case QVariantType:
        type = from.type;
    }
}

QT_END_NAMESPACE

#endif // QQMLPROPERTYRAWDATA_P_H

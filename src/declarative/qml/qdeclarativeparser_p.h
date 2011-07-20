/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVEPARSER_P_H
#define QDECLARATIVEPARSER_P_H

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

#include "qdeclarative.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

#include <private/qobject_p.h>
#include <private/qdeclarativerefcount_p.h>
#include <private/qdeclarativeglobal_p.h>
#include <private/qdeclarativepool_p.h>
#include <private/qfieldlist_p.h>
#include <private/qdeclarativepropertycache_p.h>
#include <private/qfastmetabuilder_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativePropertyCache;
namespace QDeclarativeJS { namespace AST { class Node; class StringLiteral; } }
namespace QDeclarativeCompilerTypes { class BindingReference; class ComponentCompileState; }

/*
    XXX

    These types are created (and owned) by the QDeclarativeScriptParser and consumed by the 
    QDeclarativeCompiler.  During the compilation phase the compiler will update some of
    the fields for its own use.

    The types are part of the generic sounding "QDeclarativeParser" namespace for legacy 
    reasons (there used to be more in this namespace) and will be cleaned up and
    migrated into a more appropriate location eventually.
*/
namespace QDeclarativeParser
{
    struct Location 
    {
        Location() : line(-1), column(-1) {}
        int line;
        int column;

        inline bool operator<(const Location &other) {
            return line < other.line || 
                   (line == other.line && column < other.column);
        }
    };

    struct LocationRange
    {
        LocationRange() : offset(0), length(0) {}
        quint32 offset;
        quint32 length;
    };

    struct LocationSpan
    {
        Location start;
        Location end;
        LocationRange range;

        bool operator<(LocationSpan &o) const {
            return (start.line < o.start.line) ||
                   (start.line == o.start.line && start.column < o.start.column);
        }
    };

    class Object;
    class Property;
    class Q_DECLARATIVE_EXPORT Variant 
    {
    public:
        enum Type {
            Invalid,
            Boolean,
            Number,
            String,
            Script
        };

        Variant();
        Variant(const Variant &);
        explicit Variant(bool);
        explicit Variant(double, const QStringRef &asWritten = QStringRef());
        explicit Variant(QDeclarativeJS::AST::StringLiteral *);
        explicit Variant(const QStringRef &asWritten, QDeclarativeJS::AST::Node *);
        Variant &operator=(const Variant &);

        Type type() const;

        bool isBoolean() const { return type() == Boolean; }
        bool isNumber() const { return type() == Number; }
        bool isString() const { return type() == String; }
        bool isScript() const { return type() == Script; }
        bool isStringList() const;

        bool asBoolean() const;
        QString asString() const;
        double asNumber() const;
        QString asScript() const;
        QDeclarativeJS::AST::Node *asAST() const;
        QStringList asStringList() const;

    private:
        Type t;
        union {
            bool b;
            double d;
            QDeclarativeJS::AST::StringLiteral *l;
            QDeclarativeJS::AST::Node *n;
        };
        QStringRef asWritten;
    };

    class Value : public QDeclarativePool::POD
    {
    public:
        Value();

        enum Type {
            // The type of this value assignment is not yet known
            Unknown,
            // This is used as a literal property assignment
            Literal,
            // This is used as a property binding assignment
            PropertyBinding,
            // This is used as a QDeclarativePropertyValueSource assignment
            ValueSource,
            // This is used as a QDeclarativePropertyValueInterceptor assignment
            ValueInterceptor,
            // This is used as a property QObject assignment
            CreatedObject,
            // This is used as a signal object assignment
            SignalObject,
            // This is used as a signal expression assignment
            SignalExpression,
            // This is used as an id assignment only
            Id
        };
        Type type;

        // ### Temporary (for id only)
        QString primitive() const { return value.isString() ? value.asString() : value.asScript(); }

        // Primitive value
        Variant value;
        // Object value
        Object *object;

        LocationSpan location;

        // Used by compiler
        QDeclarativeCompilerTypes::BindingReference *bindingReference;
        int signalExpressionContextStack;

        // Used in Property::ValueList lists
        Value *nextValue;
    };

    class Property : public QDeclarativePool::POD
    {
    public:
        Property();

        // The Object to which this property is attached
        Object *parent;

        Object *getValue(const LocationSpan &);
        void addValue(Value *v);
        void addOnValue(Value *v);

        // The QVariant::Type of the property, or 0 (QVariant::Invalid) if 
        // unknown.
        int type;
        // The metaobject index of this property, or -1 if unknown.
        int index;
        // The core data in the case of a regular property.  
        // XXX This has to be a value now as the synthCache may change during
        // compilation which invalidates pointers.  We should fix this.
        QDeclarativePropertyCache::Data core;

        // Returns true if this is an empty property - both value and values
        // are unset.
        bool isEmpty() const;

        typedef QFieldList<Value, &Value::nextValue> ValueList;
        // The list of values assigned to this property.  Content in values
        // and value are mutually exclusive
        ValueList values;
        // The list of values assigned to this property using the "on" syntax
        ValueList onValues;
        // The accessed property.  This is used to represent dot properties.
        // Content in value and values are mutually exclusive.
        Object *value;
        // The property name
        QStringRef name() const { return _name; }
        void setName(const QString &n) { _name = QStringRef(pool()->NewString(n)); }
        // True if this property was accessed as the default property.  
        bool isDefault;
        // True if the setting of this property will be deferred.  Set by the
        // QDeclarativeCompiler
        bool isDeferred;
        // True if this property is a value-type pseudo-property
        bool isValueTypeSubProperty;
        // True if this property is a property alias.  Set by the 
        // QDeclarativeCompiler
        bool isAlias;

        // Used for scriptStringProperties
        int scriptStringScope;

        LocationSpan location;
        LocationRange listValueRange;

        // Used in Object::MainPropertyList
        Property *nextMainProperty;

        // Used in Object::PropertyList lists
        Property *nextProperty;

    private:
        friend class Object;
        QStringRef _name;
    };

    class Object : public QDeclarativePool::Class
    {
    public:
        Object();
        virtual ~Object(); 

        // Type of the object.  The integer is an index into the 
        // QDeclarativeCompiledData::types array, or -1 if the object is a property
        // group.
        int type;

        // The fully-qualified name of this type
        QByteArray typeName;
        // The id assigned to the object (if any).  Set by the QDeclarativeCompiler
        QString id;
        // The id index assigned to the object (if any).  Set by the QDeclarativeCompiler
        int idIndex;
        // Custom parsed data
        QByteArray custom;
        // Bit mask of the properties assigned bindings
        QByteArray bindingBitmask; 
        void setBindingBit(int);
        // Returns the metaobject for this type, or 0 if not available.  
        // Internally selectd between the metatype and extObject variables
        const QMetaObject *metaObject() const;

        // The compile time metaobject for this type
        const QMetaObject *metatype;
        // The synthesized metaobject, if QML added signals or properties to
        // this type.  Otherwise null
        QAbstractDynamicMetaObject extObject;
        QByteArray metadata; // Generated by compiler
        QByteArray synthdata; // Generated by compiler
        QDeclarativePropertyCache *synthCache; // Generated by compiler

        Property *getDefaultProperty();
        // name ptr must be guarenteed to remain valid
        Property *getProperty(const QStringRef &name, bool create=true);
        Property *getProperty(const QString &name, bool create=true);

        Property *defaultProperty;

        typedef QFieldList<Property, &Property::nextMainProperty> MainPropertyList;
        MainPropertyList properties;

        // Output of the compilation phase (these properties continue to exist
        // in either the defaultProperty or properties members too)
        void addValueProperty(Property *);
        void addSignalProperty(Property *);
        void addAttachedProperty(Property *);
        void addGroupedProperty(Property *);
        void addValueTypeProperty(Property *);
        void addScriptStringProperty(Property *);

        typedef QFieldList<Property, &Property::nextProperty> PropertyList;
        PropertyList valueProperties;
        PropertyList signalProperties;
        PropertyList attachedProperties;
        PropertyList groupedProperties;
        PropertyList valueTypeProperties;
        PropertyList scriptStringProperties;

        // Script blocks that were nested under this object
        struct ScriptBlock {
            enum Pragma { 
                None   = 0x00000000,
                Shared = 0x00000001
            };
            Q_DECLARE_FLAGS(Pragmas, Pragma)

            QString code;
            QString file;
            Pragmas pragmas;
        };

        // The bytes to cast instances by to get to the QDeclarativeParserStatus 
        // interface.  -1 indicates the type doesn't support this interface.
        // Set by the QDeclarativeCompiler.
        int parserStatusCast;

        LocationSpan location;

        struct DynamicProperty {
            DynamicProperty();
            DynamicProperty(const DynamicProperty &);

            enum Type { Variant, Int, Bool, Real, String, Url, Color, Time, Date, DateTime, Alias, Custom, CustomList };

            bool isDefaultProperty;
            Type type;
            QByteArray customType;
            QByteArray name;
            QDeclarativeParser::Property *defaultValue;
            LocationSpan location;

            // Used by the compiler
            QByteArray resolvedCustomTypeName;
            QFastMetaBuilder::StringRef typeRef;
            QFastMetaBuilder::StringRef nameRef;
            QFastMetaBuilder::StringRef changedSignatureRef;
        };
        struct DynamicSignal {
            DynamicSignal();
            DynamicSignal(const DynamicSignal &);

            QByteArray name;
            QList<QByteArray> parameterTypes;
            QList<QByteArray> parameterNames;

            int parameterTypesLength() const;
            int parameterNamesLength() const;

            // Used by the compiler
            QFastMetaBuilder::StringRef signatureRef;
            QFastMetaBuilder::StringRef parameterNamesRef;
        };
        struct DynamicSlot {
            DynamicSlot();
            DynamicSlot(const DynamicSlot &);

            QByteArray name;
            QString body;
            QList<QByteArray> parameterNames;
            LocationSpan location;

            int parameterNamesLength() const;

            // Used by the compiler
            QFastMetaBuilder::StringRef signatureRef;
            QFastMetaBuilder::StringRef parameterNamesRef;
        };

        // The list of dynamic properties
        QList<DynamicProperty> dynamicProperties;
        // The list of dynamic signals
        QList<DynamicSignal> dynamicSignals;
        // The list of dynamic slots
        QList<DynamicSlot> dynamicSlots;

        // Used by compiler
        QDeclarativeCompilerTypes::ComponentCompileState *componentCompileState;

        // Used by ComponentCompileState::AliasingObjectsList
        Object *nextAliasingObject;
        // Used by ComponentComppileState::IdList
        Object *nextIdObject;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QDeclarativeParser::Object::ScriptBlock::Pragmas);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDeclarativeParser::Variant)

QT_END_HEADER

#endif // QDECLARATIVEPARSER_P_H

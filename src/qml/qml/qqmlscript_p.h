/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#ifndef QQMLSCRIPT_P_H
#define QQMLSCRIPT_P_H

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

#include <QtQml/qqmlerror.h>

#include <private/qfieldlist_p.h>
#include <private/qhashfield_p.h>
#include <private/qqmlpool_p.h>
#include <private/qqmlpropertycache_p.h>

#include <QtCore/QList>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE


class QByteArray;
class QQmlPropertyCache;
namespace QQmlJS { namespace AST { class Node; class StringLiteral; } }
namespace QQmlCompilerTypes { struct BindingReference; struct ComponentCompileState; }

namespace QQmlScript {

struct Location 
{
    Location() : line(0), column(0) {}
    quint16 line;
    quint16 column;

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

class Import
{
public:
    Import() : type(Library), majorVersion(-1), minorVersion(-1) {}

    enum Type { Library, File, Script };
    Type type;

    QString uri;
    QString qualifier;

    int majorVersion;
    int minorVersion;

    QQmlScript::LocationSpan location;
};

class Object;
class TypeReference : public QQmlPool::Class
{
public:
    // type as it has been referenced in Qml
    QString name;
    // The first use of this type in the parse tree.  Useful for error locations.
    QQmlScript::Object *firstUse;
};

class Object;
class Property;

class Q_QML_PRIVATE_EXPORT Variant
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
    explicit Variant(QQmlJS::AST::StringLiteral *);
    explicit Variant(const QStringRef &asWritten, QQmlJS::AST::Node *);
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
    QQmlJS::AST::Node *asAST() const;
    QStringList asStringList() const;

private:
    Type t;
    union {
        bool b;
        double d;
        QQmlJS::AST::StringLiteral *l;
        QQmlJS::AST::Node *n;
    };
    QStringRef asWritten;
};

class Value : public QQmlPool::POD
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
        // This is used as a QQmlPropertyValueSource assignment
        ValueSource,
        // This is used as a QQmlPropertyValueInterceptor assignment
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
    union {
        QQmlCompilerTypes::BindingReference *bindingReference;
        int signalExpressionContextStack;
    };

    // Used in Property::ValueList lists
    Value *nextValue;
};

class Property : public QQmlPool::POD
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
    QQmlPropertyData core;

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
    const QHashedStringRef &name() const { return _name; }
    void setName(const QString &n) { _name = QHashedStringRef(pool()->NewString(n)); }
    void setName(const QHashedStringRef &n) { _name = n; }
    // True if this property was accessed as the default property.  
    bool isDefault;
    // True if the setting of this property will be deferred.  Set by the
    // QQmlCompiler
    bool isDeferred;
    // True if this property is a value-type pseudo-property
    bool isValueTypeSubProperty;
    // True if this property is a property alias.  Set by the 
    // QQmlCompiler
    bool isAlias;
    // True if this is a readonly property declaration
    bool isReadOnlyDeclaration;

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
    QHashedStringRef _name;
};

class Object : public QQmlPool::Class
{
public:
    Object();
    virtual ~Object(); 

    // Type of the object.  The integer is an index into the 
    // QQmlCompiledData::types array, or -1 if the object is a property
    // group.
    int type;
    // A back pointer to the QQmlScript::TypeReference for this type, if any.
    // Set by the parser.
    TypeReference *typeReference;

    // The id assigned to the object (if any).  Set by the QQmlCompiler
    QString id;
    // The id index assigned to the object (if any).  Set by the QQmlCompiler
    int idIndex;
    // Custom parsed data
    QByteArray custom;
    // Bit mask of the properties assigned bindings
    QByteArray bindingBitmask; 
    void setBindingBit(int);

    QQmlPropertyCache *metatype;

    // The synthesized metaobject, if QML added signals or properties to
    // this type.  Otherwise null
    QByteArray synthdata;  // Generated by compiler
    QQmlPropertyCache *synthCache; // Generated by compiler

    Property *getDefaultProperty();
    // name ptr must be guaranteed to remain valid
    Property *getProperty(const QHashedStringRef &name, bool create=true);
    Property *getProperty(const QStringRef &name, bool create=true);
    Property *getProperty(const QString &name, bool create=true);

    Property *defaultProperty;

    typedef QFieldList<Property, &Property::nextMainProperty> MainPropertyList;
    MainPropertyList properties;
    QHashField propertiesHashField;

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

    // The bytes to cast instances by to get to the QQmlParserStatus 
    // interface.  -1 indicates the type doesn't support this interface.
    // Set by the QQmlCompiler.
    int parserStatusCast;

    LocationSpan location;

    struct DynamicProperty : public QQmlPool::POD 
    {
        DynamicProperty();

        enum Type { Var, Variant, Int, Bool, Real, String, Url, Color,
                    Font, Time, Date, DateTime, Rect, Point, Size,
                    Vector2D, Vector3D, Vector4D, Matrix4x4, Quaternion,
                    Alias, Custom, CustomList };

        quint32 isDefaultProperty:1;
        quint32 isReadOnly:1;

        Type type;

        QHashedStringRef customType;
        QHashedStringRef name;
        QQmlScript::Property *defaultValue;
        LocationSpan location;
        Location nameLocation;

        // Used by Object::DynamicPropertyList
        DynamicProperty *nextProperty;

        // Used by the compiler
        int nameIndex; // Points at the name and name + "Changed()" strings
    };

    struct DynamicSignal : public QQmlPool::POD
    {
        DynamicSignal();

        QHashedStringRef name;
        QQmlPool::List<DynamicProperty::Type> parameterTypes;
        QQmlPool::List<QHashedStringRef> parameterTypeNames;
        QQmlPool::List<QHashedStringRef> parameterNames;

        // Used by Object::DynamicSignalList
        DynamicSignal *nextSignal;

        // Used by the compiler
        int nameIndex;
        LocationSpan location;
    };

    struct DynamicSlot : public QQmlPool::Class
    {
        DynamicSlot();

        QHashedStringRef name;
        QString body;
        QList<QByteArray> parameterNames;
        LocationSpan location;

        int parameterNamesLength() const;

        // Used by Object::DynamicSlotList
        DynamicSlot *nextSlot;

        // Used by the compiler
        int nameIndex;
    };

    // The list of dynamic properties
    typedef QFieldList<DynamicProperty, &DynamicProperty::nextProperty> DynamicPropertyList;
    DynamicPropertyList dynamicProperties;
    // The list of dynamic signals
    typedef QFieldList<DynamicSignal, &DynamicSignal::nextSignal> DynamicSignalList;
    DynamicSignalList dynamicSignals;
    // The list of dynamic slots
    typedef QFieldList<DynamicSlot, &DynamicSlot::nextSlot> DynamicSlotList;
    DynamicSlotList dynamicSlots;

    int aggregateDynamicSignalParameterCount() const;
    int aggregateDynamicSlotParameterCount() const;

    // Used by compiler
    QQmlCompilerTypes::ComponentCompileState *componentCompileState;

    // Used by ComponentCompileState::AliasingObjectsList
    Object *nextAliasingObject;
    // Used by ComponentComppileState::IdList
    Object *nextIdObject;
};

class ParserJsASTData;
class Q_QML_PRIVATE_EXPORT Parser
{
public:
    Parser();
    ~Parser();

    bool parse(const QString &data, const QByteArray &preparseData,
               const QUrl &url = QUrl(), const QString &urlString = QString());

    QByteArray preparseData() const;

    QList<TypeReference*> referencedTypes() const;

    QQmlScript::Object *tree() const;
    QList<Import> imports() const;

    void clear();

    QList<QQmlError> errors() const;

    class JavaScriptMetaData {
    public:
        JavaScriptMetaData() 
        : pragmas(QQmlScript::Object::ScriptBlock::None) {}

        QQmlScript::Object::ScriptBlock::Pragmas pragmas;
        QList<Import> imports;
    };

    static QQmlScript::Object::ScriptBlock::Pragmas extractPragmas(QString &);
    static JavaScriptMetaData extractMetaData(QString &, QQmlError *error);


// ### private:
    int findOrCreateTypeId(const QString &name, Object *);
    void setTree(QQmlScript::Object *tree);

    void setScriptFile(const QString &filename) {_scriptFile = filename; }
    QString scriptFile() const { return _scriptFile; }

// ### private:
    QList<QQmlError> _errors;

    QQmlPool _pool;
    QQmlScript::Object *root;
    QList<Import> _imports;
    QList<TypeReference*> _refTypes;
    QString _scriptFile;
    ParserJsASTData *data;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlScript::Object::ScriptBlock::Pragmas)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlScript::Variant)

#endif // QQMLSCRIPT_P_H

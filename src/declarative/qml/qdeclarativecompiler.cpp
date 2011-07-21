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

#include "private/qdeclarativecompiler_p.h"

#include "private/qdeclarativeparser_p.h"
#include "private/qdeclarativescriptparser_p.h"
#include "qdeclarativepropertyvaluesource.h"
#include "qdeclarativecomponent.h"
#include "private/qmetaobjectbuilder_p.h"
#include "private/qfastmetabuilder_p.h"
#include "private/qdeclarativestringconverters_p.h"
#include "private/qdeclarativeengine_p.h"
#include "qdeclarativeengine.h"
#include "qdeclarativecontext.h"
#include "private/qdeclarativemetatype_p.h"
#include "private/qdeclarativecustomparser_p_p.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativecomponent_p.h"
#include "parser/qdeclarativejsast_p.h"
#include "private/qdeclarativevmemetaobject_p.h"
#include "private/qdeclarativeexpression_p.h"
#include "private/qdeclarativeproperty_p.h"
#include "private/qdeclarativerewrite_p.h"
#include "qdeclarativescriptstring.h"
#include "private/qdeclarativeglobal_p.h"
#include "private/qdeclarativescriptparser_p.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativev4compiler_p.h"
#include "private/qdeclarativeutils_p.h"

#include <QColor>
#include <QDebug>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QAtomicInt>
#include <QtCore/qdebug.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(compilerDump, QML_COMPILER_DUMP);
DEFINE_BOOL_CONFIG_OPTION(compilerStatDump, QML_COMPILER_STATS);

using namespace QDeclarativeParser;
using namespace QDeclarativeCompilerTypes;

static QString id_string(QLatin1String("id"));
static QString on_string(QLatin1String("on"));
static QString Changed_string(QLatin1String("Changed"));
static QString Component_string(QLatin1String("Component"));

/*!
    Instantiate a new QDeclarativeCompiler.
*/
QDeclarativeCompiler::QDeclarativeCompiler(QDeclarativePool *pool)
: pool(pool), output(0), engine(0), unitRoot(0), unit(0), componentStats(0)
{
    if (compilerStatDump()) 
        componentStats = pool->New<ComponentStats>();
}

/*!
    Returns true if the last call to compile() caused errors.

    \sa errors()
*/
bool QDeclarativeCompiler::isError() const
{
    return !exceptions.isEmpty();
}

/*!
    Return the list of errors from the last call to compile(), or an empty list
    if there were no errors.
*/
QList<QDeclarativeError> QDeclarativeCompiler::errors() const
{
    return exceptions;
}

/*!
    Returns true if \a name refers to an attached property, false otherwise.

    Attached property names are those that start with a capital letter.
*/
bool QDeclarativeCompiler::isAttachedPropertyName(const QString &name)
{
    return isAttachedPropertyName(QHashedStringRef(&name));
}

bool QDeclarativeCompiler::isAttachedPropertyName(const QHashedStringRef &name)
{
    return !name.isEmpty() && QDeclarativeUtils::isUpper(name.at(0));
}

/*!
    Returns true if \a name refers to a signal property, false otherwise.

    Signal property names are those that start with "on", followed by a first
    character which is either a capital letter or one or more underscores followed
    by a capital letter, which is then followed by other allowed characters.

    Note that although ECMA-262r3 supports dollarsigns and escaped unicode
    character codes in property names, for simplicity and performance reasons
    QML only supports letters, numbers and underscores.
*/
bool QDeclarativeCompiler::isSignalPropertyName(const QString &name)
{
    return isSignalPropertyName(QStringRef(&name));
}

bool QDeclarativeCompiler::isSignalPropertyName(const QHashedStringRef &name)
{
    if (name.length() < 3) return false;
    if (!name.startsWith(on_string)) return false;
    int ns = name.length();
    for (int i = 2; i < ns; ++i) {
        const QChar curr = name.at(i);
        if (curr.unicode() == '_') continue;
        if (QDeclarativeUtils::isUpper(curr)) return true;
        return false;
    }
    return false; // consists solely of underscores - invalid.
}

/*!
    \macro COMPILE_EXCEPTION
    \internal
    Inserts an error into the QDeclarativeCompiler error list, and returns false
    (failure).

    \a token is used to source the error line and column, and \a desc is the
    error itself.  \a desc can be an expression that can be piped into QDebug.

    For example:

    \code
    COMPILE_EXCEPTION(property, tr("Error for property \"%1\"").arg(property->name));
    \endcode
*/
#define COMPILE_EXCEPTION(token, desc) \
    {  \
        QString exceptionDescription; \
        QDeclarativeError error; \
        error.setUrl(output->url); \
        error.setLine((token)->location.start.line); \
        error.setColumn((token)->location.start.column); \
        error.setDescription(desc.trimmed()); \
        exceptions << error; \
        return false; \
    }

/*!
    \macro COMPILE_CHECK
    \internal
    Returns false if \a is false, otherwise does nothing.
*/
#define COMPILE_CHECK(a) \
    { \
        if (!a) return false; \
    }

/*!
    Returns true if literal \a v can be assigned to property \a prop, otherwise
    false.

    This test corresponds to action taken by genLiteralAssignment().  Any change
    made here, must have a corresponding action in genLiteralAssigment().
*/
bool QDeclarativeCompiler::testLiteralAssignment(QDeclarativeParser::Property *prop,
                                                 QDeclarativeParser::Value *v)
{
    const QDeclarativeParser::Variant &value = v->value;

    if (!prop->core.isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(prop->name().toString()));

    if (prop->core.isEnum()) {
        QMetaProperty p = prop->parent->metaObject()->property(prop->index);
        int enumValue;
        if (p.isFlagType()) {
            enumValue = p.enumerator().keysToValue(value.asString().toUtf8().constData());
        } else
            enumValue = p.enumerator().keyToValue(value.asString().toUtf8().constData());

        if (enumValue == -1)
            COMPILE_EXCEPTION(v, tr("Invalid property assignment: unknown enumeration"));

        v->value = QDeclarativeParser::Variant((double)enumValue);
        return true;
    }

    int type = prop->type;

    switch(type) {
        case QMetaType::QVariant:
            break;
        case QVariant::String:
            if (!v->value.isString()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: string expected"));
            break;
        case QVariant::ByteArray:
            if (!v->value.isString()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: byte array expected"));
            break;
        case QVariant::Url:
            if (!v->value.isString()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: url expected"));
            break;
        case QVariant::UInt:
            {
            bool ok = v->value.isNumber();
            if (ok) {
                double n = v->value.asNumber();
                if (double(uint(n)) != n)
                    ok = false;
            }
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: unsigned int expected"));
            }
            break;
        case QVariant::Int:
            {
            bool ok = v->value.isNumber();
            if (ok) {
                double n = v->value.asNumber();
                if (double(int(n)) != n)
                    ok = false;
            }
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: int expected"));
            }
            break;
        case QMetaType::Float:
            if (!v->value.isNumber()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: number expected"));
            break;
        case QVariant::Double:
            if (!v->value.isNumber()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: number expected"));
            break;
        case QVariant::Color:
            {
            bool ok;
            QDeclarativeStringConverters::colorFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: color expected"));
            }
            break;
#ifndef QT_NO_DATESTRING
        case QVariant::Date:
            {
            bool ok;
            QDeclarativeStringConverters::dateFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: date expected"));
            }
            break;
        case QVariant::Time:
            {
            bool ok;
            QDeclarativeStringConverters::timeFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: time expected"));
            }
            break;
        case QVariant::DateTime:
            {
            bool ok;
            QDeclarativeStringConverters::dateTimeFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: datetime expected"));
            }
            break;
#endif // QT_NO_DATESTRING
        case QVariant::Point:
        case QVariant::PointF:
            {
            bool ok;
            QPointF point = QDeclarativeStringConverters::pointFFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: point expected"));
            }
            break;
        case QVariant::Size:
        case QVariant::SizeF:
            {
            bool ok;
            QSizeF size = QDeclarativeStringConverters::sizeFFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: size expected"));
            }
            break;
        case QVariant::Rect:
        case QVariant::RectF:
            {
            bool ok;
            QRectF rect = QDeclarativeStringConverters::rectFFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: rect expected"));
            }
            break;
        case QVariant::Bool:
            {
            if (!v->value.isBoolean()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: boolean expected"));
            }
            break;
        case QVariant::Vector3D:
            {
            bool ok;
            QDeclarativeStringConverters::vector3DFromString(value.asString(), &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: 3D vector expected"));
            }
            break;
        case QVariant::Vector4D:
            {
            bool ok;
            QDeclarativeStringConverters::vector4DFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: 4D vector expected"));
            }
            break;
        default:
            {
            QDeclarativeMetaType::StringConverter converter = QDeclarativeMetaType::customStringConverter(type);
            if (!converter)
                COMPILE_EXCEPTION(v, tr("Invalid property assignment: unsupported type \"%1\"").arg(QString::fromLatin1(QVariant::typeToName((QVariant::Type)type))));
            }
            break;
    }
    return true;
}

/*!
    Generate a store instruction for assigning literal \a v to property \a prop.

    Any literal assignment that is approved in testLiteralAssignment() must have
    a corresponding action in this method.
*/
void QDeclarativeCompiler::genLiteralAssignment(QDeclarativeParser::Property *prop,
                                                QDeclarativeParser::Value *v)
{
    QDeclarativeInstruction instr;

    if (prop->core.isEnum()) {
        Q_ASSERT(v->value.isNumber());
        // Preresolved value
        int value = (int)v->value.asNumber();

        instr.setType(QDeclarativeInstruction::StoreInteger);
        instr.storeInteger.propertyIndex = prop->index;
        instr.storeInteger.value = value;
        output->addInstruction(instr);
        return;
    }

    int type = prop->type;
    switch(type) {
        case QMetaType::QVariant:
            {
            if (v->value.isNumber()) {
                double n = v->value.asNumber();
                if (double(int(n)) == n) {
                    instr.setType(QDeclarativeInstruction::StoreVariantInteger);
                    instr.storeInteger.propertyIndex = prop->index;
                    instr.storeInteger.value = int(n);
                } else {
                    instr.setType(QDeclarativeInstruction::StoreVariantDouble);
                    instr.storeDouble.propertyIndex = prop->index;
                    instr.storeDouble.value = n;
                }
            } else if(v->value.isBoolean()) {
                instr.setType(QDeclarativeInstruction::StoreVariantBool);
                instr.storeBool.propertyIndex = prop->index;
                instr.storeBool.value = v->value.asBoolean();
            } else {
                instr.setType(QDeclarativeInstruction::StoreVariant);
                instr.storeString.propertyIndex = prop->index;
                instr.storeString.value = output->indexForString(v->value.asString());
            }
            }
            break;
        case QVariant::String:
            {
            instr.setType(QDeclarativeInstruction::StoreString);
            instr.storeString.propertyIndex = prop->index;
            instr.storeString.value = output->indexForString(v->value.asString());
            }
            break;
        case QVariant::ByteArray:
            {
            instr.setType(QDeclarativeInstruction::StoreByteArray);
            instr.storeByteArray.propertyIndex = prop->index;
            instr.storeByteArray.value = output->indexForByteArray(v->value.asString().toLatin1());
            }
            break;
        case QVariant::Url:
            {
            instr.setType(QDeclarativeInstruction::StoreUrl);
            QString string = v->value.asString();
            QUrl u = string.isEmpty() ? QUrl() : output->url.resolved(QUrl(string));
            instr.storeUrl.propertyIndex = prop->index;
            instr.storeUrl.value = output->indexForUrl(u);
            }
            break;
        case QVariant::UInt:
            {
            instr.setType(QDeclarativeInstruction::StoreInteger);
            instr.storeInteger.propertyIndex = prop->index;
            instr.storeInteger.value = uint(v->value.asNumber());
            }
            break;
        case QVariant::Int:
            {
            instr.setType(QDeclarativeInstruction::StoreInteger);
            instr.storeInteger.propertyIndex = prop->index;
            instr.storeInteger.value = int(v->value.asNumber());
            }
            break;
        case QMetaType::Float:
            {
            instr.setType(QDeclarativeInstruction::StoreFloat);
            instr.storeFloat.propertyIndex = prop->index;
            instr.storeFloat.value = float(v->value.asNumber());
            }
            break;
        case QVariant::Double:
            {
            instr.setType(QDeclarativeInstruction::StoreDouble);
            instr.storeDouble.propertyIndex = prop->index;
            instr.storeDouble.value = v->value.asNumber();
            }
            break;
        case QVariant::Color:
            {
            QColor c = QDeclarativeStringConverters::colorFromString(v->value.asString());
            instr.setType(QDeclarativeInstruction::StoreColor);
            instr.storeColor.propertyIndex = prop->index;
            instr.storeColor.value = c.rgba();
            }
            break;
#ifndef QT_NO_DATESTRING
        case QVariant::Date:
            {
            QDate d = QDeclarativeStringConverters::dateFromString(v->value.asString());
            instr.setType(QDeclarativeInstruction::StoreDate);
            instr.storeDate.propertyIndex = prop->index;
            instr.storeDate.value = d.toJulianDay();
            }
            break;
        case QVariant::Time:
            {
            QTime time = QDeclarativeStringConverters::timeFromString(v->value.asString());
            instr.setType(QDeclarativeInstruction::StoreTime);
            instr.storeTime.propertyIndex = prop->index;
            Q_ASSERT(sizeof(instr.storeTime.time) == sizeof(QTime));
            ::memcpy(&instr.storeTime.time, &time, sizeof(QTime));            }
            }
            break;
        case QVariant::DateTime:
            {
            QDateTime dateTime = QDeclarativeStringConverters::dateTimeFromString(v->value.asString());
            QTime time = dateTime.time();
            instr.setType(QDeclarativeInstruction::StoreDateTime);
            instr.storeDateTime.propertyIndex = prop->index;
            instr.storeDateTime.date = dateTime.date().toJulianDay();
            Q_ASSERT(sizeof(instr.storeDateTime.time) == sizeof(QTime));
            ::memcpy(&instr.storeDateTime.time, &time, sizeof(QTime));
            }
            break;
#endif // QT_NO_DATESTRING
        case QVariant::Point:
            {
            bool ok;
            QPoint point = QDeclarativeStringConverters::pointFFromString(v->value.asString(), &ok).toPoint();
            instr.setType(QDeclarativeInstruction::StorePoint);
            instr.storePoint.propertyIndex = prop->index;
            instr.storePoint.point.xp = point.x();
            instr.storePoint.point.yp = point.y();
            }
            break;
        case QVariant::PointF:
            {
            bool ok;
            QPointF point = QDeclarativeStringConverters::pointFFromString(v->value.asString(), &ok);
            instr.setType(QDeclarativeInstruction::StorePointF);
            instr.storePointF.propertyIndex = prop->index;
            instr.storePointF.point.xp = point.x();
            instr.storePointF.point.yp = point.y();
            }
            break;
        case QVariant::Size:
            {
            bool ok;
            QSize size = QDeclarativeStringConverters::sizeFFromString(v->value.asString(), &ok).toSize();
            instr.setType(QDeclarativeInstruction::StoreSize);
            instr.storeSize.propertyIndex = prop->index;
            instr.storeSize.size.wd = size.width();
            instr.storeSize.size.ht = size.height();
            }
            break;
        case QVariant::SizeF:
            {
            bool ok;
            QSizeF size = QDeclarativeStringConverters::sizeFFromString(v->value.asString(), &ok);
            instr.setType(QDeclarativeInstruction::StoreSizeF);
            instr.storeSizeF.propertyIndex = prop->index;
            instr.storeSizeF.size.wd = size.width();
            instr.storeSizeF.size.ht = size.height();
            }
            break;
        case QVariant::Rect:
            {
            bool ok;
            QRect rect = QDeclarativeStringConverters::rectFFromString(v->value.asString(), &ok).toRect();
            instr.setType(QDeclarativeInstruction::StoreRect);
            instr.storeRect.propertyIndex = prop->index;
            instr.storeRect.rect.x1 = rect.left();
            instr.storeRect.rect.y1 = rect.top();
            instr.storeRect.rect.x2 = rect.right();
            instr.storeRect.rect.y2 = rect.bottom();
            }
            break;
        case QVariant::RectF:
            {
            bool ok;
            QRectF rect = QDeclarativeStringConverters::rectFFromString(v->value.asString(), &ok);
            instr.setType(QDeclarativeInstruction::StoreRectF);
            instr.storeRectF.propertyIndex = prop->index;
            instr.storeRectF.rect.xp = rect.left();
            instr.storeRectF.rect.yp = rect.top();
            instr.storeRectF.rect.w = rect.width();
            instr.storeRectF.rect.h = rect.height();
            }
            break;
        case QVariant::Bool:
            {
            bool b = v->value.asBoolean();
            instr.setType(QDeclarativeInstruction::StoreBool);
            instr.storeBool.propertyIndex = prop->index;
            instr.storeBool.value = b;
            }
            break;
        case QVariant::Vector3D:
            {
            bool ok;
            QVector3D vector = QDeclarativeStringConverters::vector3DFromString(v->value.asString(), &ok);
            instr.setType(QDeclarativeInstruction::StoreVector3D);
            instr.storeVector3D.propertyIndex = prop->index;
            instr.storeVector3D.vector.xp = vector.x();
            instr.storeVector3D.vector.yp = vector.y();
            instr.storeVector3D.vector.zp = vector.z();
            }
            break;
    case QVariant::Vector4D:
            {
            bool ok;
            QVector4D vector = QDeclarativeStringConverters::vector4DFromString(string, &ok);
            instr.setType(QDeclarativeInstruction::StoreVector4D);
            instr.storeVector4D.propertyIndex = prop.propertyIndex();
            instr.storeVector4D.vector.xp = vector.x();
            instr.storeVector4D.vector.yp = vector.y();
            instr.storeVector4D.vector.zp = vector.z();
            instr.storeVector4D.vector.wp = vector.w();
            }
            break;
        default:
            {
            instr.setType(QDeclarativeInstruction::AssignCustomType);
            instr.assignCustomType.propertyIndex = prop->index;
            instr.assignCustomType.primitive = output->indexForString(v->value.asString());
            instr.assignCustomType.type = type;
            }
            break;
    }
    output->addInstruction(instr);
}

/*!
    Resets data by clearing the lists that the QDeclarativeCompiler modifies.
*/
void QDeclarativeCompiler::reset(QDeclarativeCompiledData *data)
{
    data->types.clear();
    data->primitives.clear();
    data->datas.clear();
    data->bytecode.resize(0);
}

/*!
    Compile \a unit, and store the output in \a out.  \a engine is the QDeclarativeEngine
    with which the QDeclarativeCompiledData will be associated.

    Returns true on success, false on failure.  On failure, the compile errors
    are available from errors().

    If the environment variant QML_COMPILER_DUMP is set
    (eg. QML_COMPILER_DUMP=1) the compiled instructions will be dumped to stderr
    on a successful compiler.
*/
bool QDeclarativeCompiler::compile(QDeclarativeEngine *engine,
                                   QDeclarativeTypeData *unit,
                                   QDeclarativeCompiledData *out)
{
    exceptions.clear();

    Q_ASSERT(out);
    reset(out);

    output = out;

    // Compile types
    const QList<QDeclarativeTypeData::TypeReference>  &resolvedTypes = unit->resolvedTypes();
    QList<QDeclarativeScriptParser::TypeReference *> referencedTypes = unit->parser().referencedTypes();

    for (int ii = 0; ii < resolvedTypes.count(); ++ii) {
        QDeclarativeCompiledData::TypeReference ref;

        const QDeclarativeTypeData::TypeReference &tref = resolvedTypes.at(ii);
        QDeclarativeScriptParser::TypeReference *parserRef = referencedTypes.at(ii);

        if (tref.type) {
            ref.type = tref.type;
            if (!ref.type->isCreatable()) {
                QString err = ref.type->noCreationReason();
                if (err.isEmpty())
                    err = tr( "Element is not creatable.");
                COMPILE_EXCEPTION(parserRef->refObjects.first(), err);
            }
            
            if (ref.type->containsRevisionedAttributes()) {
                QDeclarativeError cacheError;
                ref.typePropertyCache = 
                    QDeclarativeEnginePrivate::get(engine)->cache(ref.type, resolvedTypes.at(ii).minorVersion, cacheError);

                if (!ref.typePropertyCache) {
                    COMPILE_EXCEPTION(parserRef->refObjects.first(), cacheError.description());
                }
                ref.typePropertyCache->addref();
            }

        } else if (tref.typeData) {
            ref.component = tref.typeData->compiledData();
        }
        ref.className = parserRef->name;
        out->types << ref;
    }

    QDeclarativeParser::Object *root = unit->parser().tree();
    Q_ASSERT(root);

    this->engine = engine;
    this->enginePrivate = QDeclarativeEnginePrivate::get(engine);
    this->unit = unit;
    this->unitRoot = root;
    compileTree(root);

    if (!isError()) {
        if (compilerDump())
            out->dumpInstructions();
        if (componentStats)
            dumpStats();
        Q_ASSERT(out->rootPropertyCache);
    } else {
        reset(out);
    }

    compileState = 0;
    output = 0;
    this->engine = 0;
    this->enginePrivate = 0;
    this->unit = 0;
    this->unitRoot = 0;

    return !isError();
}

void QDeclarativeCompiler::compileTree(QDeclarativeParser::Object *tree)
{
    compileState = pool->New<ComponentCompileState>();

    compileState->root = tree;
    if (componentStats)
        componentStats->componentStat.lineNumber = tree->location.start.line;

    // Build global import scripts
    QStringList importedScriptIndexes;

    foreach (const QDeclarativeTypeData::ScriptReference &script, unit->resolvedScripts()) {
        importedScriptIndexes.append(script.qualifier);

        QDeclarativeInstruction import;
        import.setType(QDeclarativeInstruction::StoreImportedScript);
        import.storeScript.value = output->scripts.count();

        QDeclarativeScriptData *scriptData = script.script->scriptData();
        scriptData->addref();
        output->scripts << scriptData;
        output->addInstruction(import);
    }

    // We generate the importCache before we build the tree so that
    // it can be used in the binding compiler.  Given we "expect" the
    // QML compilation to succeed, this isn't a waste.
    output->importCache = new QDeclarativeTypeNameCache(engine);
    for (int ii = 0; ii < importedScriptIndexes.count(); ++ii) 
        output->importCache->add(importedScriptIndexes.at(ii), ii);
    unit->imports().populateCache(output->importCache, engine);

    if (!buildObject(tree, BindingContext()) || !completeComponentBuild())
        return;

    QDeclarativeInstruction init;
    init.setType(QDeclarativeInstruction::Init);
    init.init.bindingsSize = compileState->bindings.count();
    init.init.parserStatusSize = compileState->parserStatusCount;
    init.init.contextCache = genContextCache();
    if (compileState->compiledBindingData.isEmpty())
        init.init.compiledBinding = -1;
    else
        init.init.compiledBinding = output->indexForByteArray(compileState->compiledBindingData);
    output->addInstruction(init);

    if (!compileState->v8BindingProgram.isEmpty()) {
        QDeclarativeInstruction bindings;
        bindings.setType(QDeclarativeInstruction::InitV8Bindings);
        bindings.initV8Bindings.program = output->indexForString(compileState->v8BindingProgram);
        bindings.initV8Bindings.programIndex = compileState->v8BindingProgramIndex;
        bindings.initV8Bindings.line = compileState->v8BindingProgramLine;
        output->addInstruction(bindings);
    }

    genObject(tree);

    QDeclarativeInstruction def;
    def.setType(QDeclarativeInstruction::SetDefault);
    output->addInstruction(def);

    QDeclarativeInstruction done;
    done.setType(QDeclarativeInstruction::Done);
    output->addInstruction(done);

    Q_ASSERT(tree->metatype);

    if (tree->metadata.isEmpty()) {
        output->root = tree->metatype;
    } else {
        static_cast<QMetaObject &>(output->rootData) = *tree->metaObject();
        output->root = &output->rootData;
    }
    if (!tree->metadata.isEmpty()) 
        enginePrivate->registerCompositeType(output);
}

static bool QStringList_contains(const QStringList &list, const QHashedStringRef &string)
{
    for (int ii = 0; ii < list.count(); ++ii)
        if (string == list.at(ii))
            return true;

    return false;
}

bool QDeclarativeCompiler::buildObject(QDeclarativeParser::Object *obj, const BindingContext &ctxt)
{
    if (componentStats)
        componentStats->componentStat.objects++;

    Q_ASSERT (obj->type != -1);
    const QDeclarativeCompiledData::TypeReference &tr =
        output->types.at(obj->type);
    obj->metatype = tr.metaObject();

    if (tr.type) 
        obj->typeName = tr.type->qmlTypeName();

    // This object is a "Component" element
    if (tr.type && obj->metatype == &QDeclarativeComponent::staticMetaObject) {
        COMPILE_CHECK(buildComponent(obj, ctxt));
        return true;
    } 

    // Object instantiations reset the binding context
    BindingContext objCtxt(obj);

    // Create the synthesized meta object, ignoring aliases
    COMPILE_CHECK(checkDynamicMeta(obj)); 
    COMPILE_CHECK(mergeDynamicMetaProperties(obj));
    COMPILE_CHECK(buildDynamicMeta(obj, IgnoreAliases));

    // Find the native type and check for the QDeclarativeParserStatus interface
    QDeclarativeType *type = toQmlType(obj);
    Q_ASSERT(type);
    obj->parserStatusCast = type->parserStatusCast();
    if (obj->parserStatusCast != -1)
        compileState->parserStatusCount++;

    // Check if this is a custom parser type.  Custom parser types allow
    // assignments to non-existent properties.  These assignments are then
    // compiled by the type.
    bool isCustomParser = output->types.at(obj->type).type &&
                          output->types.at(obj->type).type->customParser() != 0;
    QList<QDeclarativeCustomParserProperty> customProps;

    // Fetch the list of deferred properties
    QStringList deferredList = deferredProperties(obj);

    // Must do id property first.  This is to ensure that the id given to any
    // id reference created matches the order in which the objects are
    // instantiated
    for (Property *prop = obj->properties.first(); prop; prop = obj->properties.next(prop)) {
        if (prop->name() == id_string) {
            COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
            break;
        }
    }

    // Merge 
    Property *defaultProperty = 0;
    Property *skipProperty = 0;
    if (obj->defaultProperty) {
        defaultProperty = obj->defaultProperty;

        const QMetaObject *metaObject = obj->metaObject();
        Q_ASSERT(metaObject);
        QMetaProperty p = QDeclarativeMetaType::defaultProperty(metaObject);
        if (p.name()) {
            Property *explicitProperty = obj->getProperty(QString::fromUtf8(p.name()), false);
            if (explicitProperty && !explicitProperty->value && !explicitProperty->values.isEmpty()) {

                skipProperty = explicitProperty; // We merge the values into defaultProperty

                // Find the correct insertion point
                Value *insertPos = 0;

                for (Value *v = defaultProperty->values.first(); v; v = Property::ValueList::next(v)) {
                    if (!(v->location.start < explicitProperty->values.first()->location.start))
                        break;
                    insertPos = v;
                }

                defaultProperty->values.insertAfter(insertPos, explicitProperty->values);

            } 
        }
    }

    QDeclarativeCustomParser *cp = 0;
    if (isCustomParser)
        cp = output->types.at(obj->type).type->customParser();

    // Build all explicit properties specified
    for (Property *prop = obj->properties.first(); prop; prop = obj->properties.next(prop)) {

        if (prop == skipProperty)
            continue;
        if (prop->name() == id_string)
            continue;

        bool canDefer = false;
        if (isCustomParser) {
            if (doesPropertyExist(prop, obj) &&
                (!(cp->flags() & QDeclarativeCustomParser::AcceptsAttachedProperties) ||
                 !isAttachedPropertyName(prop->name()))) {
                int ids = compileState->ids.count();
                COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
                canDefer = ids == compileState->ids.count();
            } else if (isSignalPropertyName(prop->name()) &&
                    (cp->flags() & QDeclarativeCustomParser::AcceptsSignalHandlers)) {
                COMPILE_CHECK(buildSignal(prop,obj,objCtxt));
            } else {
                customProps << QDeclarativeCustomParserNodePrivate::fromProperty(prop);
            }
        } else {
            if (isSignalPropertyName(prop->name())) {
                COMPILE_CHECK(buildSignal(prop,obj,objCtxt));
            } else {
                int ids = compileState->ids.count();
                COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
                canDefer = ids == compileState->ids.count();
            }
        }

        if (canDefer && !deferredList.isEmpty() && QStringList_contains(deferredList, prop->name()))
            prop->isDeferred = true;

    }

    // Build the default property
    if (defaultProperty)  {
        Property *prop = defaultProperty;

        bool canDefer = false;
        if (isCustomParser) {
            if (doesPropertyExist(prop, obj)) {
                int ids = compileState->ids.count();
                COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
                canDefer = ids == compileState->ids.count();
            } else {
                customProps << QDeclarativeCustomParserNodePrivate::fromProperty(prop);
            }
        } else {
            int ids = compileState->ids.count();
            COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
            canDefer = ids == compileState->ids.count();
        }

        if (canDefer && !deferredList.isEmpty() && QStringList_contains(deferredList, prop->name()))
            prop->isDeferred = true;
    }

    // Compile custom parser parts
    if (isCustomParser && !customProps.isEmpty()) {
        cp->clearErrors();
        cp->compiler = this;
        cp->object = obj;
        obj->custom = cp->compile(customProps);
        cp->compiler = 0;
        cp->object = 0;
        foreach (QDeclarativeError err, cp->errors()) {
            err.setUrl(output->url);
            exceptions << err;
        }
    }

    return true;
}

void QDeclarativeCompiler::genObject(QDeclarativeParser::Object *obj)
{
    QDeclarativeCompiledData::TypeReference &tr = output->types[obj->type];
    if (tr.type && obj->metatype == &QDeclarativeComponent::staticMetaObject) {
        genComponent(obj);
        return;
    }

    // Create the object
    if (obj->custom.isEmpty() && output->types.at(obj->type).type &&
        !output->types.at(obj->type).type->isExtendedType() && obj != compileState->root) {

        QDeclarativeInstruction create;
        create.setType(QDeclarativeInstruction::CreateSimpleObject);
        create.createSimple.create = output->types.at(obj->type).type->createFunction();
        create.createSimple.typeSize = output->types.at(obj->type).type->createSize();
        create.createSimple.type = obj->type;
        create.createSimple.line = obj->location.start.line;
        create.createSimple.column = obj->location.start.column;
        output->addInstruction(create);

    } else {

        QDeclarativeInstruction create;
        create.setType(QDeclarativeInstruction::CreateObject);
        create.create.line = obj->location.start.line;
        create.create.column = obj->location.start.column;
        create.create.data = -1;
        if (!obj->custom.isEmpty())
            create.create.data = output->indexForByteArray(obj->custom);
        create.create.type = obj->type;
        if (!output->types.at(create.create.type).type && 
            !obj->bindingBitmask.isEmpty()) {
            Q_ASSERT(obj->bindingBitmask.size() % 4 == 0);
            create.create.bindingBits = 
                output->indexForByteArray(obj->bindingBitmask);
        } else {
            create.create.bindingBits = -1;
        }
        output->addInstruction(create);

    }

    // Setup the synthesized meta object if necessary
    if (!obj->metadata.isEmpty()) {
        QDeclarativeInstruction meta;
        meta.setType(QDeclarativeInstruction::StoreMetaObject);
        meta.storeMeta.data = output->indexForByteArray(obj->metadata);
        meta.storeMeta.aliasData = output->indexForByteArray(obj->synthdata);
        meta.storeMeta.propertyCache = output->propertyCaches.count();

        QDeclarativePropertyCache *propertyCache = obj->synthCache;
        Q_ASSERT(propertyCache);
        propertyCache->addref();

        // Add flag for alias properties
        if (!obj->synthdata.isEmpty()) {
            const QDeclarativeVMEMetaData *vmeMetaData = 
                reinterpret_cast<const QDeclarativeVMEMetaData *>(obj->synthdata.constData());
            for (int ii = 0; ii < vmeMetaData->aliasCount; ++ii) {
                int index = obj->metaObject()->propertyOffset() + vmeMetaData->propertyCount + ii;
                QDeclarativePropertyCache::Data *data = propertyCache->property(index);
                data->setFlags(data->getFlags() | QDeclarativePropertyCache::Data::IsAlias);
            }
        }

        if (obj == unitRoot) {
            propertyCache->addref();
            output->rootPropertyCache = propertyCache;
        }

        output->propertyCaches << propertyCache;
        output->addInstruction(meta);
    } else if (obj == unitRoot) {
        output->rootPropertyCache = tr.createPropertyCache(engine);
        output->rootPropertyCache->addref();
    }

    // Set the object id
    if (!obj->id.isEmpty()) {
        QDeclarativeInstruction id;
        id.setType(QDeclarativeInstruction::SetId);
        id.setId.value = output->indexForString(obj->id);
        id.setId.index = obj->idIndex;
        output->addInstruction(id);
    }

    // Begin the class
    if (tr.type && obj->parserStatusCast != -1) {
        QDeclarativeInstruction begin;
        begin.setType(QDeclarativeInstruction::BeginObject);
        begin.begin.castValue = obj->parserStatusCast;
        output->addInstruction(begin);
    }

    genObjectBody(obj);
}

void QDeclarativeCompiler::genObjectBody(QDeclarativeParser::Object *obj)
{
    for (Property *prop = obj->scriptStringProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        Q_ASSERT(prop->scriptStringScope != -1);
        const QString &script = prop->values.first()->value.asScript();
        QDeclarativeInstruction ss;
        ss.setType(QDeclarativeInstruction::StoreScriptString);
        ss.storeScriptString.propertyIndex = prop->index;
        ss.storeScriptString.value = output->indexForString(script);
        ss.storeScriptString.scope = prop->scriptStringScope;
//        ss.storeScriptString.bindingId = rewriteBinding(script, prop->name());
        ss.storeScriptString.bindingId = rewriteBinding(script, QString()); // XXX
        ss.storeScriptString.line = prop->location.start.line;
        output->addInstruction(ss);
    }

    bool seenDefer = false;
    for (Property *prop = obj->valueProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        if (prop->isDeferred) {
            seenDefer = true;
            continue;
        }
        if (!prop->isAlias)
            genValueProperty(prop, obj);
    }
    if (seenDefer) {
        QDeclarativeInstruction defer;
        defer.setType(QDeclarativeInstruction::Defer);
        defer.defer.deferCount = 0;
        int deferIdx = output->addInstruction(defer);
        int nextInstructionIndex = output->nextInstructionIndex();

        QDeclarativeInstruction init;
        init.setType(QDeclarativeInstruction::Init);
        init.init.bindingsSize = compileState->bindings.count(); // XXX - bigger than necessary
        init.init.parserStatusSize = compileState->parserStatusCount; // XXX - bigger than necessary
        init.init.contextCache = -1;
        init.init.compiledBinding = -1;
        output->addInstruction(init);

        for (Property *prop = obj->valueProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
            if (!prop->isDeferred)
                continue;
            genValueProperty(prop, obj);
        }

        QDeclarativeInstruction done;
        done.setType(QDeclarativeInstruction::Done);
        output->addInstruction(done);

        output->instruction(deferIdx)->defer.deferCount = output->nextInstructionIndex() - nextInstructionIndex;
    }

    for (Property *prop = obj->signalProperties.first(); prop; prop = Object::PropertyList::next(prop)) {

        QDeclarativeParser::Value *v = prop->values.first();

        if (v->type == Value::SignalObject) {

            genObject(v->object);

            QDeclarativeInstruction assign;
            assign.setType(QDeclarativeInstruction::AssignSignalObject);
            assign.assignSignalObject.line = v->location.start.line;
            assign.assignSignalObject.signal = output->indexForString(prop->name().toString());
            output->addInstruction(assign);

        } else if (v->type == Value::SignalExpression) {

            QDeclarativeInstruction store;
            store.setType(QDeclarativeInstruction::StoreSignal);
            store.storeSignal.signalIndex = prop->index;
            store.storeSignal.value =
                output->indexForString(v->value.asScript().trimmed());
            store.storeSignal.context = v->signalExpressionContextStack;
            store.storeSignal.name = output->indexForByteArray(prop->name().toUtf8());
            store.storeSignal.line = v->location.start.line;
            output->addInstruction(store);

        }

    }

    for (Property *prop = obj->attachedProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        QDeclarativeInstruction fetch;
        fetch.setType(QDeclarativeInstruction::FetchAttached);
        fetch.fetchAttached.id = prop->index;
        fetch.fetchAttached.line = prop->location.start.line;
        output->addInstruction(fetch);

        genObjectBody(prop->value);

        QDeclarativeInstruction pop;
        pop.setType(QDeclarativeInstruction::PopFetchedObject);
        output->addInstruction(pop);
    }

    for (Property *prop = obj->groupedProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        QDeclarativeInstruction fetch;
        fetch.setType(QDeclarativeInstruction::FetchObject);
        fetch.fetch.property = prop->index;
        fetch.fetch.line = prop->location.start.line;
        output->addInstruction(fetch);

        if (!prop->value->metadata.isEmpty()) {
            QDeclarativeInstruction meta;
            meta.setType(QDeclarativeInstruction::StoreMetaObject);
            meta.storeMeta.data = output->indexForByteArray(prop->value->metadata);
            meta.storeMeta.aliasData = output->indexForByteArray(prop->value->synthdata);
            meta.storeMeta.propertyCache = -1;
            output->addInstruction(meta);
        }

        genObjectBody(prop->value);

        QDeclarativeInstruction pop;
        pop.setType(QDeclarativeInstruction::PopFetchedObject);
        output->addInstruction(pop);
    }

    for (Property *prop = obj->valueTypeProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        if (!prop->isAlias)
            genValueTypeProperty(obj, prop);
    }

    for (Property *prop = obj->valueProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        if (prop->isDeferred) 
            continue;
        if (prop->isAlias)
            genValueProperty(prop, obj);
    }

    for (Property *prop = obj->valueTypeProperties.first(); prop; prop = Object::PropertyList::next(prop)) {
        if (prop->isAlias)
            genValueTypeProperty(obj, prop);
    }
}

void QDeclarativeCompiler::genValueTypeProperty(QDeclarativeParser::Object *obj,QDeclarativeParser::Property *prop)
{
    QDeclarativeInstruction fetch;
    fetch.setType(QDeclarativeInstruction::FetchValueType);
    fetch.fetchValue.property = prop->index;
    fetch.fetchValue.type = prop->type;
    fetch.fetchValue.bindingSkipList = 0;

    if (obj->type == -1 || output->types.at(obj->type).component) {
        // We only have to do this if this is a composite type.  If it is a builtin
        // type it can't possibly already have bindings that need to be cleared.
        for (Property *vprop = prop->value->valueProperties.first(); vprop; vprop = Object::PropertyList::next(vprop)) {
            if (!vprop->values.isEmpty()) {
                Q_ASSERT(vprop->index >= 0 && vprop->index < 32);
                fetch.fetchValue.bindingSkipList |= (1 << vprop->index);
            }
        }
    }

    output->addInstruction(fetch);

    for (Property *vprop = prop->value->valueProperties.first(); vprop; vprop = Object::PropertyList::next(vprop)) {
        genPropertyAssignment(vprop, prop->value, prop);
    }

    QDeclarativeInstruction pop;
    pop.setType(QDeclarativeInstruction::PopValueType);
    pop.fetchValue.property = prop->index;
    pop.fetchValue.type = prop->type;
    pop.fetchValue.bindingSkipList = 0;
    output->addInstruction(pop);
}

void QDeclarativeCompiler::genComponent(QDeclarativeParser::Object *obj)
{
    QDeclarativeParser::Object *root = obj->defaultProperty->values.first()->object;
    Q_ASSERT(root);

    QDeclarativeInstruction create;
    create.setType(QDeclarativeInstruction::CreateComponent);
    create.createComponent.line = root->location.start.line;
    create.createComponent.column = root->location.start.column;
    create.createComponent.endLine = root->location.end.line;
    int createInstruction = output->addInstruction(create);
    int nextInstructionIndex = output->nextInstructionIndex();

    ComponentCompileState *oldCompileState = compileState;
    compileState = componentState(root);

    QDeclarativeInstruction init;
    init.setType(QDeclarativeInstruction::Init);
    init.init.bindingsSize = compileState->bindings.count();
    init.init.parserStatusSize = compileState->parserStatusCount;
    init.init.contextCache = genContextCache();
    if (compileState->compiledBindingData.isEmpty())
        init.init.compiledBinding = -1;
    else
        init.init.compiledBinding = output->indexForByteArray(compileState->compiledBindingData);
    output->addInstruction(init);

    if (!compileState->v8BindingProgram.isEmpty()) {
        QDeclarativeInstruction bindings;
        bindings.setType(QDeclarativeInstruction::InitV8Bindings);
        bindings.initV8Bindings.program = output->indexForString(compileState->v8BindingProgram);
        bindings.initV8Bindings.programIndex = compileState->v8BindingProgramIndex;
        bindings.initV8Bindings.line = compileState->v8BindingProgramLine;
        output->addInstruction(bindings);
    }

    genObject(root);

    QDeclarativeInstruction def;
    def.setType(QDeclarativeInstruction::SetDefault);
    output->addInstruction(def);

    QDeclarativeInstruction done;
    done.setType(QDeclarativeInstruction::Done);
    output->addInstruction(done);

    output->instruction(createInstruction)->createComponent.count = 
        output->nextInstructionIndex() - nextInstructionIndex;

    compileState = oldCompileState;

    if (!obj->id.isEmpty()) {
        QDeclarativeInstruction id;
        id.setType(QDeclarativeInstruction::SetId);
        id.setId.value = output->indexForString(obj->id);
        id.setId.index = obj->idIndex;
        output->addInstruction(id);
    }

    if (obj == unitRoot) {
        output->rootPropertyCache = output->types[obj->type].createPropertyCache(engine);
        output->rootPropertyCache->addref();
    }
}

bool QDeclarativeCompiler::buildComponent(QDeclarativeParser::Object *obj,
                                 const BindingContext &ctxt)
{
    // The special "Component" element can only have the id property and a
    // default property, that actually defines the component's tree

    // Find, check and set the "id" property (if any)
    Property *idProp = 0;
    if (obj->properties.isMany() ||
       (obj->properties.isOne() && obj->properties.first()->name() != id_string))
        COMPILE_EXCEPTION(obj->properties.first(), tr("Component elements may not contain properties other than id"));
       
    if (!obj->properties.isEmpty())
        idProp = obj->properties.first();

    if (idProp) {
       if (idProp->value || idProp->values.isMany() || idProp->values.first()->object) 
           COMPILE_EXCEPTION(idProp, tr("Invalid component id specification"));
       COMPILE_CHECK(checkValidId(idProp->values.first(), idProp->values.first()->primitive()))

        QString idVal = idProp->values.first()->primitive();

        if (compileState->ids.value(idVal))
            COMPILE_EXCEPTION(idProp, tr("id is not unique"));

        obj->id = idVal;
        addId(idVal, obj);
    }

    // Check the Component tree is well formed
    if (obj->defaultProperty &&
       (obj->defaultProperty->value || obj->defaultProperty->values.isMany() ||
        (obj->defaultProperty->values.isOne() && !obj->defaultProperty->values.first()->object)))
        COMPILE_EXCEPTION(obj, tr("Invalid component body specification"));

    if (!obj->dynamicProperties.isEmpty())
        COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new properties."));
    if (!obj->dynamicSignals.isEmpty())
        COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new signals."));
    if (!obj->dynamicSlots.isEmpty())
        COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new functions."));

    QDeclarativeParser::Object *root = 0;
    if (obj->defaultProperty && !obj->defaultProperty->values.isEmpty())
        root = obj->defaultProperty->values.first()->object;

    if (!root)
        COMPILE_EXCEPTION(obj, tr("Cannot create empty component specification"));

    // Build the component tree
    COMPILE_CHECK(buildComponentFromRoot(root, ctxt));

    return true;
}

bool QDeclarativeCompiler::buildComponentFromRoot(QDeclarativeParser::Object *obj,
                                         const BindingContext &ctxt)
{
    ComponentCompileState *oldComponentCompileState = compileState;
    compileState = pool->New<ComponentCompileState>();
    compileState->root = obj;
    compileState->nested = true;

    if (componentStats) {
        ComponentStat oldComponentStat = componentStats->componentStat;

        componentStats->componentStat = ComponentStat();
        componentStats->componentStat.lineNumber = obj->location.start.line;

        if (obj)
            COMPILE_CHECK(buildObject(obj, ctxt));

        COMPILE_CHECK(completeComponentBuild());

        componentStats->componentStat = oldComponentStat;
    } else {
        if (obj)
            COMPILE_CHECK(buildObject(obj, ctxt));

        COMPILE_CHECK(completeComponentBuild());
    }

    compileState = oldComponentCompileState;

    return true;
}


// Build a sub-object.  A sub-object is one that was not created directly by
// QML - such as a grouped property object, or an attached object.  Sub-object's
// can't have an id, involve a custom parser, have attached properties etc.
bool QDeclarativeCompiler::buildSubObject(QDeclarativeParser::Object *obj, const BindingContext &ctxt)
{
    Q_ASSERT(obj->metatype);
    Q_ASSERT(!obj->defaultProperty);
    Q_ASSERT(ctxt.isSubContext()); // sub-objects must always be in a binding
                                   // sub-context

    for (Property *prop = obj->properties.first(); prop; prop = obj->properties.next(prop)) {
        if (isSignalPropertyName(prop->name())) {
            COMPILE_CHECK(buildSignal(prop, obj, ctxt));
        } else {
            COMPILE_CHECK(buildProperty(prop, obj, ctxt));
        }
    }

    return true;
}

int QDeclarativeCompiler::componentTypeRef()
{
    QDeclarativeType *t = QDeclarativeMetaType::qmlType("QtQuick/Component",2,0);
    for (int ii = output->types.count() - 1; ii >= 0; --ii) {
        if (output->types.at(ii).type == t)
            return ii;
    }
    QDeclarativeCompiledData::TypeReference ref;
    ref.className = Component_string;
    ref.type = t;
    output->types << ref;
    return output->types.count() - 1;
}

bool QDeclarativeCompiler::buildSignal(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj,
                                       const BindingContext &ctxt)
{
    Q_ASSERT(obj->metaObject());

    const QHashedStringRef &propName = prop->name();

    Q_ASSERT(propName.startsWith(on_string));
    QString name = propName.mid(2, -1).toString();

    // Note that the property name could start with any alpha or '_' or '$' character,
    // so we need to do the lower-casing of the first alpha character.
    for (int firstAlphaIndex = 0; firstAlphaIndex < name.size(); ++firstAlphaIndex) {
        if (QDeclarativeUtils::isUpper(name.at(firstAlphaIndex))) {
            name[firstAlphaIndex] = name.at(firstAlphaIndex).toLower();
            break;
        }
    }

    bool notInRevision = false;

    QDeclarativePropertyCache::Data *sig = signal(obj, QStringRef(&name), &notInRevision);

    if (sig == 0) {

        if (notInRevision && 0 == property(obj, propName, 0)) {
            Q_ASSERT(obj->type != -1);
            const QList<QDeclarativeTypeData::TypeReference>  &resolvedTypes = unit->resolvedTypes();
            const QDeclarativeTypeData::TypeReference &type = resolvedTypes.at(obj->type);
            if (type.type) {
                COMPILE_EXCEPTION(prop, tr("\"%1.%2\" is not available in %3 %4.%5.").arg(elementName(obj)).arg(prop->name().toString()).arg(QString::fromUtf8(type.type->module())).arg(type.majorVersion).arg(type.minorVersion));
            } else {
                COMPILE_EXCEPTION(prop, tr("\"%1.%2\" is not available due to component versioning.").arg(elementName(obj)).arg(prop->name().toString()));
            }
        }

        // If the "on<Signal>" name doesn't resolve into a signal, try it as a
        // property.
        COMPILE_CHECK(buildProperty(prop, obj, ctxt));

    }  else {

        if (prop->value || !prop->values.isOne())
            COMPILE_EXCEPTION(prop, tr("Incorrectly specified signal assignment"));

        prop->index = sig->coreIndex;
        prop->core = *sig;

        obj->addSignalProperty(prop);

        if (prop->values.first()->object) {
            COMPILE_CHECK(buildObject(prop->values.first()->object, ctxt));
            prop->values.first()->type = Value::SignalObject;
        } else {
            prop->values.first()->type = Value::SignalExpression;

            if (!prop->values.first()->value.isScript())
                COMPILE_EXCEPTION(prop, tr("Cannot assign a value to a signal (expecting a script to be run)"));

            QString script = prop->values.first()->value.asScript().trimmed();
            if (script.isEmpty())
                COMPILE_EXCEPTION(prop, tr("Empty signal assignment"));

            prop->values.first()->signalExpressionContextStack = ctxt.stack;
        }
    }

    return true;
}


/*!
    Returns true if (value) property \a prop exists on obj, false otherwise.
*/
bool QDeclarativeCompiler::doesPropertyExist(QDeclarativeParser::Property *prop,
                                             QDeclarativeParser::Object *obj)
{
    if(isAttachedPropertyName(prop->name()) || prop->name() == id_string)
        return true;

    if (prop->isDefault) {
        const QMetaObject *mo = obj->metaObject();
        QMetaProperty p = QDeclarativeMetaType::defaultProperty(mo);
        return p.name() != 0;
    } else {
        return property(obj, prop->name()) != 0;
    }
}

bool QDeclarativeCompiler::buildProperty(QDeclarativeParser::Property *prop,
                                         QDeclarativeParser::Object *obj,
                                         const BindingContext &ctxt)
{
    if (prop->isEmpty()) 
        COMPILE_EXCEPTION(prop, tr("Empty property assignment"));

    const QMetaObject *metaObject = obj->metaObject();
    Q_ASSERT(metaObject);

    if (isAttachedPropertyName(prop->name())) {
        // Setup attached property data

        if (ctxt.isSubContext()) {
            // Attached properties cannot be used on sub-objects.  Sub-objects
            // always exist in a binding sub-context, which is what we test
            // for here.
            COMPILE_EXCEPTION(prop, tr("Attached properties cannot be used here"));
        }

        QDeclarativeType *type = 0;
        QDeclarativeImportedNamespace *typeNamespace = 0;
        unit->imports().resolveType(prop->name().toUtf8(), &type, 0, 0, 0, &typeNamespace);

        if (typeNamespace) {
            COMPILE_CHECK(buildPropertyInNamespace(typeNamespace, prop, obj, 
                                                   ctxt));
            return true;
        } else if (!type || !type->attachedPropertiesType())  {
            COMPILE_EXCEPTION(prop, tr("Non-existent attached object"));
        }

        if (!prop->value)
            COMPILE_EXCEPTION(prop, tr("Invalid attached object assignment"));

        Q_ASSERT(type->attachedPropertiesFunction());
        prop->index = type->attachedPropertiesId();
        prop->value->metatype = type->attachedPropertiesType();
    } else {
        // Setup regular property data

        if (prop->isDefault) {
            QMetaProperty p = QDeclarativeMetaType::defaultProperty(metaObject);

            if (p.name()) {
                prop->setName(QString::fromLatin1(p.name()));

                QDeclarativePropertyCache::Data *d = property(obj, p.propertyIndex());
                prop->index = d->coreIndex;
                prop->core = *d;
            }

        } else {
            bool notInRevision = false;
            QDeclarativePropertyCache::Data *d = property(obj, prop->name(), &notInRevision);

            if (d == 0 && notInRevision) {
                const QList<QDeclarativeTypeData::TypeReference>  &resolvedTypes = unit->resolvedTypes();
                const QDeclarativeTypeData::TypeReference &type = resolvedTypes.at(obj->type);
                if (type.type) {
                    COMPILE_EXCEPTION(prop, tr("\"%1.%2\" is not available in %3 %4.%5.").arg(elementName(obj)).arg(prop->name().toString()).arg(QString::fromUtf8(type.type->module())).arg(type.majorVersion).arg(type.minorVersion));
                } else {
                    COMPILE_EXCEPTION(prop, tr("\"%1.%2\" is not available due to component versioning.").arg(elementName(obj)).arg(prop->name().toString()));
                }
            } else if (d) {
                prop->index = d->coreIndex;
                prop->core = *d;
            }
        }

        // We can't error here as the "id" property does not require a
        // successful index resolution
        if (prop->index != -1) 
            prop->type = prop->core.propType;

        // Check if this is an alias
        if (prop->index != -1 && 
            prop->parent && 
            prop->parent->type != -1 && 
            output->types.at(prop->parent->type).component) {

            QDeclarativePropertyCache *cache = output->types.at(prop->parent->type).component->rootPropertyCache;
            if (cache && cache->property(prop->index) && cache->property(prop->index)->isAlias())
                prop->isAlias = true;
        }

        if (prop->index != -1 && !prop->values.isEmpty()) 
            prop->parent->setBindingBit(prop->index);
    }

    if (!prop->isDefault && prop->name() == id_string && !ctxt.isSubContext()) {

        // The magic "id" behavior doesn't apply when "id" is resolved as a
        // default property or to sub-objects (which are always in binding
        // sub-contexts)
        COMPILE_CHECK(buildIdProperty(prop, obj));
        if (prop->type == QVariant::String &&
            prop->values.first()->value.isString())
            COMPILE_CHECK(buildPropertyAssignment(prop, obj, ctxt));

    } else if (isAttachedPropertyName(prop->name())) {

        COMPILE_CHECK(buildAttachedProperty(prop, obj, ctxt));

    } else if (prop->index == -1) {

        if (prop->isDefault) {
            COMPILE_EXCEPTION(prop->values.first(), tr("Cannot assign to non-existent default property"));
        } else {
            COMPILE_EXCEPTION(prop, tr("Cannot assign to non-existent property \"%1\"").arg(prop->name().toString()));
        }

    } else if (prop->value) {

        COMPILE_CHECK(buildGroupedProperty(prop, obj, ctxt));

    } else if (enginePrivate->isList(prop->type)) {

        COMPILE_CHECK(buildListProperty(prop, obj, ctxt));

    } else if (prop->type == qMetaTypeId<QDeclarativeScriptString>()) {

        COMPILE_CHECK(buildScriptStringProperty(prop, obj, ctxt));

    } else {

        COMPILE_CHECK(buildPropertyAssignment(prop, obj, ctxt));

    }

    return true;
}

bool QDeclarativeCompiler::buildPropertyInNamespace(QDeclarativeImportedNamespace *ns,
                                                    QDeclarativeParser::Property *nsProp, 
                                                    QDeclarativeParser::Object *obj, 
                                                    const BindingContext &ctxt)
{
    if (!nsProp->value)
        COMPILE_EXCEPTION(nsProp, tr("Invalid use of namespace"));

    for (Property *prop = nsProp->value->properties.first(); prop; prop = nsProp->value->properties.next(prop)) {

        if (!isAttachedPropertyName(prop->name()))
            COMPILE_EXCEPTION(prop, tr("Not an attached property name"));

        // Setup attached property data

        QDeclarativeType *type = 0;
        unit->imports().resolveType(ns, prop->name().toUtf8(), &type, 0, 0, 0);

        if (!type || !type->attachedPropertiesType()) 
            COMPILE_EXCEPTION(prop, tr("Non-existent attached object"));

        if (!prop->value)
            COMPILE_EXCEPTION(prop, tr("Invalid attached object assignment"));

        Q_ASSERT(type->attachedPropertiesFunction());
        prop->index = type->index();
        prop->value->metatype = type->attachedPropertiesType();

        COMPILE_CHECK(buildAttachedProperty(prop, obj, ctxt));
    }

    return true;
}

void QDeclarativeCompiler::genValueProperty(QDeclarativeParser::Property *prop,
                                   QDeclarativeParser::Object *obj)
{
    if (enginePrivate->isList(prop->type)) {
        genListProperty(prop, obj);
    } else {
        genPropertyAssignment(prop, obj);
    }
}

void QDeclarativeCompiler::genListProperty(QDeclarativeParser::Property *prop,
                                  QDeclarativeParser::Object *obj)
{
    int listType = enginePrivate->listType(prop->type);

    QDeclarativeInstruction fetch;
    fetch.setType(QDeclarativeInstruction::FetchQList);
    fetch.fetchQmlList.property = prop->index;
    bool listTypeIsInterface = QDeclarativeMetaType::isInterface(listType);
    fetch.fetchQmlList.type = listType;
    output->addInstruction(fetch);

    for (Value *v = prop->values.first(); v; v = Property::ValueList::next(v)) {

        if (v->type == Value::CreatedObject) {

            genObject(v->object);
            if (listTypeIsInterface) {
                QDeclarativeInstruction assign;
                assign.setType(QDeclarativeInstruction::AssignObjectList);
                assign.assignObjectList.line = prop->location.start.line;
                output->addInstruction(assign);
            } else {
                QDeclarativeInstruction store;
                store.setType(QDeclarativeInstruction::StoreObjectQList);
                output->addInstruction(store);
            }

        } else if (v->type == Value::PropertyBinding) {

            genBindingAssignment(v, prop, obj);

        }

    }

    QDeclarativeInstruction pop;
    pop.setType(QDeclarativeInstruction::PopQList);
    output->addInstruction(pop);
}

void QDeclarativeCompiler::genPropertyAssignment(QDeclarativeParser::Property *prop,
                                        QDeclarativeParser::Object *obj,
                                        QDeclarativeParser::Property *valueTypeProperty)
{
    for (Value *v = prop->values.first(); v; v = Property::ValueList::next(v)) {

        Q_ASSERT(v->type == Value::CreatedObject ||
                 v->type == Value::PropertyBinding ||
                 v->type == Value::Literal);

        if (v->type == Value::CreatedObject) {

            genObject(v->object);

            if (QDeclarativeMetaType::isInterface(prop->type)) {

                QDeclarativeInstruction store;
                store.setType(QDeclarativeInstruction::StoreInterface);
                store.storeObject.line = v->object->location.start.line;
                store.storeObject.propertyIndex = prop->index;
                output->addInstruction(store);

            } else if (prop->type == QMetaType::QVariant) {

                QDeclarativeInstruction store;
                store.setType(QDeclarativeInstruction::StoreVariantObject);
                store.storeObject.line = v->object->location.start.line;
                store.storeObject.propertyIndex = prop->index;
                output->addInstruction(store);

            } else {

                QDeclarativeInstruction store;
                store.setType(QDeclarativeInstruction::StoreObject);
                store.storeObject.line = v->object->location.start.line;
                store.storeObject.propertyIndex = prop->index;
                output->addInstruction(store);

            }
        } else if (v->type == Value::PropertyBinding) {

            genBindingAssignment(v, prop, obj, valueTypeProperty);

        } else if (v->type == Value::Literal) {

            genLiteralAssignment(prop, v);

        }

    }

    for (Value *v = prop->onValues.first(); v; v = Property::ValueList::next(v)) {

        Q_ASSERT(v->type == Value::ValueSource ||
                 v->type == Value::ValueInterceptor);

        if (v->type == Value::ValueSource) {
            genObject(v->object);

            QDeclarativeInstruction store;
            store.setType(QDeclarativeInstruction::StoreValueSource);
            if (valueTypeProperty) {
                store.assignValueSource.property = genValueTypeData(prop, valueTypeProperty);
                store.assignValueSource.owner = 1;
            } else {
                store.assignValueSource.property = genPropertyData(prop);
                store.assignValueSource.owner = 0;
            }
            QDeclarativeType *valueType = toQmlType(v->object);
            store.assignValueSource.castValue = valueType->propertyValueSourceCast();
            output->addInstruction(store);

        } else if (v->type == Value::ValueInterceptor) {
            genObject(v->object);

            QDeclarativeInstruction store;
            store.setType(QDeclarativeInstruction::StoreValueInterceptor);
            if (valueTypeProperty) {
                store.assignValueInterceptor.property = genValueTypeData(prop, valueTypeProperty);
                store.assignValueInterceptor.owner = 1;
            } else {
                store.assignValueInterceptor.property = genPropertyData(prop);
                store.assignValueInterceptor.owner = 0;
            }
            QDeclarativeType *valueType = toQmlType(v->object);
            store.assignValueInterceptor.castValue = valueType->propertyValueInterceptorCast();
            output->addInstruction(store);
        }

    }
}

bool QDeclarativeCompiler::buildIdProperty(QDeclarativeParser::Property *prop,
                                  QDeclarativeParser::Object *obj)
{
    if (prop->value ||
        prop->values.isMany() ||
        prop->values.first()->object)
        COMPILE_EXCEPTION(prop, tr("Invalid use of id property"));

    QDeclarativeParser::Value *idValue = prop->values.first();
    QString val = idValue->primitive();

    COMPILE_CHECK(checkValidId(idValue, val));

    if (compileState->ids.value(val))
        COMPILE_EXCEPTION(prop, tr("id is not unique"));

    prop->values.first()->type = Value::Id;

    obj->id = val;
    addId(val, obj);

    return true;
}

void QDeclarativeCompiler::addId(const QString &id, QDeclarativeParser::Object *obj)
{
    Q_ASSERT(!compileState->ids.value(id));
    Q_ASSERT(obj->id == id);
    obj->idIndex = compileState->ids.count();
    compileState->ids.append(obj);
}

void QDeclarativeCompiler::addBindingReference(BindingReference *ref)
{
    Q_ASSERT(ref->value && !ref->value->bindingReference);
    ref->value->bindingReference = ref;
    compileState->bindings.prepend(ref);
}

void QDeclarativeCompiler::saveComponentState()
{
    Q_ASSERT(compileState->root);
    Q_ASSERT(compileState->root->componentCompileState == 0);

    compileState->root->componentCompileState = compileState;

    if (componentStats) 
        componentStats->savedComponentStats.append(componentStats->componentStat);
}

QDeclarativeCompilerTypes::ComponentCompileState *
QDeclarativeCompiler::componentState(QDeclarativeParser::Object *obj)
{
    Q_ASSERT(obj->componentCompileState);
    return obj->componentCompileState;
}

// Build attached property object.  In this example,
// Text {
//    GridView.row: 10
// }
// GridView is an attached property object.
bool QDeclarativeCompiler::buildAttachedProperty(QDeclarativeParser::Property *prop,
                                        QDeclarativeParser::Object *obj,
                                        const BindingContext &ctxt)
{
    Q_ASSERT(prop->value);
    Q_ASSERT(prop->index != -1); // This is set in buildProperty()

    obj->addAttachedProperty(prop);

    COMPILE_CHECK(buildSubObject(prop->value, ctxt.incr()));

    return true;
}


// Build "grouped" properties. In this example:
// Text {
//     font.pointSize: 12
//     font.family: "Helvetica"
// }
// font is a nested property.  pointSize and family are not.
bool QDeclarativeCompiler::buildGroupedProperty(QDeclarativeParser::Property *prop,
                                                QDeclarativeParser::Object *obj,
                                                const BindingContext &ctxt)
{
    Q_ASSERT(prop->type != 0);
    Q_ASSERT(prop->index != -1);

    if (QDeclarativeValueTypeFactory::isValueType(prop->type)) {
        if (prop->type >= 0 && enginePrivate->valueTypes[prop->type]) {

            if (!prop->values.isEmpty()) {
                if (prop->values.first()->location < prop->value->location) {
                    COMPILE_EXCEPTION(prop->value, tr( "Property has already been assigned a value"));
                } else {
                    COMPILE_EXCEPTION(prop->values.first(), tr( "Property has already been assigned a value"));
                }
            }

            if (!obj->metaObject()->property(prop->index).isWritable()) {
                COMPILE_EXCEPTION(prop, tr( "Invalid property assignment: \"%1\" is a read-only property").arg(prop->name().toString()));
            }


            if (prop->isAlias) {
                for (Property *vtProp = prop->value->properties.first(); vtProp; vtProp = prop->value->properties.next(vtProp)) {
                    vtProp->isAlias = true;
                }
            }

            COMPILE_CHECK(buildValueTypeProperty(enginePrivate->valueTypes[prop->type],
                                                 prop->value, obj, ctxt.incr()));
            obj->addValueTypeProperty(prop);
        } else {
            COMPILE_EXCEPTION(prop, tr("Invalid grouped property access"));
        }

    } else {
        // Load the nested property's meta type
        prop->value->metatype = enginePrivate->metaObjectForType(prop->type);
        if (!prop->value->metatype)
            COMPILE_EXCEPTION(prop, tr("Invalid grouped property access"));

        if (!prop->values.isEmpty()) 
            COMPILE_EXCEPTION(prop->values.first(), tr( "Cannot assign a value directly to a grouped property"));

        obj->addGroupedProperty(prop);

        COMPILE_CHECK(buildSubObject(prop->value, ctxt.incr()));
    }

    return true;
}

bool QDeclarativeCompiler::buildValueTypeProperty(QObject *type,
                                                  QDeclarativeParser::Object *obj,
                                                  QDeclarativeParser::Object *baseObj,
                                                  const BindingContext &ctxt)
{
    if (obj->defaultProperty)
        COMPILE_EXCEPTION(obj, tr("Invalid property use"));
    obj->metatype = type->metaObject();

    for (Property *prop = obj->properties.first(); prop; prop = obj->properties.next(prop)) {

        QDeclarativePropertyCache::Data *d = property(obj, prop->name());
        if (d == 0) 
            COMPILE_EXCEPTION(prop, tr("Cannot assign to non-existent property \"%1\"").arg(prop->name().toString()));

        prop->index = d->coreIndex;
        prop->type = d->propType;
        prop->core = *d;
        prop->isValueTypeSubProperty = true;

        if (prop->value)
            COMPILE_EXCEPTION(prop, tr("Property assignment expected"));

        if (prop->values.isMany()) {
            COMPILE_EXCEPTION(prop, tr("Single property assignment expected"));
        } else if (!prop->values.isEmpty()) {
            QDeclarativeParser::Value *value = prop->values.first();

            if (value->object) {
                COMPILE_EXCEPTION(prop, tr("Unexpected object assignment"));
            } else if (value->value.isScript()) {
                // ### Check for writability

                //optimization for <Type>.<EnumValue> enum assignments
                bool isEnumAssignment = false;

                if (prop->core.isEnum()) 
                    COMPILE_CHECK(testQualifiedEnumAssignment(obj->metatype->property(prop->index), obj, 
                                                              value, &isEnumAssignment));

                if (isEnumAssignment) {
                    value->type = Value::Literal;
                } else {
                    BindingReference *reference = pool->New<BindingReference>();
                    reference->expression = value->value;
                    reference->property = prop;
                    reference->value = value;
                    reference->bindingContext = ctxt;
                    reference->bindingContext.owner++;
                    addBindingReference(reference);
                    value->type = Value::PropertyBinding;
                }
            } else  {
                COMPILE_CHECK(testLiteralAssignment(prop, value));
                value->type = Value::Literal;
            }
        }

        for (Value *v = prop->onValues.first(); v; v = Property::ValueList::next(v)) {
            Q_ASSERT(v->object);

            COMPILE_CHECK(buildPropertyOnAssignment(prop, obj, baseObj, v, ctxt)); 
        }

        obj->addValueProperty(prop);
    }

    return true;
}

// Build assignments to QML lists.  QML lists are properties of type
// QDeclarativeListProperty<T>.  List properties can accept a list of 
// objects, or a single binding.
bool QDeclarativeCompiler::buildListProperty(QDeclarativeParser::Property *prop,
                                             QDeclarativeParser::Object *obj,
                                             const BindingContext &ctxt)
{
    Q_ASSERT(enginePrivate->isList(prop->type));

    int t = prop->type;

    obj->addValueProperty(prop);

    int listType = enginePrivate->listType(t);
    bool listTypeIsInterface = QDeclarativeMetaType::isInterface(listType);

    bool assignedBinding = false;
    for (Value *v = prop->values.first(); v; v = Property::ValueList::next(v)) {
        if (v->object) {
            v->type = Value::CreatedObject;
            COMPILE_CHECK(buildObject(v->object, ctxt));

            // We check object coercian here.  We check interface assignment
            // at runtime.
            if (!listTypeIsInterface) {
                if (!canCoerce(listType, v->object)) {
                    COMPILE_EXCEPTION(v, tr("Cannot assign object to list"));
                }
            }

        } else if (v->value.isScript()) {
            if (assignedBinding)
                COMPILE_EXCEPTION(v, tr("Can only assign one binding to lists"));

            assignedBinding = true;
            COMPILE_CHECK(buildBinding(v, prop, ctxt));
            v->type = Value::PropertyBinding;
        } else {
            COMPILE_EXCEPTION(v, tr("Cannot assign primitives to lists"));
        }
    }

    return true;
}

// Compiles an assignment to a QDeclarativeScriptString property
bool QDeclarativeCompiler::buildScriptStringProperty(QDeclarativeParser::Property *prop,
                                            QDeclarativeParser::Object *obj,
                                            const BindingContext &ctxt)
{
    if (prop->values.isMany())
        COMPILE_EXCEPTION(prop->values.first()->nextValue, tr( "Cannot assign multiple values to a script property"));

    if (prop->values.first()->object)
        COMPILE_EXCEPTION(prop->values.first(), tr( "Invalid property assignment: script expected"));

    prop->scriptStringScope = ctxt.stack;
    obj->addScriptStringProperty(prop);

    return true;
}

// Compile regular property assignments of the form "property: <value>"
bool QDeclarativeCompiler::buildPropertyAssignment(QDeclarativeParser::Property *prop,
                                          QDeclarativeParser::Object *obj,
                                          const BindingContext &ctxt)
{
    obj->addValueProperty(prop);

    if (prop->values.isMany())
        COMPILE_EXCEPTION(prop->values.first(), tr( "Cannot assign multiple values to a singular property") );

    for (Value *v = prop->values.first(); v; v = Property::ValueList::next(v)) {
        if (v->object) {

            COMPILE_CHECK(buildPropertyObjectAssignment(prop, obj, v, ctxt));

        } else {

            COMPILE_CHECK(buildPropertyLiteralAssignment(prop, obj, v, ctxt));

        }
    }

    for (Value *v = prop->onValues.first(); v; v = Property::ValueList::next(v)) {
        Q_ASSERT(v->object);
        COMPILE_CHECK(buildPropertyOnAssignment(prop, obj, obj, v, ctxt));
    }

    return true;
}

// Compile assigning a single object instance to a regular property
bool QDeclarativeCompiler::buildPropertyObjectAssignment(QDeclarativeParser::Property *prop,
                                                         QDeclarativeParser::Object *obj,
                                                         QDeclarativeParser::Value *v,
                                                         const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);
    Q_ASSERT(v->object->type != -1);

    if (!obj->metaObject()->property(prop->index).isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(prop->name().toString()));

    if (QDeclarativeMetaType::isInterface(prop->type)) {

        // Assigning an object to an interface ptr property
        COMPILE_CHECK(buildObject(v->object, ctxt));

        v->type = Value::CreatedObject;

    } else if (prop->type == QMetaType::QVariant) {

        // Assigning an object to a QVariant
        COMPILE_CHECK(buildObject(v->object, ctxt));

        v->type = Value::CreatedObject;
    } else {
        // Normally buildObject() will set this up, but we need the static
        // meta object earlier to test for assignability.  It doesn't matter
        // that there may still be outstanding synthesized meta object changes
        // on this type, as they are not relevant for assignability testing
        v->object->metatype = output->types.at(v->object->type).metaObject();
        Q_ASSERT(v->object->metaObject());

        // We want to raw metaObject here as the raw metaobject is the
        // actual property type before we applied any extensions that might
        // effect the properties on the type, but don't effect assignability
        const QMetaObject *propertyMetaObject = enginePrivate->rawMetaObjectForType(prop->type);

        // Will be true if the assgned type inherits propertyMetaObject
        bool isAssignable = false;
        // Determine isAssignable value
        if (propertyMetaObject) {
            const QMetaObject *c = v->object->metatype;
            while(c) {
                isAssignable |= (QDeclarativePropertyPrivate::equal(c, propertyMetaObject));
                c = c->superClass();
            }
        }

        if (isAssignable) {
            // Simple assignment
            COMPILE_CHECK(buildObject(v->object, ctxt));

            v->type = Value::CreatedObject;
        } else if (propertyMetaObject == &QDeclarativeComponent::staticMetaObject) {
            // Automatic "Component" insertion
            QDeclarativeParser::Object *root = v->object;
            QDeclarativeParser::Object *component = pool->New<Object>();
            component->type = componentTypeRef();
            component->typeName = "Qt/Component";
            component->metatype = &QDeclarativeComponent::staticMetaObject;
            component->location = root->location;
            QDeclarativeParser::Value *componentValue = pool->New<Value>();
            componentValue->object = root;
            component->getDefaultProperty()->addValue(componentValue);
            v->object = component;
            COMPILE_CHECK(buildPropertyObjectAssignment(prop, obj, v, ctxt));
        } else {
            COMPILE_EXCEPTION(v->object, tr("Cannot assign object to property"));
        }
    }

    return true;
}

// Compile assigning a single object instance to a regular property using the "on" syntax.
//
// For example:
//     Item {
//         NumberAnimation on x { }
//     }
bool QDeclarativeCompiler::buildPropertyOnAssignment(QDeclarativeParser::Property *prop,
                                                     QDeclarativeParser::Object *obj,
                                                     QDeclarativeParser::Object *baseObj,
                                                     QDeclarativeParser::Value *v,
                                                     const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);
    Q_ASSERT(v->object->type != -1);

    if (!obj->metaObject()->property(prop->index).isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(prop->name().toString()));


    // Normally buildObject() will set this up, but we need the static
    // meta object earlier to test for assignability.  It doesn't matter
    // that there may still be outstanding synthesized meta object changes
    // on this type, as they are not relevant for assignability testing
    v->object->metatype = output->types.at(v->object->type).metaObject();
    Q_ASSERT(v->object->metaObject());

    // Will be true if the assigned type inherits QDeclarativePropertyValueSource
    bool isPropertyValue = false;
    // Will be true if the assigned type inherits QDeclarativePropertyValueInterceptor
    bool isPropertyInterceptor = false;
    if (QDeclarativeType *valueType = toQmlType(v->object)) {
        isPropertyValue = valueType->propertyValueSourceCast() != -1;
        isPropertyInterceptor = valueType->propertyValueInterceptorCast() != -1;
    }

    if (isPropertyValue || isPropertyInterceptor) {
        // Assign as a property value source
        COMPILE_CHECK(buildObject(v->object, ctxt));

        if (isPropertyInterceptor && prop->parent->synthdata.isEmpty())
            buildDynamicMeta(baseObj, ForceCreation);
        v->type = isPropertyValue ? Value::ValueSource : Value::ValueInterceptor;
    } else {
        COMPILE_EXCEPTION(v, tr("\"%1\" cannot operate on \"%2\"").arg(QString::fromUtf8(v->object->typeName)).arg(prop->name().toString()));
    }

    return true;
}

// Compile assigning a literal or binding to a regular property
bool QDeclarativeCompiler::buildPropertyLiteralAssignment(QDeclarativeParser::Property *prop,
                                                          QDeclarativeParser::Object *obj,
                                                          QDeclarativeParser::Value *v,
                                                          const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);

    if (v->value.isScript()) {

        //optimization for <Type>.<EnumValue> enum assignments
        if (prop->core.isEnum()) {
            bool isEnumAssignment = false;
            COMPILE_CHECK(testQualifiedEnumAssignment(obj->metaObject()->property(prop->index), obj, 
                                                      v, &isEnumAssignment));
            if (isEnumAssignment) {
                v->type = Value::Literal;
                return true;
            }
        }

        COMPILE_CHECK(buildBinding(v, prop, ctxt));

        v->type = Value::PropertyBinding;

    } else {

        COMPILE_CHECK(testLiteralAssignment(prop, v));

        v->type = Value::Literal;
    }

    return true;
}

bool QDeclarativeCompiler::testQualifiedEnumAssignment(const QMetaProperty &prop,
                                                       QDeclarativeParser::Object *obj,
                                                       QDeclarativeParser::Value *v,
                                                       bool *isAssignment)
{
    *isAssignment = false;
    if (!prop.isEnumType())
        return true;

    if (!prop.isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop.name())));

    QString string = v->value.asString();
    if (!QDeclarativeUtils::isUpper(string.at(0)))
        return true;

    QStringList parts = string.split(QLatin1Char('.'));
    if (parts.count() != 2)
        return true;

    QString typeName = parts.at(0);
    QDeclarativeType *type = 0;
    unit->imports().resolveType(typeName.toUtf8(), &type, 0, 0, 0, 0);

    //handle enums on value types (where obj->typeName is empty)
    QByteArray objTypeName = obj->typeName;
    if (objTypeName.isEmpty()) {
        QDeclarativeType *objType = toQmlType(obj);
        if (objType)
            objTypeName = objType->qmlTypeName();
    }

    if (!type)
        return true;

    QString enumValue = parts.at(1);
    int value = -1;

    if (objTypeName == type->qmlTypeName()) {
        // When these two match, we can short cut the search
        if (prop.isFlagType()) {
            value = prop.enumerator().keysToValue(enumValue.toUtf8().constData());
        } else {
            value = prop.enumerator().keyToValue(enumValue.toUtf8().constData());
        }
    } else {
        // Otherwise we have to search the whole type
        // This matches the logic in QV8TypeWrapper
        QByteArray enumName = enumValue.toUtf8();
        const QMetaObject *metaObject = type->baseMetaObject();
        for (int ii = metaObject->enumeratorCount() - 1; value == -1 && ii >= 0; --ii) {
            QMetaEnum e = metaObject->enumerator(ii);
            value = e.keyToValue(enumName.constData());
        }
    }

    if (value == -1)
        return true;

    v->type = Value::Literal;
    v->value = QDeclarativeParser::Variant((double)value);
    *isAssignment = true;

    return true;
}

struct StaticQtMetaObject : public QObject
{
    static const QMetaObject *get()
        { return &static_cast<StaticQtMetaObject*> (0)->staticQtMetaObject; }
};

// Similar logic to above, but not knowing target property.
int QDeclarativeCompiler::evaluateEnum(const QByteArray& script) const
{
    int dot = script.indexOf('.');
    if (dot > 0) {
        const QByteArray &scope = script.left(dot);
        QDeclarativeType *type = 0;
        unit->imports().resolveType(scope, &type, 0, 0, 0, 0);
        if (!type && scope != "Qt")
            return -1;
        const QMetaObject *mo = type ? type->metaObject() : StaticQtMetaObject::get();
        const char *key = script.constData() + dot+1;
        int i = mo->enumeratorCount();
        while (i--) {
            int v = mo->enumerator(i).keyToValue(key);
            if (v >= 0)
                return v;
        }
    }
    return -1;
}

const QMetaObject *QDeclarativeCompiler::resolveType(const QByteArray& name) const
{
    QDeclarativeType *qmltype = 0;
    if (!unit->imports().resolveType(name, &qmltype, 0, 0, 0, 0)) 
        return 0;
    if (!qmltype)
        return 0;
    return qmltype->metaObject();
}

// similar to logic of completeComponentBuild, but also sticks data
// into primitives at the end
int QDeclarativeCompiler::rewriteBinding(const QString& expression, const QString& name)
{
    QDeclarativeRewrite::RewriteBinding rewriteBinding;
    rewriteBinding.setName(QLatin1Char('$') + name.mid(name.lastIndexOf('.') + 1));

    QString rewrite = rewriteBinding(expression, 0, 0);

    return output->indexForString(rewrite);
}

// Ensures that the dynamic meta specification on obj is valid
bool QDeclarativeCompiler::checkDynamicMeta(QDeclarativeParser::Object *obj)
{
    // XXX aakenned - inefficient copying of the string ref
    QStringHash<bool> propNames;
    QSet<QByteArray> methodNames;
    bool seenDefaultProperty = false;

    // Check properties
    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        const QDeclarativeParser::Object::DynamicProperty &prop =
            obj->dynamicProperties.at(ii);

        if (prop.isDefaultProperty) {
            if (seenDefaultProperty)
                COMPILE_EXCEPTION(&prop, tr("Duplicate default property"));
            seenDefaultProperty = true;
        }

        if (propNames.contains(prop.name))
            COMPILE_EXCEPTION(&prop, tr("Duplicate property name"));

        if (QDeclarativeUtils::isUpper(prop.name.at(0)))
            COMPILE_EXCEPTION(&prop, tr("Property names cannot begin with an upper case letter"));

        if (enginePrivate->v8engine()->illegalNames().contains(prop.name))
            COMPILE_EXCEPTION(&prop, tr("Illegal property name"));

        propNames.insert(prop.name.toString(), true);
    }

    for (Object::DynamicSignal *s = obj->dynamicSignals.first(); s; s = obj->dynamicSignals.next(s)) {
        const QDeclarativeScript::Object::DynamicSignal &currSig = *s;

        if (methodNames.testAndSet(currSig.name.hash())) {
            for (Object::DynamicSignal *s2 = obj->dynamicSignals.first(); s2 != s;
                 s2 = obj->dynamicSignals.next(s2)) {
                if (s2->name == currSig.name)
                    COMPILE_EXCEPTION(&currSig, tr("Duplicate signal name"));
            }
        }

        if (currSig.name.at(0).isUpper())
            COMPILE_EXCEPTION(&currSig, tr("Signal names cannot begin with an upper case letter"));
        if (enginePrivate->v8engine()->illegalNames().contains(currSig.name))
            COMPILE_EXCEPTION(&currSig, tr("Illegal signal name"));
    }

    for (Object::DynamicSlot *s = obj->dynamicSlots.first(); s; s = obj->dynamicSlots.next(s)) {
        const QDeclarativeScript::Object::DynamicSlot &currSlot = *s;

        if (methodNames.testAndSet(currSlot.name.hash())) {
            for (Object::DynamicSignal *s2 = obj->dynamicSignals.first(); s2;
                 s2 = obj->dynamicSignals.next(s2)) {
                if (s2->name == currSlot.name)
                    COMPILE_EXCEPTION(&currSlot, tr("Duplicate method name"));
            }
            for (Object::DynamicSlot *s2 = obj->dynamicSlots.first(); s2 != s;
                 s2 = obj->dynamicSlots.next(s2)) {
                if (s2->name == currSlot.name)
                    COMPILE_EXCEPTION(&currSlot, tr("Duplicate method name"));
            }
        }

        if (currSlot.name.at(0).isUpper())
            COMPILE_EXCEPTION(&currSlot, tr("Method names cannot begin with an upper case letter"));
        if (enginePrivate->v8engine()->illegalNames().contains(currSlot.name))
            COMPILE_EXCEPTION(&currSlot, tr("Illegal method name"));
    }

    return true;
}

bool QDeclarativeCompiler::mergeDynamicMetaProperties(QDeclarativeParser::Object *obj)
{
    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        const Object::DynamicProperty &p = obj->dynamicProperties.at(ii);

        if (!p.defaultValue || p.type == Object::DynamicProperty::Alias)
            continue;

        Property *property = 0;
        if (p.isDefaultProperty) {
            property = obj->getDefaultProperty();
        } else {
            property = obj->getProperty(p.name);
            if (!property->values.isEmpty()) 
                COMPILE_EXCEPTION(property, tr("Property value set multiple times"));
        }

        if (property->value)
            COMPILE_EXCEPTION(property, tr("Invalid property nesting"));

        property->values.append(p.defaultValue->values);
    }
    return true;
}

Q_GLOBAL_STATIC(QAtomicInt, classIndexCounter)

bool QDeclarativeCompiler::buildDynamicMeta(QDeclarativeParser::Object *obj, DynamicMetaMode mode)
{
    Q_ASSERT(obj);
    Q_ASSERT(obj->metatype);

    if (mode != ForceCreation &&
        obj->dynamicProperties.isEmpty() &&
        obj->dynamicSignals.isEmpty() &&
        obj->dynamicSlots.isEmpty())
        return true;

    bool resolveAlias = (mode == ResolveAliases);

    const Object::DynamicProperty *defaultProperty = 0;
    int aliasCount = 0;

    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        const Object::DynamicProperty &p = obj->dynamicProperties.at(ii);

        if (p.type == Object::DynamicProperty::Alias)
            aliasCount++;

        if (p.isDefaultProperty && 
            (resolveAlias || p.type != Object::DynamicProperty::Alias))
            defaultProperty = &p;

        if (!resolveAlias) {
            // No point doing this for both the alias and non alias cases
            QDeclarativePropertyCache::Data *d = property(obj, p.name);
            if (d && d->isFinal())
                COMPILE_EXCEPTION(&p, tr("Cannot override FINAL property"));
        }
    }

    bool buildData = resolveAlias || aliasCount == 0;

    QByteArray dynamicData;
    if (buildData) {
        typedef QDeclarativeVMEMetaData VMD;

        dynamicData = QByteArray(sizeof(QDeclarativeVMEMetaData) +
                                 (obj->dynamicProperties.count() - aliasCount) * sizeof(VMD::PropertyData) +
                                 obj->dynamicSlots.count() * sizeof(VMD::MethodData) +
                                 aliasCount * sizeof(VMD::AliasData), 0);
    }

    int uniqueClassId = classIndexCounter()->fetchAndAddRelaxed(1);

    QByteArray newClassName = obj->metatype->className();
    newClassName.append("_QML_");
    newClassName.append(QByteArray::number(uniqueClassId));

    if (compileState->root == obj && !compileState->nested) {
        QString path = output->url.path();
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        if (lastSlash > -1) {
            QString nameBase = path.mid(lastSlash + 1, path.length()-lastSlash-5);
            if (!nameBase.isEmpty() && QDeclarativeUtils::isUpper(nameBase.at(0)))
                newClassName = nameBase.toUtf8() + "_QMLTYPE_" + QByteArray::number(uniqueClassId);
        }
    }

    QFastMetaBuilder builder;
    QFastMetaBuilder::StringRef classNameRef = builder.init(newClassName.length(), 
            obj->dynamicProperties.count() - (resolveAlias?0:aliasCount),
            obj->dynamicSlots.count(),
            obj->dynamicSignals.count() + obj->dynamicProperties.count(),
            defaultProperty?1:0);

    struct TypeData {
        Object::DynamicProperty::Type dtype;
        int metaType;
        const char *cppType;
    } builtinTypes[] = {
        { Object::DynamicProperty::Variant, 0, "QVariant" },
        { Object::DynamicProperty::Int, QMetaType::Int, "int" },
        { Object::DynamicProperty::Bool, QMetaType::Bool, "bool" },
        { Object::DynamicProperty::Real, QMetaType::Double, "double" },
        { Object::DynamicProperty::String, QMetaType::QString, "QString" },
        { Object::DynamicProperty::Url, QMetaType::QUrl, "QUrl" },
        { Object::DynamicProperty::Color, QMetaType::QColor, "QColor" },
        { Object::DynamicProperty::Time, QMetaType::QTime, "QTime" },
        { Object::DynamicProperty::Date, QMetaType::QDate, "QDate" },
        { Object::DynamicProperty::DateTime, QMetaType::QDateTime, "QDateTime" },
    };
    static const int builtinTypeCount = sizeof(builtinTypes) / sizeof(TypeData);
    QFastMetaBuilder::StringRef typeRefs[builtinTypeCount]; 

    // Reserve dynamic properties
    if (obj->dynamicProperties.count()) {
        typedef QDeclarativeVMEMetaData VMD;

        int effectivePropertyIndex = 0;
        for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
            Object::DynamicProperty &p = obj->dynamicProperties[ii];

            // Reserve space for name
            p.nameRef = builder.newString(p.name.utf8length());

            int propertyType = 0;
            bool readonly = false;
            QFastMetaBuilder::StringRef typeRef;

            if (p.type == Object::DynamicProperty::Alias) {
                continue;
            } else if (p.type < builtinTypeCount) {
                Q_ASSERT(builtinTypes[p.type].dtype == p.type);
                propertyType = builtinTypes[p.type].metaType;
                if (typeRefs[p.type].isEmpty()) 
                    typeRefs[p.type] = builder.newString(strlen(builtinTypes[p.type].cppType));
                typeRef = typeRefs[p.type];
                if (p.type == Object::DynamicProperty::Variant)
                    propertyType = qMetaTypeId<QVariant>();

            } else {
                Q_ASSERT(p.type == Object::DynamicProperty::CustomList ||
                         p.type == Object::DynamicProperty::Custom);

                // XXX don't double resolve this in the case of an alias run

                QByteArray customTypeName;
                QDeclarativeType *qmltype = 0;
                QString url;
                if (!unit->imports().resolveType(p.customType.toUtf8(), &qmltype, &url, 0, 0, 0)) 
                    COMPILE_EXCEPTION(&p, tr("Invalid property type"));

                if (!qmltype) {
                    QDeclarativeTypeData *tdata = enginePrivate->typeLoader.get(QUrl(url));
                    Q_ASSERT(tdata);
                    Q_ASSERT(tdata->isComplete());

                    QDeclarativeCompiledData *data = tdata->compiledData();
                    customTypeName = data->root->className();
                    data->release();
                    tdata->release();
                } else {
                    customTypeName = qmltype->typeName();
                }

                if (p.type == Object::DynamicProperty::Custom) {
                    customTypeName += '*';
                    propertyType = QMetaType::QObjectStar;
                } else {
                    readonly = true;
                    customTypeName = QByteArray("QDeclarativeListProperty<") + customTypeName + QByteArray(">");
                    propertyType = qMetaTypeId<QDeclarativeListProperty<QObject> >();
                }

                p.resolvedCustomTypeName = pool->NewByteArray(customTypeName);
                p.typeRef = builder.newString(customTypeName.length());
                typeRef = p.typeRef;
            }

            if (buildData) {
                VMD *vmd = (QDeclarativeVMEMetaData *)dynamicData.data();
                vmd->propertyCount++;
                (vmd->propertyData() + effectivePropertyIndex)->propertyType = propertyType;
            }

            if (p.type < builtinTypeCount)
                builder.setProperty(effectivePropertyIndex, p.nameRef, typeRef, (QMetaType::Type)propertyType, 
                                    readonly?QFastMetaBuilder::None:QFastMetaBuilder::Writable, 
                                    effectivePropertyIndex);
            else 
                builder.setProperty(effectivePropertyIndex, p.nameRef, typeRef, 
                                    readonly?QFastMetaBuilder::None:QFastMetaBuilder::Writable, 
                                    effectivePropertyIndex);

            p.changedSignatureRef = builder.newString(p.name.utf8length() + strlen("Changed()"));
            builder.setSignal(effectivePropertyIndex, p.changedSignatureRef);

            effectivePropertyIndex++;
        }
        
        if (aliasCount) {
            int aliasIndex = 0;
            for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
                Object::DynamicProperty &p = obj->dynamicProperties[ii];
                if (p.type == Object::DynamicProperty::Alias) {
                    if (resolveAlias) {
                        Q_ASSERT(buildData);
                        ((QDeclarativeVMEMetaData *)dynamicData.data())->aliasCount++;
                        COMPILE_CHECK(compileAlias(builder, dynamicData, obj, effectivePropertyIndex, aliasIndex, p));
                    }
                    // Even if we aren't resolving the alias, we need a fake signal so that the 
                    // metaobject remains consistent across the resolve and non-resolve alias runs
                    p.changedSignatureRef = builder.newString(p.name.utf8length() + strlen("Changed()"));
                    builder.setSignal(effectivePropertyIndex, p.changedSignatureRef);
                    effectivePropertyIndex++;
                    aliasIndex++;
                }
            }
        }
    }

    // Reserve default property
    QFastMetaBuilder::StringRef defPropRef;
    if (defaultProperty) {
        defPropRef = builder.newString(strlen("DefaultProperty"));
        builder.setClassInfo(0, defPropRef, defaultProperty->nameRef);
    }

    // Reserve dynamic signals
    for (int ii = 0; ii < obj->dynamicSignals.count(); ++ii) {
        Object::DynamicSignal &s = obj->dynamicSignals[ii];

        int paramCount = s.parameterNames.count();

        int signatureSize = s.name.length() + 2 /* paren */;
        int namesSize = 0;
        if (paramCount) signatureSize += s.parameterTypesLength() + (paramCount - 1) /* commas */;
        if (paramCount) namesSize += s.parameterNamesLength() + (paramCount - 1) /* commas */;

        s.signatureRef = builder.newString(signatureSize);
        if (namesSize) s.parameterNamesRef = builder.newString(namesSize);

        if (buildData)
            ((QDeclarativeVMEMetaData *)dynamicData.data())->signalCount++;
        
        builder.setSignal(ii + obj->dynamicProperties.count(), s.signatureRef, s.parameterNamesRef);
    }

    // Reserve dynamic slots
    if (obj->dynamicSlots.count()) {

        // Allocate QVariant string
        if (typeRefs[0].isEmpty()) 
            typeRefs[0] = builder.newString(strlen(builtinTypes[0].cppType));

        typedef QDeclarativeVMEMetaData VMD;

        for (int ii = 0; ii < obj->dynamicSlots.count(); ++ii) {
            Object::DynamicSlot &s = obj->dynamicSlots[ii];
            int paramCount = s.parameterNames.count();

            int signatureSize = s.name.length() + 2 /* paren */;
            int namesSize = 0; 
            if (paramCount) signatureSize += (paramCount * strlen("QVariant") + (paramCount - 1));
            if (paramCount) namesSize += s.parameterNamesLength() + (paramCount - 1 /* commas */); 

            s.signatureRef = builder.newString(signatureSize);
            if (namesSize) s.parameterNamesRef = builder.newString(namesSize);

            builder.setMethod(ii, s.signatureRef, s.parameterNamesRef, typeRefs[0]);

            if (buildData) {
                QString funcScript;
                funcScript.reserve(strlen("(function ") + s.name.length() + 1 /* lparen */ + 
                        namesSize + 1 /* rparen */ + s.body.length() + 1 /* rparen */);
                funcScript = QLatin1String("(function ") + s.name + QLatin1Char('(');
                for (int jj = 0; jj < paramCount; ++jj) {
                    if (jj) funcScript.append(QLatin1Char(','));
                    funcScript.append(QLatin1String(s.parameterNames.at(jj)));
                }
                funcScript += QLatin1Char(')') + s.body + QLatin1Char(')');

                VMD::MethodData methodData = { s.parameterNames.count(), 0, 
                                               funcScript.length(), 
                                               s.location.start.line };

                VMD *vmd = (QDeclarativeVMEMetaData *)dynamicData.data();
                vmd->methodCount++;

                VMD::MethodData &md = *(vmd->methodData() + ii);
                md = methodData;
                md.bodyOffset = dynamicData.size();

                dynamicData.append((const char *)funcScript.constData(),
                                   (funcScript.length() * sizeof(QChar)));
            }

        }
    }

    // Now allocate used builtin types
    for (int ii = 0; ii < builtinTypeCount; ++ii) {
        if (!typeRefs[ii].isEmpty())
            typeRefs[ii].load(builtinTypes[ii].cppType);
    }

    // Now allocate properties
    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        Object::DynamicProperty &p = obj->dynamicProperties[ii];

        char *d = p.changedSignatureRef.data();
        p.name.writeUtf8(d);
        strcpy(d + p.name.utf8length(), "Changed()");

        if (p.type == Object::DynamicProperty::Alias && !resolveAlias)
            continue;

        p.nameRef.load(p.name);

        if (p.type >= builtinTypeCount) {
            Q_ASSERT(p.resolvedCustomTypeName);
            p.typeRef.load(*p.resolvedCustomTypeName);
        }
    }

    // Allocate default property if necessary
    if (defaultProperty) 
        strcpy(defPropRef.data(), "DefaultProperty");

    // Now allocate signals
    for (int ii = 0; ii < obj->dynamicSignals.count(); ++ii) {
        Object::DynamicSignal &s = obj->dynamicSignals[ii];

        char *d = s.signatureRef.data();
        char *d2 = s.parameterNamesRef.isEmpty()?0:s.parameterNamesRef.data();
        strcpy(d, s.name.constData());
        d += s.name.length();
        *d++ = '('; 

        for (int jj = 0; jj < s.parameterNames.count(); ++jj) {
            if (jj != 0) { *d++ = ','; *d2++ = ','; }
            strcpy(d, s.parameterTypes.at(jj).constData());
            d += s.parameterTypes.at(jj).length();
            strcpy(d2, s.parameterNames.at(jj).constData());
            d2 += s.parameterNames.at(jj).length();
        }
        *d++ = ')';
        *d = 0;
        if (d2) *d2 = 0;
    }

    // Now allocate methods
    for (int ii = 0; ii < obj->dynamicSlots.count(); ++ii) {
        Object::DynamicSlot &s = obj->dynamicSlots[ii];
        char *d = s.signatureRef.data();
        char *d2 = s.parameterNamesRef.isEmpty()?0:s.parameterNamesRef.data();
        strcpy(d, s.name.constData());
        d += s.name.length();
        *d++ = '('; 
        for (int jj = 0; jj < s.parameterNames.count(); ++jj) {
            if (jj != 0) { *d++ = ','; *d2++ = ','; }
            strcpy(d, "QVariant");
            d += strlen("QVariant");
            strcpy(d2, s.parameterNames.at(jj).constData());
            d2 += s.parameterNames.at(jj).length();
        }
        *d++ = ')';
        *d = 0;
        if (d2) *d2 = 0;
    }

    // Now allocate class name
    classNameRef.load(newClassName);

    obj->metadata = builder.toData();
    builder.fromData(&obj->extObject, obj->metatype, obj->metadata);

    if (mode == IgnoreAliases && aliasCount) 
        compileState->aliasingObjects.append(obj);

    obj->synthdata = dynamicData;

    if (obj->synthCache) {
        obj->synthCache->release();
        obj->synthCache = 0;
    }

    if (obj->type != -1) {
        QDeclarativePropertyCache *cache = output->types[obj->type].createPropertyCache(engine)->copy();
        cache->append(engine, &obj->extObject, QDeclarativePropertyCache::Data::NoFlags,
                      QDeclarativePropertyCache::Data::IsVMEFunction, 
                      QDeclarativePropertyCache::Data::IsVMESignal);
        obj->synthCache = cache;
    }

    return true;
}

bool QDeclarativeCompiler::checkValidId(QDeclarativeParser::Value *v, const QString &val)
{
    if (val.isEmpty()) 
        COMPILE_EXCEPTION(v, tr( "Invalid empty ID"));

    QChar ch = val.at(0);
    if (QDeclarativeUtils::isLetter(ch) && !QDeclarativeUtils::isLower(ch))
        COMPILE_EXCEPTION(v, tr( "IDs cannot start with an uppercase letter"));

    QChar u(QLatin1Char('_'));
    if (!QDeclarativeUtils::isLetter(ch) && ch != u)
        COMPILE_EXCEPTION(v, tr( "IDs must start with a letter or underscore"));

    for (int ii = 1; ii < val.count(); ++ii) {
        ch = val.at(ii);
        if (!QDeclarativeUtils::isLetterOrNumber(ch) && ch != u)
            COMPILE_EXCEPTION(v, tr( "IDs must contain only letters, numbers, and underscores"));
    }

    if (enginePrivate->v8engine()->illegalNames().contains(val))
        COMPILE_EXCEPTION(v, tr( "ID illegally masks global JavaScript property"));

    return true;
}

#include <qdeclarativejsparser_p.h>

static QStringList astNodeToStringList(QDeclarativeJS::AST::Node *node)
{
    if (node->kind == QDeclarativeJS::AST::Node::Kind_IdentifierExpression) {
        QString name =
            static_cast<QDeclarativeJS::AST::IdentifierExpression *>(node)->name.toString();
        return QStringList() << name;
    } else if (node->kind == QDeclarativeJS::AST::Node::Kind_FieldMemberExpression) {
        QDeclarativeJS::AST::FieldMemberExpression *expr = static_cast<QDeclarativeJS::AST::FieldMemberExpression *>(node);

        QStringList rv = astNodeToStringList(expr->base);
        if (rv.isEmpty())
            return rv;
        rv.append(expr->name.toString());
        return rv;
    }
    return QStringList();
}

bool QDeclarativeCompiler::compileAlias(QFastMetaBuilder &builder,
                                        QByteArray &data,
                                        QDeclarativeParser::Object *obj,
                                        int propIndex, int aliasIndex,
                                        Object::DynamicProperty &prop)
{
    if (!prop.defaultValue)
        COMPILE_EXCEPTION(obj, tr("No property alias location"));

    if (!prop.defaultValue->values.isOne() ||
        prop.defaultValue->values.first()->object ||
        !prop.defaultValue->values.first()->value.isScript())
        COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));

    QDeclarativeJS::AST::Node *node = prop.defaultValue->values.first()->value.asAST();
    if (!node)
        COMPILE_EXCEPTION(obj, tr("No property alias location")); // ### Can this happen?

    QStringList alias = astNodeToStringList(node);

    if (alias.count() < 1 || alias.count() > 3)
        COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias reference. An alias reference must be specified as <id>, <id>.<property> or <id>.<value property>.<property>"));

    QDeclarativeParser::Object *idObject = compileState->ids.value(alias.at(0));
    if (!idObject)
        COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias reference. Unable to find id \"%1\"").arg(alias.at(0)));

    QByteArray typeName;

    int propIdx = -1;
    int flags = 0;
    int type = 0;
    bool writable = false;
    bool resettable = false;
    if (alias.count() == 2 || alias.count() == 3) {
        propIdx = indexOfProperty(idObject, alias.at(1));

        if (-1 == propIdx) {
            COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));
        } else if (propIdx > 0xFFFF) {
            COMPILE_EXCEPTION(prop.defaultValue, tr("Alias property exceeds alias bounds"));
        }

        QMetaProperty aliasProperty = idObject->metaObject()->property(propIdx);
        if (!aliasProperty.isScriptable())
            COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));

        writable = aliasProperty.isWritable();
        resettable = aliasProperty.isResettable();

        if (aliasProperty.type() < QVariant::UserType)
            type = aliasProperty.type();

        if (alias.count() == 3) {
            QDeclarativeValueType *valueType = enginePrivate->valueTypes[aliasProperty.type()];
            if (!valueType)
                COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));

            propIdx |= ((unsigned int)aliasProperty.type()) << 24;

            int valueTypeIndex = valueType->metaObject()->indexOfProperty(alias.at(2).toUtf8().constData());
            if (valueTypeIndex == -1)
                COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));
            Q_ASSERT(valueTypeIndex <= 0xFF);
            
            aliasProperty = valueType->metaObject()->property(valueTypeIndex);
            propIdx |= (valueTypeIndex << 16);
        }

        if (aliasProperty.isEnumType()) 
            typeName = "int";  // Avoid introducing a dependency on the aliased metaobject
        else
            typeName = aliasProperty.typeName();
    } else {
        Q_ASSERT(idObject->type != -1); // How else did it get an id?

        const QDeclarativeCompiledData::TypeReference &ref = output->types.at(idObject->type);
        if (ref.type)
            typeName = ref.type->typeName();
        else
            typeName = ref.component->root->className();

        typeName += '*';
    }

    if (typeName.endsWith('*'))
        flags |= QML_ALIAS_FLAG_PTR;

    QDeclarativeVMEMetaData::AliasData aliasData = { idObject->idIndex, propIdx, flags };

    typedef QDeclarativeVMEMetaData VMD;
    VMD *vmd = (QDeclarativeVMEMetaData *)data.data();
    *(vmd->aliasData() + aliasIndex) = aliasData;

    prop.nameRef = builder.newString(prop.name.length());
    prop.resolvedCustomTypeName = pool->NewByteArray(typeName);
    prop.typeRef = builder.newString(typeName.length());

    builder.setProperty(propIndex, prop.nameRef, prop.typeRef, (QMetaType::Type)type, 
                        (QFastMetaBuilder::PropertyFlag)(writable?int(QFastMetaBuilder::Writable):0), 
                        propIndex);

    return true;
}

bool QDeclarativeCompiler::buildBinding(QDeclarativeParser::Value *value,
                                        QDeclarativeParser::Property *prop,
                                        const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);
    Q_ASSERT(prop->parent);
    Q_ASSERT(prop->parent->metaObject());

    // XXX aakenned
    QMetaProperty mp = prop->parent->metaObject()->property(prop->index);
    if (!mp.isWritable() && !QDeclarativeMetaType::isList(prop->type))
        COMPILE_EXCEPTION(prop, tr("Invalid property assignment: \"%1\" is a read-only property").arg(prop->name().toString()));

    BindingReference *reference = pool->New<BindingReference>();
    reference->expression = value->value;
    reference->property = prop;
    reference->value = value;
    reference->bindingContext = ctxt;
    addBindingReference(reference);

    return true;
}

void QDeclarativeCompiler::genBindingAssignment(QDeclarativeParser::Value *binding,
                                                QDeclarativeParser::Property *prop,
                                                QDeclarativeParser::Object *obj,
                                                QDeclarativeParser::Property *valueTypeProperty)
{
    Q_UNUSED(obj);
    Q_ASSERT(binding->bindingReference);

    const BindingReference &ref = *binding->bindingReference;
    if (ref.dataType == BindingReference::V4) {
        QDeclarativeInstruction store;
        store.setType(QDeclarativeInstruction::StoreV4Binding);
        store.assignBinding.value = ref.compiledIndex;
        store.assignBinding.context = ref.bindingContext.stack;
        store.assignBinding.owner = ref.bindingContext.owner;
        if (valueTypeProperty) 
            store.assignBinding.property = (valueTypeProperty->index & 0xFFFF) |
                                           ((valueTypeProperty->type & 0xFF)) << 16 |
                                           ((prop->index & 0xFF) << 24);
        else 
            store.assignBinding.property = prop->index;
        store.assignBinding.line = binding->location.start.line;
        output->addInstruction(store);
    } else if (ref.dataType == BindingReference::V8) {
        QDeclarativeInstruction store;
        store.setType(QDeclarativeInstruction::StoreV8Binding);
        store.assignBinding.value = ref.compiledIndex;
        store.assignBinding.context = ref.bindingContext.stack;
        store.assignBinding.owner = ref.bindingContext.owner;
        store.assignBinding.line = binding->location.start.line;

        Q_ASSERT(ref.bindingContext.owner == 0 ||
                 (ref.bindingContext.owner != 0 && valueTypeProperty));
        if (ref.bindingContext.owner) {
            store.assignBinding.property = genValueTypeData(prop, valueTypeProperty);
        } else {
            store.assignBinding.property = genPropertyData(prop);
        }

        output->addInstruction(store);
    } else {
        QDeclarativeInstruction store;
        if (!prop->isAlias)
            store.setType(QDeclarativeInstruction::StoreBinding);
        else
            store.setType(QDeclarativeInstruction::StoreBindingOnAlias);
        store.assignBinding.value = output->indexForString(ref.rewrittenExpression);
        store.assignBinding.context = ref.bindingContext.stack;
        store.assignBinding.owner = ref.bindingContext.owner;
        store.assignBinding.line = binding->location.start.line;

        Q_ASSERT(ref.bindingContext.owner == 0 ||
                 (ref.bindingContext.owner != 0 && valueTypeProperty));
        if (ref.bindingContext.owner) {
            store.assignBinding.property = genValueTypeData(prop, valueTypeProperty);
        } else {
            store.assignBinding.property = genPropertyData(prop);
        }
        output->addInstruction(store);
    }
}

int QDeclarativeCompiler::genContextCache()
{
    if (compileState->ids.count() == 0)
        return -1;

    QDeclarativeIntegerCache *cache = new QDeclarativeIntegerCache();
    cache->reserve(compileState->ids.count());
    for (Object *o = compileState->ids.first(); o; o = compileState->ids.next(o)) 
        cache->add(o->id, o->idIndex);

    output->contextCaches.append(cache);
    return output->contextCaches.count() - 1;
}

int QDeclarativeCompiler::genValueTypeData(QDeclarativeParser::Property *valueTypeProp, 
                                  QDeclarativeParser::Property *prop)
{
    typedef QDeclarativePropertyPrivate QDPP;
    QByteArray data = QDPP::saveValueType(prop->parent->metaObject(), prop->index, 
                                          enginePrivate->valueTypes[prop->type]->metaObject(), 
                                          valueTypeProp->index, engine);

    return output->indexForByteArray(data);
}

int QDeclarativeCompiler::genPropertyData(QDeclarativeParser::Property *prop)
{
    typedef QDeclarativePropertyPrivate QDPP;
    QByteArray data = QDPP::saveProperty(prop->parent->metaObject(), prop->index, engine);

    return output->indexForByteArray(data);
}

bool QDeclarativeCompiler::completeComponentBuild()
{
    if (componentStats)
        componentStats->componentStat.ids = compileState->ids.count();

    for (Object *aliasObject = compileState->aliasingObjects.first(); aliasObject; 
         aliasObject = compileState->aliasingObjects.next(aliasObject)) 
        COMPILE_CHECK(buildDynamicMeta(aliasObject, ResolveAliases));

    QDeclarativeV4Compiler::Expression expr(unit->imports());
    expr.component = compileState->root;
    expr.ids = &compileState->ids;
    expr.importCache = output->importCache;

    QDeclarativeV4Compiler bindingCompiler;

    QList<BindingReference*> sharedBindings;

    for (BindingReference *b = compileState->bindings.first(); b; b = b->nextReference) {

        BindingReference &binding = *b;

        // ### We don't currently optimize for bindings on alias's - because 
        // of the solution to QTBUG-13719
        if (!binding.property->isAlias) {
            expr.context = binding.bindingContext.object;
            expr.property = binding.property;
            expr.expression = binding.expression;

            int index = bindingCompiler.compile(expr, enginePrivate);
            if (index != -1) {
                binding.dataType = BindingReference::V4;
                binding.compiledIndex = index;
                if (componentStats)
                    componentStats->componentStat.optimizedBindings.append(b->value->location);
                continue;
            } 
        }

        // Pre-rewrite the expression
        QString expression = binding.expression.asScript();

        QDeclarativeRewrite::RewriteBinding rewriteBinding;
        rewriteBinding.setName(QLatin1Char('$')+binding.property->name().toString());
        bool isSharable = false;
        binding.rewrittenExpression = rewriteBinding(binding.expression.asAST(), expression, &isSharable);

        if (isSharable && !binding.property->isAlias /* See above re alias */ &&
            binding.property->type != qMetaTypeId<QDeclarativeBinding*>()) {
            binding.dataType = BindingReference::V8;
            sharedBindings.append(b);
        } else {
            binding.dataType = BindingReference::QtScript;
        }

        if (componentStats)
            componentStats->componentStat.scriptBindings.append(b->value->location);
    }

    if (!sharedBindings.isEmpty()) {
        struct Sort {
            static bool lt(const BindingReference *lhs, const BindingReference *rhs)
            {
                return lhs->value->location.start.line < rhs->value->location.start.line;
            }
        };

        qSort(sharedBindings.begin(), sharedBindings.end(), Sort::lt);

        int startLineNumber = sharedBindings.at(0)->value->location.start.line;
        int lineNumber = startLineNumber;

        QString functionArray(QLatin1String("["));
        for (int ii = 0; ii < sharedBindings.count(); ++ii) {
            BindingReference *reference = sharedBindings.at(ii);
            QDeclarativeParser::Value *value = reference->value;
            const QString &expression = reference->rewrittenExpression;

            if (ii != 0) functionArray += QLatin1String(",");

            while (lineNumber < value->location.start.line) {
                lineNumber++;
                functionArray += QLatin1String("\n");
            }

            functionArray += expression;
            reference->compiledIndex = ii;
        }
        functionArray += QLatin1String("]");

        compileState->v8BindingProgram = functionArray;
        compileState->v8BindingProgramLine = startLineNumber;
        compileState->v8BindingProgramIndex = output->v8bindings.count();
        output->v8bindings.append(v8::Persistent<v8::Array>());
    }

    if (bindingCompiler.isValid()) 
        compileState->compiledBindingData = bindingCompiler.program();

    saveComponentState();

    return true;
}

void QDeclarativeCompiler::dumpStats()
{
    Q_ASSERT(componentStats);
    qWarning().nospace() << "QML Document: " << output->url.toString();
    for (int ii = 0; ii < componentStats->savedComponentStats.count(); ++ii) {
        const ComponentStat &stat = componentStats->savedComponentStats.at(ii);
        qWarning().nospace() << "    Component Line " << stat.lineNumber;
        qWarning().nospace() << "        Total Objects:      " << stat.objects;
        qWarning().nospace() << "        IDs Used:           " << stat.ids;
        qWarning().nospace() << "        Optimized Bindings: " << stat.optimizedBindings.count();

        {
        QByteArray output;
        for (int ii = 0; ii < stat.optimizedBindings.count(); ++ii) {
            if (0 == (ii % 10)) {
                if (ii) output.append("\n");
                output.append("            ");
            }

            output.append("(");
            output.append(QByteArray::number(stat.optimizedBindings.at(ii).start.line));
            output.append(":");
            output.append(QByteArray::number(stat.optimizedBindings.at(ii).start.column));
            output.append(") ");
        }
        if (!output.isEmpty())
            qWarning().nospace() << output.constData();
        }

        qWarning().nospace() << "        QScript Bindings:   " << stat.scriptBindings.count();
        {
        QByteArray output;
        for (int ii = 0; ii < stat.scriptBindings.count(); ++ii) {
            if (0 == (ii % 10)) {
                if (ii) output.append("\n");
                output.append("            ");
            }

            output.append("(");
            output.append(QByteArray::number(stat.scriptBindings.at(ii).start.line));
            output.append(":");
            output.append(QByteArray::number(stat.scriptBindings.at(ii).start.column));
            output.append(") ");
        }
        if (!output.isEmpty())
            qWarning().nospace() << output.constData();
        }
    }
}

/*!
    Returns true if from can be assigned to a (QObject) property of type
    to.
*/
bool QDeclarativeCompiler::canCoerce(int to, QDeclarativeParser::Object *from)
{
    const QMetaObject *toMo = 
        enginePrivate->rawMetaObjectForType(to);
    const QMetaObject *fromMo = from->metaObject();

    while (fromMo) {
        if (QDeclarativePropertyPrivate::equal(fromMo, toMo))
            return true;
        fromMo = fromMo->superClass();
    }
    return false;
}

/*!
    Returns the element name, as written in the QML file, for o.
*/
QString QDeclarativeCompiler::elementName(QDeclarativeParser::Object *o)
{
    Q_ASSERT(o);
    if (o->type != -1) {
        return output->types.at(o->type).className;
    } else {
        return QString();
    }
}

QDeclarativeType *QDeclarativeCompiler::toQmlType(QDeclarativeParser::Object *from)
{
    // ### Optimize
    const QMetaObject *mo = from->metatype;
    QDeclarativeType *type = 0;
    while (!type && mo) {
        type = QDeclarativeMetaType::qmlType(mo);
        mo = mo->superClass();
    }
   return type;
}

QStringList QDeclarativeCompiler::deferredProperties(QDeclarativeParser::Object *obj)
{
    const QMetaObject *mo = obj->metatype;

    int idx = mo->indexOfClassInfo("DeferredPropertyNames");
    if (idx == -1)
        return QStringList();

    QMetaClassInfo classInfo = mo->classInfo(idx);
    QStringList rv = QString::fromUtf8(classInfo.value()).split(QLatin1Char(','));
    return rv;
}

QDeclarativePropertyCache::Data *
QDeclarativeCompiler::property(QDeclarativeParser::Object *object, int index)
{
    QDeclarativePropertyCache *cache = 0;

    if (object->synthCache)
        cache = object->synthCache;
    else if (object->type != -1)
        cache = output->types[object->type].createPropertyCache(engine);
    else
        cache = QDeclarativeEnginePrivate::get(engine)->cache(object->metaObject());

    return cache->property(index);
}

QDeclarativePropertyCache::Data *
QDeclarativeCompiler::property(QDeclarativeParser::Object *object, const QHashedStringRef &name, bool *notInRevision)
{
    if (notInRevision) *notInRevision = false;

    QDeclarativePropertyCache *cache = 0;

    if (object->synthCache)
        cache = object->synthCache;
    else if (object->type != -1)
        cache = output->types[object->type].createPropertyCache(engine);
    else
        cache = QDeclarativeEnginePrivate::get(engine)->cache(object->metaObject());

    QDeclarativePropertyCache::Data *d = cache->property(name);

    // Find the first property
    while (d && d->isFunction())
        d = cache->overrideData(d);

    if (d && !cache->isAllowedInRevision(d)) {
        if (notInRevision) *notInRevision = true;
        return 0;
    } else {
        return d;
    }
}

// This code must match the semantics of QDeclarativePropertyPrivate::findSignalByName
QDeclarativePropertyCache::Data *
QDeclarativeCompiler::signal(QDeclarativeParser::Object *object, const QHashedStringRef &name, bool *notInRevision)
{
    if (notInRevision) *notInRevision = false;

    QDeclarativePropertyCache *cache = 0;

    if (object->synthCache)
        cache = object->synthCache;
    else if (object->type != -1)
        cache = output->types[object->type].createPropertyCache(engine);
    else
        cache = QDeclarativeEnginePrivate::get(engine)->cache(object->metaObject());


    QDeclarativePropertyCache::Data *d = cache->property(name);
    if (notInRevision) *notInRevision = false;

    while (d && !(d->isFunction()))
        d = cache->overrideData(d);

    if (d && !cache->isAllowedInRevision(d)) {
        if (notInRevision) *notInRevision = true;
        return 0;
    } else if (d) {
        return d;
    }

    if (name.endsWith(Changed_string)) {
        QHashedStringRef propName = name.mid(0, name.length() - Changed_string.length());

        d = property(object, propName, notInRevision);
        if (d) 
            return cache->method(d->notifyIndex);
    }

    return 0;
}

// This code must match the semantics of QDeclarativePropertyPrivate::findSignalByName
int QDeclarativeCompiler::indexOfSignal(QDeclarativeParser::Object *object, const QString &name, 
                                        bool *notInRevision)
{
    QDeclarativePropertyCache::Data *d = signal(object, QStringRef(&name), notInRevision);
    return d?d->coreIndex:-1;
}

int QDeclarativeCompiler::indexOfProperty(QDeclarativeParser::Object *object, const QString &name, 
                                          bool *notInRevision)
{
    return indexOfProperty(object, QStringRef(&name), notInRevision);
}

int QDeclarativeCompiler::indexOfProperty(QDeclarativeParser::Object *object, const QHashedStringRef &name, 
                                          bool *notInRevision)
{
    QDeclarativePropertyCache::Data *d = property(object, name, notInRevision);
    return d?d->coreIndex:-1;
}

QT_END_NAMESPACE

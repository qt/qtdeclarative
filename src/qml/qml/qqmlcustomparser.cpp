// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qml/qqmlpropertyvalidator_p.h"
#include "qqmlcustomparser_p.h"

#include <private/qv4compileddata_p.h>
#include <private/qqmlsourcecoordinate_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQmlCustomParser
    \brief The QQmlCustomParser class allows you to add new arbitrary types to QML.
    \internal

    By subclassing QQmlCustomParser, you can add a parser for
    building a particular type.

    The subclass must implement compile() and setCustomData(), and register
    itself in the meta type system by calling the macro:

    \code
    QML_REGISTER_CUSTOM_TYPE(Module, MajorVersion, MinorVersion, Name, TypeClass, ParserClass)
    \endcode
*/

/*
    \fn QByteArray QQmlCustomParser::compile(const QList<QQmlCustomParserProperty> & properties)

    The custom parser processes \a properties, and returns
    a QByteArray containing data meaningful only to the
    custom parser; the type engine will pass this same data to
    setCustomData() when making an instance of the data.

    Errors must be reported via the error() functions.

    The QByteArray may be cached between executions of the system, so
    it must contain correctly-serialized data (not, for example,
    pointers to stack objects).
*/

/*
    \fn void QQmlCustomParser::setCustomData(QObject *object, const QByteArray &data)

    This function sets \a object to have the properties defined
    by \a data, which is a block of data previously returned by a call
    to compile().

    Errors should be reported using qmlWarning(object).

    The \a object will be an instance of the TypeClass specified by QML_REGISTER_CUSTOM_TYPE.
*/

void QQmlCustomParser::clearErrors()
{
    exceptions.clear();
}

/*!
    Reports an error with the given \a description.

    An error is generated referring to the \a location in the source file.
*/
void QQmlCustomParser::error(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
    error.setDescription(description);

    exceptions << error;
}

/*!
    If \a script is a simple enumeration expression (eg. Text.AlignLeft),
    returns the integer equivalent (eg. 1), and sets \a ok to true.

    Otherwise sets \a ok to false.

    A valid \a ok must be provided, or the function will assert.
*/
int QQmlCustomParser::evaluateEnum(const QString &script, bool *ok) const
{
    Q_ASSERT_X(ok, "QQmlCustomParser::evaluateEnum", "ok must not be a null pointer");
    *ok = false;

    // we support one or two '.' in the enum phrase:
    // * <TypeName>.<EnumValue>
    // * <TypeName>.<ScopedEnumName>.<EnumValue>

    auto nextDot = [&](int dot) {
        const int nextDot = script.indexOf(u'.', dot + 1);
        return (nextDot == script.size() - 1) ? -1 : nextDot;
    };

    int dot = nextDot(-1);
    if (dot == -1)
        return -1;

    const QString scope = script.left(dot);

    if (scope != QLatin1String("Qt")) {
        if (imports.isNull())
            return -1;

        QQmlTypeLoader *loader = typeLoader();
        if (!loader)
            return -1;

        QQmlType type;
        if (imports.isT1()) {
            QQmlImportNamespace *ns = nullptr;

            // Pass &recursionDetected to resolveType because that implicitly allows recursion.
            // This way we can find the QQmlType of the document we're currently validating.
            bool recursionDetected = false;

            if (!imports.asT1()->resolveType(
                        loader, scope, &type, nullptr, &ns, nullptr,
                        QQmlType::AnyRegistrationType, &recursionDetected)) {
                return -1;
            }

            if (!type.isValid() && ns != nullptr) {
                dot = nextDot(dot);
                if (dot == -1 || !imports.asT1()->resolveType(
                            loader, script.left(dot), &type, nullptr, nullptr, nullptr,
                            QQmlType::AnyRegistrationType, &recursionDetected)) {
                    return -1;
                }
            }
        } else {
            // Allow recursion so that we can find enums from the same document.
            const QQmlTypeNameCache::Result result
                    = imports.asT2()->query<QQmlImport::AllowRecursion>(scope, loader);
            if (result.type.isValid()) {
                type = result.type;
            } else if (result.importNamespace) {
                dot = nextDot(dot);
                if (dot != -1) {
                    type = imports.asT2()->query<QQmlImport::AllowRecursion>(
                                                 script.left(dot), loader).type;
                }
            }
        }

        if (!type.isValid())
            return -1;

        const int dot2 = nextDot(dot);
        const bool dot2Valid = (dot2 != -1);
        const QString enumValue = script.mid(dot2Valid ? dot2 + 1 : dot + 1);
        const QString scopedEnumName = dot2Valid ? script.mid(dot + 1, dot2 - dot - 1) : QString();

        // If we're currently validating the same document, we won't be able to find its enums using
        // the QQmlType. However, we do have the property cache already, and that one contains the
        // enums.
        const QUrl documentUrl = validator ? validator->documentSourceUrl() : QUrl();
        if (documentUrl.isValid() && documentUrl == type.sourceUrl()) {
            Q_ASSERT(validator);
            const QQmlPropertyCache::ConstPtr rootCache = validator->rootPropertyCache();
            const int count = rootCache->qmlEnumCount();
            for (int ii = 0; ii < count; ++ii) {
                const QQmlEnumData *enumData = rootCache->qmlEnum(ii);
                if (!scopedEnumName.isEmpty() && scopedEnumName != enumData->name)
                    continue;

                for (int jj = 0; jj < enumData->values.size(); ++jj) {
                    const QQmlEnumValue value = enumData->values.at(jj);
                    if (value.namedValue == enumValue) {
                        *ok = true;
                        return value.value;
                    }
                }
            }
            return -1;
        }

        if (!scopedEnumName.isEmpty())
            return type.scopedEnumValue(loader, scopedEnumName, enumValue, ok);
        else
            return type.enumValue(loader, enumValue, ok);
    }

    const QString enumValue = script.mid(dot + 1);
    const QMetaObject *mo = &Qt::staticMetaObject;
    int i = mo->enumeratorCount();
    while (i--) {
        int v = mo->enumerator(i).keyToValue(enumValue.toUtf8().constData(), ok);
        if (*ok)
            return v;
    }
    return -1;
}

/*!
    Resolves \a name to a type, or 0 if it is not a type. This can be used
    to type-check object nodes.
*/
const QMetaObject *QQmlCustomParser::resolveType(const QString& name) const
{
    if (!imports.isT1())
        return nullptr;

    QQmlTypeLoader *loader = typeLoader();
    if (!loader)
        return nullptr;

    QQmlType qmltype;
    if (!imports.asT1()->resolveType(loader, name, &qmltype, nullptr, nullptr))
        return nullptr;

    return qmltype.metaObject();
}

QQmlTypeLoader *QQmlCustomParser::typeLoader() const
{
    if (!engine && !validator)
        return nullptr;

    return validator ? validator->typeLoader() : &engine->typeLoader;
}

QT_END_NAMESPACE

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

#include "qjsconverter_p.h"
#include <stdlib.h>

#include <private/qv4jsir_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4dateobject_p.h>

#ifndef QJSCONVERTER_IMPL_P_H
#define QJSCONVERTER_IMPL_P_H

#ifdef Q_OS_QNX
#include <malloc.h>
#endif

QT_BEGIN_NAMESPACE

extern char *qdtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve, char **digits_str);
Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

// Converts a JS RegExp to a QRegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
QRegExp QJSConverter::toRegExp(const QV4::RegExpObject *jsRegExp)
{
    QV4::RegExp *re = jsRegExp->value;
    QString pattern = re->pattern();
    Qt::CaseSensitivity caseSensitivity = re->ignoreCase() ? Qt::CaseInsensitive : Qt::CaseSensitive;
    return QRegExp(pattern, caseSensitivity, QRegExp::RegExp2);
}

// Converts a QRegExp to a JS RegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
QV4::RegExpObject *QJSConverter::toRegExp(const QRegExp &re)
{
    // Convert the pattern to a ECMAScript pattern.
    QString pattern = qt_regexp_toCanonical(re.pattern(), re.patternSyntax());
    if (re.isMinimal()) {
        QString ecmaPattern;
        int len = pattern.length();
        ecmaPattern.reserve(len);
        int i = 0;
        const QChar *wc = pattern.unicode();
        bool inBracket = false;
        while (i < len) {
            QChar c = wc[i++];
            ecmaPattern += c;
            switch (c.unicode()) {
            case '?':
            case '+':
            case '*':
            case '}':
                if (!inBracket)
                    ecmaPattern += QLatin1Char('?');
                break;
            case '\\':
                if (i < len)
                    ecmaPattern += wc[i++];
                break;
            case '[':
                inBracket = true;
                break;
            case ']':
                inBracket = false;
                break;
            default:
                break;
            }
        }
        pattern = ecmaPattern;
    }

    int flags = 0;
    if (re.caseSensitivity() == Qt::CaseInsensitive)
        flags |= QQmlJS::V4IR::RegExp::RegExp_IgnoreCase;

    QV4::ExecutionEngine *e = v8::Isolate::GetCurrent()->GetEngine();
    return e->newRegExpObject(pattern, flags);
}

// Converts a QStringList to JS.
// The result is a new Array object with length equal to the length
// of the QStringList, and the elements being the QStringList's
// elements converted to JS Strings.
QV4::Value QJSConverter::toStringList(const QStringList &list)
{
    QV4::ExecutionEngine *e = v8::Isolate::GetCurrent()->GetEngine();
    QV4::ArrayObject *a = e->newArrayObject();
    int len = list.count();
    a->arrayReserve(len);
    for (int ii = 0; ii < len; ++ii)
        a->arrayData[ii].value = QV4::Value::fromString(e->newString(list.at(ii)));
    a->setArrayLengthUnchecked(len);
    return QV4::Value::fromObject(a);
}

// Converts a JS Array object to a QStringList.
// The result is a QStringList with length equal to the length
// of the JS Array, and elements being the JS Array's elements
// converted to QStrings.
QStringList QJSConverter::toStringList(const QV4::Value &jsArray)
{
    QStringList result;

    QV4::ArrayObject *a = jsArray.asArrayObject();
    if (!a)
        return result;
    QV4::ExecutionEngine *e = a->internalClass->engine;

    uint32_t length = a->arrayLength();
    for (uint32_t i = 0; i < length; ++i)
        result.append(a->getIndexed(e->current, i).toString(e->current)->toQString());
    return result;
}


// Converts a JS Date to a QDateTime.
QDateTime QJSConverter::toDateTime(QV4::DateObject *jsDate)
{
    return QDateTime::fromMSecsSinceEpoch(jsDate->value.doubleValue());
}

// Converts a QDateTime to a JS Date.
QV4::DateObject *QJSConverter::toDateTime(const QDateTime &dt)
{
    double date;
    if (!dt.isValid())
        date = qSNaN();
    else
        date = dt.toMSecsSinceEpoch();
    QV4::ExecutionEngine *e = v8::Isolate::GetCurrent()->GetEngine();
    return e->newDateObject(QV4::Value::fromDouble(date));
}

QT_END_NAMESPACE

#endif // QJSCONVERTER_IMPL_P_H

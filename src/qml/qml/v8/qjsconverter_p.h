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

#ifndef QJSCONVERTER_P_H
#define QJSCONVERTER_P_H

#include "qjsvalue_p.h"
#include <QtCore/qglobal.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qregexp.h>
#include <QtCore/qdatetime.h>

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

/*
  \internal
  \class QJSConverter
  QJSValue and QJSEngine helper class. This class's responsibility is to convert values
  between JS values and Qt/C++ values.

  This is a nice way to inline these functions in both QJSValue and QJSEngine.
*/
class QJSConverter {
public:
    static inline quint32 toArrayIndex(const QString& string);

    static inline QString toString(v8::Handle<v8::String> jsString);
    static inline v8::Local<v8::String> toString(const QString& string);
    static inline QString toString(double value);

    enum {
        PropertyAttributeMask = v8::ReadOnly | v8::DontDelete | v8::DontEnum,
    };
    // return a mask of v8::PropertyAttribute that may also contains QScriptValue::PropertyGetter or QScriptValue::PropertySetter
    static inline uint toPropertyAttributes(const QFlags<QJSValuePrivate::PropertyFlag>& flags);

    // Converts a JS RegExp to a QRegExp.
    // The conversion is not 100% exact since ECMA regexp and QRegExp
    // have different semantics/flags, but we try to do our best.
    static inline QRegExp toRegExp(v8::Handle<v8::RegExp> jsRegExp);

    // Converts a QRegExp to a JS RegExp.
    // The conversion is not 100% exact since ECMA regexp and QRegExp
    // have different semantics/flags, but we try to do our best.
    static inline v8::Local<v8::RegExp> toRegExp(const QRegExp &re);

    // Converts a QStringList to JS.
    // The result is a new Array object with length equal to the length
    // of the QStringList, and the elements being the QStringList's
    // elements converted to JS Strings.
    static inline v8::Local<v8::Array> toStringList(const QStringList &lst);

    // Converts a JS Array object to a QStringList.
    // The result is a QStringList with length equal to the length
    // of the JS Array, and elements being the JS Array's elements
    // converted to QStrings.
    static inline QStringList toStringList(v8::Handle<v8::Array> jsArray);

    // Converts a JS Date to a QDateTime.
    static inline QDateTime toDateTime(v8::Handle<v8::Date> jsDate);

    // Converts a QDateTime to a JS Date.
    static inline v8::Local<v8::Value> toDateTime(const QDateTime &dt);
};

QT_END_NAMESPACE

#endif

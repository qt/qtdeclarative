/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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
#ifndef QQMLTYPECOMPILER_P_H
#define QQMLTYPECOMPILER_P_H

#include <qglobal.h>
#include <qqmlerror.h>
#include <qhash.h>
#include <private/qqmlcompiler_p.h>

QT_BEGIN_NAMESPACE

class QQmlEnginePrivate;
class QQmlCompiledData;
class QQmlError;
class QQmlTypeData;
class QQmlImports;

namespace QtQml {
struct ParsedQML;
}

namespace QV4 {
namespace CompiledData {
struct QmlUnit;
struct Location;
}
}

struct QQmlTypeCompiler
{
    QQmlTypeCompiler(QQmlEnginePrivate *engine, QQmlCompiledData *compiledData, QQmlTypeData *typeData, QtQml::ParsedQML *parsedQML);

    bool compile();

    QList<QQmlError> compilationErrors() const { return errors; }
    void recordError(const QQmlError &error);

    QString stringAt(int idx) const;

    const QV4::CompiledData::QmlUnit *qmlUnit() const;

    QQmlEnginePrivate *enginePrivate() const { return engine; }
    const QQmlImports *imports() const;
    QHash<int, QQmlCompiledData::TypeReference> *resolvedTypes();
    QList<QtQml::QmlObject*> *qmlObjects();
    int rootObjectIndex() const;
    const QList<QQmlPropertyCache *> &propertyCaches() const;
    QList<QByteArray> *vmeMetaObjects() const;
    QHash<int, int> *objectIndexToIdForRoot();
    QHash<int, QHash<int, int> > *objectIndexToIdPerComponent();
    QHash<int, QByteArray> *customParserData();

private:
    QList<QQmlError> errors;
    QQmlEnginePrivate *engine;
    QQmlCompiledData *compiledData;
    QQmlTypeData *typeData;
    QtQml::ParsedQML *parsedQML;
};

struct QQmlCompilePass
{
    QQmlCompilePass(QQmlTypeCompiler *typeCompiler);
    QList<QQmlError> errors;

    QString stringAt(int idx) const { return compiler->stringAt(idx); }
protected:
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    const QUrl url;
    QQmlTypeCompiler *compiler;
};

QT_END_NAMESPACE

#endif // QQMLTYPECOMPILER_P_H

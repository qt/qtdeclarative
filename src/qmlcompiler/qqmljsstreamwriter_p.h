/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQMLJSSTREAMWRITER_P_H
#define QQMLJSSTREAMWRITER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QScopedPointer>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE

class QQmlJSStreamWriter
{
public:
    QQmlJSStreamWriter(QByteArray *array);

    void writeStartDocument();
    void writeEndDocument();
    void writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as = QString());
    //void writeFilesystemImport(const QString &file, const QString &as = QString());
    void writeStartObject(const QString &component);
    void writeEndObject();
    void writeScriptBinding(const QString &name, const QString &rhs);
    void writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue);
    void writeArrayBinding(const QString &name, const QStringList &elements);
    void write(const QString &data);
    void writeBooleanBinding(const QString &name, bool value);

private:
    void writeIndent();
    void writePotentialLine(const QByteArray &line);
    void flushPotentialLinesWithNewlines();

    int m_indentDepth;
    QList<QByteArray> m_pendingLines;
    int m_pendingLineLength;
    bool m_maybeOneline;
    QScopedPointer<QIODevice> m_stream;
};

QT_END_NAMESPACE

#endif // QQMLJSSTREAMWRITER_P_H

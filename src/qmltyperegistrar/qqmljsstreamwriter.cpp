// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsstreamwriter_p.h"

#include <QtCore/QBuffer>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

QQmlJSStreamWriter::QQmlJSStreamWriter(QByteArray *array)
    : m_indentDepth(0)
    , m_pendingLineLength(0)
    , m_maybeOneline(false)
    , m_stream(new QBuffer(array))
{
    m_stream->open(QIODevice::WriteOnly);
}

void QQmlJSStreamWriter::writeStartDocument()
{
}

void QQmlJSStreamWriter::writeEndDocument()
{
}

void QQmlJSStreamWriter::writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as)
{
    m_stream->write(QString::fromLatin1("import %1 %2.%3").arg(uri, QString::number(majorVersion), QString::number(minorVersion)).toUtf8());
    if (!as.isEmpty())
        m_stream->write(QString::fromLatin1(" as %1").arg(as).toUtf8());
    m_stream->write("\n");
}

void QQmlJSStreamWriter::writeStartObject(const QString &component)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString::fromLatin1("%1 {").arg(component).toUtf8());
    ++m_indentDepth;
    m_maybeOneline = true;
}

void QQmlJSStreamWriter::writeEndObject()
{
    if (m_maybeOneline) {
        --m_indentDepth;
        for (int i = 0; i < m_pendingLines.size(); ++i) {
            m_stream->write(" ");
            m_stream->write(m_pendingLines.at(i).trimmed());
            if (i != m_pendingLines.size() - 1)
                m_stream->write(";");
        }

        if (!m_pendingLines.isEmpty())
            m_stream->write(" }\n");
        else
            m_stream->write("}\n");

        m_pendingLines.clear();
        m_pendingLineLength = 0;
        m_maybeOneline = false;
    } else {
        flushPotentialLinesWithNewlines();
        --m_indentDepth;
        writeIndent();
        m_stream->write("}\n");
    }
}

void QQmlJSStreamWriter::writeScriptBinding(const QString &name, const QString &rhs)
{
    writePotentialLine(QString::fromLatin1("%1: %2").arg(name, rhs).toUtf8());
}

void QQmlJSStreamWriter::writeBooleanBinding(const QString &name, bool value)
{
    writeScriptBinding(name, value ? QLatin1String("true") : QLatin1String("false"));
}

void QQmlJSStreamWriter::writeArrayBinding(const QString &name, const QStringList &elements)
{
    flushPotentialLinesWithNewlines();
    writeIndent();

    // try to use a single line
    QString singleLine;
    singleLine += QString::fromLatin1("%1: [").arg(name);
    for (int i = 0; i < elements.size(); ++i) {
        singleLine += elements.at(i);
        if (i != elements.size() - 1)
            singleLine += QLatin1String(", ");
    }
    singleLine += QLatin1String("]\n");
    if (singleLine.size() + m_indentDepth * 4 < 80) {
        m_stream->write(singleLine.toUtf8());
        return;
    }

    // write multi-line
    m_stream->write(QString::fromLatin1("%1: [\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < elements.size(); ++i) {
        writeIndent();
        m_stream->write(elements.at(i).toUtf8());
        if (i != elements.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("]\n");
}

void QQmlJSStreamWriter::write(const QString &data)
{
    flushPotentialLinesWithNewlines();
    m_stream->write(data.toUtf8());
}

void QQmlJSStreamWriter::writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString::fromLatin1("%1: {\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < keyValue.size(); ++i) {
        const QString key = keyValue.at(i).first;
        const QString value = keyValue.at(i).second;
        writeIndent();
        m_stream->write(QString::fromLatin1("%1: %2").arg(key, value).toUtf8());
        if (i != keyValue.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("}\n");
}

void QQmlJSStreamWriter::writeIndent()
{
    m_stream->write(QByteArray(m_indentDepth * 4, ' '));
}

void QQmlJSStreamWriter::writePotentialLine(const QByteArray &line)
{
    m_pendingLines.append(line);
    m_pendingLineLength += line.size();
    if (m_pendingLineLength >= 80) {
        flushPotentialLinesWithNewlines();
    }
}

void QQmlJSStreamWriter::flushPotentialLinesWithNewlines()
{
    if (m_maybeOneline)
        m_stream->write("\n");
    for (const QByteArray &line : std::as_const(m_pendingLines)) {
        writeIndent();
        m_stream->write(line);
        m_stream->write("\n");
    }
    m_pendingLines.clear();
    m_pendingLineLength = 0;
    m_maybeOneline = false;
}

QT_END_NAMESPACE

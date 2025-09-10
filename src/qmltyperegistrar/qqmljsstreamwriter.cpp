// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsstreamwriter_p.h"
#include "qanystringviewutils_p.h"

#include <QtCore/QBuffer>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

// TODO: All of this could be improved by using and re-using one buffer for all the writing.
//       We don't really need to allocate any temporary byte arrays.

static QByteArray enquoteByteArray(QByteArrayView string)
{
    const qsizetype length = string.length();
    QByteArray buffer;
    buffer.reserve(length + 2);
    buffer.append(' ');
    buffer.append(string);
    buffer.append(' ');
    buffer.replace('\\', "\\\\").replace('"', "\\\"");
    buffer[0] = '"';
    buffer[buffer.length() - 1] = '"';
    return buffer;
}

static QByteArray enquoteAnyString(QAnyStringView string)
{
    return string.visit([](auto data) {
        return QAnyStringViewUtils::processAsUtf8(data, [](QByteArrayView view) {
            return enquoteByteArray(view);
        });
    });
}

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

void QQmlJSStreamWriter::writeLibraryImport(
        QByteArrayView uri, int majorVersion, int minorVersion, QByteArrayView as)
{
    m_stream->write("import ");
    m_stream->write(uri.data(), uri.length());
    m_stream->write(" ");
    m_stream->write(QByteArray::number(majorVersion));
    m_stream->write(".");
    m_stream->write(QByteArray::number(minorVersion));
    if (!as.isEmpty()) {
        m_stream->write(" as ");
        m_stream->write(as.data(), as.length());
    }
    m_stream->write("\n");
}

void QQmlJSStreamWriter::writeStartObject(QByteArrayView component)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(component.data(), component.length());
    m_stream->write(" {");
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

void QQmlJSStreamWriter::writeScriptBinding(QByteArrayView name, QByteArrayView rhs)
{
    QByteArray buffer;
    buffer.reserve(name.length() + 2 + rhs.length());
    buffer.append(name);
    buffer.append(": ");
    buffer.append(rhs);
    writePotentialLine(buffer);
}

void QQmlJSStreamWriter::writeStringBinding(QByteArrayView name, QAnyStringView value)
{
    writeScriptBinding(name, enquoteAnyString(value));
}

void QQmlJSStreamWriter::writeNumberBinding(QByteArrayView name, qint64 value)
{
    writeScriptBinding(name, QByteArray::number(value));
}

void QQmlJSStreamWriter::writeBooleanBinding(QByteArrayView name, bool value)
{
    writeScriptBinding(name, value ? "true" : "false");
}

template<typename String, typename ElementHandler>
void QQmlJSStreamWriter::doWriteArrayBinding(
        QByteArrayView name, const QList<String> &elements, ElementHandler &&handler)
{
    flushPotentialLinesWithNewlines();
    writeIndent();

    // try to use a single line
    QByteArray singleLine(name.data(), name.length());
    singleLine += ": [";
    for (int i = 0; i < elements.size(); ++i) {
        QAnyStringViewUtils::processAsUtf8(elements.at(i), [&](QByteArrayView element) {
            singleLine += handler(element);
        });
        if (i != elements.size() - 1)
            singleLine += ", ";
    }
    singleLine += "]\n";
    if (singleLine.size() + m_indentDepth * 4 < 80) {
        m_stream->write(singleLine);
        return;
    }

    // write multi-line
    m_stream->write(name.data(), name.length());
    m_stream->write(": [\n");
    ++m_indentDepth;
    for (int i = 0; i < elements.size(); ++i) {
        writeIndent();
        QAnyStringViewUtils::processAsUtf8(elements.at(i), [&](QByteArrayView element) {
            const auto handled = handler(element);
            m_stream->write(handled.data(), handled.length());
        });
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

void QQmlJSStreamWriter::writeArrayBinding(QByteArrayView name, const QByteArrayList &elements)
{
    doWriteArrayBinding(name, elements, [](QByteArrayView view) { return view; });
}

void QQmlJSStreamWriter::writeStringListBinding(
        QByteArrayView name, const QList<QAnyStringView> &elements)
{
    doWriteArrayBinding(name, elements, enquoteByteArray);
}

void QQmlJSStreamWriter::write(QByteArrayView data)
{
    flushPotentialLinesWithNewlines();
    m_stream->write(data.data(), data.length());
}

void QQmlJSStreamWriter::writeEnumObjectLiteralBinding(
    QByteArrayView name, const QList<QPair<QAnyStringView, int> > &keyValue)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(name.data(), name.length());
    m_stream->write(": {\n");
    ++m_indentDepth;
    for (int i = 0, end = keyValue.size(); i != end; ++i) {
        writeIndent();
        const auto &entry = keyValue[i];
        m_stream->write(enquoteAnyString(entry.first));
        m_stream->write(": ");
        m_stream->write(QByteArray::number(entry.second));
        if (i != end - 1)
            m_stream->write(",\n");
        else
            m_stream->write("\n");
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("}\n");
}

void QQmlJSStreamWriter::writeIndent()
{
    for (int i = 0; i < m_indentDepth; ++i)
        m_stream->write("    ");
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

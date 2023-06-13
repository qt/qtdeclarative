// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qresourcerelocater_p.h"

#include <QXmlStreamReader>
#include <QFile>
#include <QDir>

QT_BEGIN_NAMESPACE

/*!
  \internal
  Changes all the paths in resource file \a input so that they are relative to
  location \a output and writes the result to resource file \a output.
 */
int qRelocateResourceFile(const QString &input, const QString &output)
{
    enum State {
        InitialState,
        InRCC,
        InResource,
        InFile
    };
    State state = InitialState;

    QString prefix;
    QString currentFileName;
    QXmlStreamAttributes fileAttributes;

    QFile file(input);
    if (!file.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Cannot open %s for reading.\n", qPrintable(input));
        return EXIT_FAILURE;
    }

    QDir inputDirectory = QFileInfo(file).absoluteDir();
    QDir outputDirectory = QFileInfo(output).absoluteDir();

    QString outputString;
    QXmlStreamWriter writer(&outputString);
    writer.setAutoFormatting(true);

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartDocument: {
            QStringView version = reader.documentVersion();
            if (!version.isEmpty())
                writer.writeStartDocument(version.toString());
            else
                writer.writeStartDocument();
            break;
        }
        case QXmlStreamReader::EndDocument:
            writer.writeEndDocument();
            break;
        case QXmlStreamReader::StartElement:
            if (reader.name() == QStringLiteral("RCC")) {
                if (state != InitialState) {
                    fprintf(stderr, "Unexpected RCC tag in line %d\n", int(reader.lineNumber()));
                    return EXIT_FAILURE;
                }
                state = InRCC;
            } else if (reader.name() == QStringLiteral("qresource")) {
                if (state != InRCC) {
                    fprintf(stderr, "Unexpected qresource tag in line %d\n", int(reader.lineNumber()));
                    return EXIT_FAILURE;
                }
                state = InResource;
                QXmlStreamAttributes attributes = reader.attributes();
                if (attributes.hasAttribute(QStringLiteral("prefix")))
                    prefix = attributes.value(QStringLiteral("prefix")).toString();
                if (!prefix.startsWith(QLatin1Char('/')))
                    prefix.prepend(QLatin1Char('/'));
                if (!prefix.endsWith(QLatin1Char('/')))
                    prefix.append(QLatin1Char('/'));
            } else if (reader.name() == QStringLiteral("file")) {
                if (state != InResource) {
                    fprintf(stderr, "Unexpected file tag in line %d\n", int(reader.lineNumber()));
                    return EXIT_FAILURE;
                }
                state = InFile;
                fileAttributes = reader.attributes();
                continue;
            }
            writer.writeStartElement(reader.name().toString());
            writer.writeAttributes(reader.attributes());
            continue;

        case QXmlStreamReader::EndElement:
            if (reader.name() == QStringLiteral("file")) {
                if (state != InFile) {
                    fprintf(stderr, "Unexpected end of file tag in line %d\n", int(reader.lineNumber()));
                    return EXIT_FAILURE;
                }
                state = InResource;
                continue;
            } else if (reader.name() == QStringLiteral("qresource")) {
                if (state != InResource) {
                    fprintf(stderr, "Unexpected end of qresource tag in line %d\n", int(reader.lineNumber()));
                    return EXIT_FAILURE;
                }
                state = InRCC;
            } else if (reader.name() == QStringLiteral("RCC")) {
                if (state != InRCC) {
                    fprintf(stderr, "Unexpected end of RCC tag in line %d\n", int(reader.lineNumber()));
                    return EXIT_FAILURE;
                }
                state = InitialState;
            }
            writer.writeEndElement();
            continue;

        case QXmlStreamReader::Characters:
            if (reader.isWhitespace())
                break;
            if (state != InFile)
                return EXIT_FAILURE;
            currentFileName = reader.text().toString();
            if (currentFileName.isEmpty())
                continue;

            writer.writeStartElement(QStringLiteral("file"));

            if (!fileAttributes.hasAttribute(QStringLiteral("alias")))
                fileAttributes.append(QStringLiteral("alias"), currentFileName);

            currentFileName = inputDirectory.absoluteFilePath(currentFileName);
            currentFileName = outputDirectory.relativeFilePath(currentFileName);

            writer.writeAttributes(fileAttributes);
            writer.writeCharacters(currentFileName);
            writer.writeEndElement();
            continue;

        default: break;
    }
    }

    QFile outputFile(output);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fprintf(stderr, "Cannot open %s for writing.\n", qPrintable(output));
        return EXIT_FAILURE;
    }
    const QByteArray outputStringUtf8 = outputString.toUtf8();
    if (outputFile.write(outputStringUtf8) != outputStringUtf8.size())
        return EXIT_FAILURE;

    outputFile.close();
    if (outputFile.error() != QFileDevice::NoError)
        return EXIT_FAILURE;


    return EXIT_SUCCESS;
}

QT_END_NAMESPACE

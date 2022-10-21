/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#ifndef QMLDOMOUTWRITER_P_H
#define QMLDOMOUTWRITER_P_H

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

#include "qqmldom_global.h"
#include "qqmldom_fwd_p.h"
#include "qqmldomattachedinfo_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

#define QMLDOM_USTRING(s) u##s
#define QMLDOM_REGION(name) constexpr const auto name = QMLDOM_USTRING(#name)
// namespace, so it cam be reopened to add more entries
namespace Regions {
} // namespace Regions

class QMLDOM_EXPORT OutWriterState
{
public:
    OutWriterState(Path itPath, DomItem &it, FileLocations::Tree fLoc);

    void closeState(OutWriter &);

    Path itemCanonicalPath;
    DomItem item;
    PendingSourceLocationId fullRegionId;
    FileLocations::Tree currentMap;
    QMap<QString, PendingSourceLocationId> pendingRegions;
    QMap<QString, CommentedElement> pendingComments;
};

class QMLDOM_EXPORT OutWriter
{
public:
    int indent = 0;
    int indenterId = -1;
    bool indentNextlines = false;
    bool skipComments = false;
    LineWriter &lineWriter;
    Path currentPath;
    FileLocations::Tree topLocation;
    QString writtenStr;
    UpdatedScriptExpression::Tree reformattedScriptExpressions;
    QList<OutWriterState> states;

    explicit OutWriter(LineWriter &lw)
        : lineWriter(lw),
          topLocation(FileLocations::createTree(Path())),
          reformattedScriptExpressions(UpdatedScriptExpression::createTree(Path()))
    {
        lineWriter.addInnerSink([this](QStringView s) { writtenStr.append(s); });
        indenterId =
                lineWriter.addTextAddCallback([this](LineWriter &, LineWriter::TextAddType tt) {
                    if (indentNextlines && tt == LineWriter::TextAddType::Normal
                        && QStringView(lineWriter.currentLine()).trimmed().isEmpty())
                        lineWriter.setLineIndent(indent);
                    return true;
                });
    }

    OutWriterState &state(int i = 0);

    int increaseIndent(int level = 1)
    {
        int oldIndent = indent;
        indent += lineWriter.options().formatOptions.indentSize * level;
        return oldIndent;
    }
    int decreaseIndent(int level = 1, int expectedIndent = -1)
    {
        indent -= lineWriter.options().formatOptions.indentSize * level;
        Q_ASSERT(expectedIndent < 0 || expectedIndent == indent);
        return indent;
    }

    void itemStart(DomItem &it);
    void itemEnd(DomItem &it);
    void regionStart(QString rName);
    void regionStart(QStringView rName) { regionStart(rName.toString()); }
    void regionEnd(QString rName);
    void regionEnd(QStringView rName) { regionEnd(rName.toString()); }

    quint32 counter() const { return lineWriter.counter(); }
    OutWriter &writeRegion(QString rName, QStringView toWrite);
    OutWriter &writeRegion(QStringView rName, QStringView toWrite)
    {
        return writeRegion(rName.toString(), toWrite);
    }
    OutWriter &writeRegion(QString t) { return writeRegion(t, t); }
    OutWriter &writeRegion(QStringView t) { return writeRegion(t.toString(), t); }
    OutWriter &ensureNewline(int nNewlines = 1)
    {
        lineWriter.ensureNewline(nNewlines);
        return *this;
    }
    OutWriter &ensureSpace()
    {
        lineWriter.ensureSpace();
        return *this;
    }
    OutWriter &ensureSpace(QStringView space)
    {
        lineWriter.ensureSpace(space);
        return *this;
    }
    OutWriter &newline()
    {
        lineWriter.newline();
        return *this;
    }
    OutWriter &space()
    {
        lineWriter.space();
        return *this;
    }
    OutWriter &write(QStringView v, LineWriter::TextAddType t = LineWriter::TextAddType::Normal)
    {
        lineWriter.write(v, t);
        return *this;
    }
    OutWriter &write(QStringView v, SourceLocation *toUpdate)
    {
        lineWriter.write(v, toUpdate);
        return *this;
    }
    void flush() { lineWriter.flush(); }
    void eof(bool ensureNewline = true) { lineWriter.eof(ensureNewline); }
    int addNewlinesAutospacerCallback(int nLines)
    {
        return lineWriter.addNewlinesAutospacerCallback(nLines);
    }
    int addTextAddCallback(std::function<bool(LineWriter &, LineWriter::TextAddType)> callback)
    {
        return lineWriter.addTextAddCallback(callback);
    }
    bool removeTextAddCallback(int i) { return lineWriter.removeTextAddCallback(i); }
    void addReformattedScriptExpression(Path p, std::shared_ptr<ScriptExpression> exp)
    {
        if (auto updExp = UpdatedScriptExpression::ensure(reformattedScriptExpressions, p,
                                                          AttachedInfo::PathType::Canonical)) {
            updExp->info().expr = exp;
        }
    }
    DomItem updatedFile(DomItem &qmlFile);
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMOUTWRITER_P_H

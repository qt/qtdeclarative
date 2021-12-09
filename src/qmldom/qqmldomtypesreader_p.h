/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLDOMTYPESREADER_H
#define QQMLDOMTYPESREADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmldomexternalitems_p.h"

#include <QtQml/private/qqmljsastfwd_p.h>

// for Q_DECLARE_TR_FUNCTIONS
#include <QtCore/qcoreapplication.h>
#ifdef QMLDOM_STANDALONE
#    include "qmlcompiler/qqmljsmetatypes_p.h"
#    include "qmlcompiler/qqmljsscope_p.h"
#else
#    include <private/qqmljsmetatypes_p.h>
#    include <private/qqmljsscope_p.h>
#endif
QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QmltypesReader
{
    Q_DECLARE_TR_FUNCTIONS(TypeDescriptionReader)
public:
    explicit QmltypesReader(DomItem qmltypesFile)
        : m_qmltypesFilePtr(qmltypesFile.ownerAs<QmltypesFile>()), m_qmltypesFile(qmltypesFile)
    {
    }

    bool parse();
    // static void read
private:
    void addError(ErrorMessage message);

    void insertProperty(QQmlJSScope::Ptr jsScope, const QQmlJSMetaProperty &property,
                        QMap<int, QmlObject> &objs);
    void insertSignalOrMethod(const QQmlJSMetaMethod &metaMethod, QMap<int, QmlObject> &objs);
    void insertComponent(const QQmlJSScope::Ptr &jsScope,
                         const QList<QQmlJSScope::Export> &exportsList);
    EnumDecl enumFromMetaEnum(const QQmlJSMetaEnum &metaEnum);

    std::shared_ptr<QmltypesFile> qmltypesFilePtr() { return m_qmltypesFilePtr; }
    DomItem &qmltypesFile() { return m_qmltypesFile; }
    ErrorHandler handler()
    {
        return [this](ErrorMessage m) { this->addError(m); };
    }

private:
    bool m_isValid;
    std::shared_ptr<QmltypesFile> m_qmltypesFilePtr;
    DomItem m_qmltypesFile;
    Path m_currentPath;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMTYPESREADER_H

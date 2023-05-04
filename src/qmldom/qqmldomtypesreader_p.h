// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#include <private/qqmljsmetatypes_p.h>
#include <private/qqmljsscope_p.h>

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

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
#ifndef QQMLDOMEXTERNALITEMS_P_H
#define QQMLDOMEXTERNALITEMS_P_H

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

#include "qqmldomitem_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmldirparser_p.h>
#include <QtCore/QMetaType>

#include <limits>

Q_DECLARE_METATYPE(QQmlDirParser::Plugin)

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

/*!
\internal
\class QQmlJS::Dom::ExternalOwningItem

\brief A OwningItem that refers to an external resource (file,...)

Every owning item has a file or directory it refers to.


*/
class QMLDOM_EXPORT ExternalOwningItem: public OwningItem {
public:
    ExternalOwningItem(QString filePath, QDateTime lastDataUpdateAt, Path pathFromTop, int derivedFrom=0);
    ExternalOwningItem(const ExternalOwningItem &o);
    QString canonicalFilePath(const DomItem &) const override;
    QString canonicalFilePath() const;
    Path canonicalPath(const DomItem &) const override;
    Path canonicalPath() const;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool (Path, DomItem &)> visitor) override {
        bool cont = OwningItem::iterateDirectSubpaths(self, visitor);
        cont = cont && self.subDataField(Fields::canonicalFilePath, canonicalFilePath()).visit(visitor);
        cont = cont && self.subDataField(Fields::isValid, isValid()).visit(visitor);
//        if (!code().isNull())
//            cont = cont && self.subDataField(Fields::code, code()).visit(visitor);
        return cont;
    }

    bool isValid() const {
        QMutexLocker l(mutex());
        return m_isValid;
    }
    void setIsValid(bool val) {
        QMutexLocker l(mutex());
        m_isValid = val;
    }
    // null code means invalid
    virtual QString code() const { return QString(); }
protected:
    QString m_canonicalFilePath;
    Path m_path;
    bool m_isValid = false;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMEXTERNALITEMS_P_H

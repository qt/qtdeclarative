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
#ifndef QQMLDOMMODULEINDEX_P_H
#define QQMLDOMMODULEINDEX_P_H

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

#include "qqmldomelements_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT ModuleScope final : public DomBase
{
public:
    constexpr static DomType kindValue = DomType::ModuleScope;
    DomType kind() const override { return kindValue; }

    ModuleScope(QString uri = QString(), const Version &version = Version())
        : uri(uri), version(version)
    {
    }

    Path pathFromOwner() const
    {
        return Path::Field(Fields::moduleScope)
                .key(version.isValid() ? QString::number(version.minorVersion) : QString());
    }
    Path pathFromOwner(DomItem &) const override { return pathFromOwner(); }
    Path canonicalPath(DomItem &self) const override
    {
        return self.owner().canonicalPath().path(pathFromOwner());
    }
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    QString uri;
    Version version;
};

class QMLDOM_EXPORT ModuleIndex final : public OwningItem
{
    Q_DECLARE_TR_FUNCTIONS(ModuleIndex);

protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &self) const override;

public:
    enum class Status { NotLoaded, Loading, Loaded };
    constexpr static DomType kindValue = DomType::ModuleIndex;
    DomType kind() const override { return kindValue; }

    ModuleIndex(QString uri, int majorVersion, int derivedFrom = 0,
                QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0))
        : OwningItem(derivedFrom, lastDataUpdateAt), m_uri(uri), m_majorVersion(majorVersion)
    {
    }

    ModuleIndex(const ModuleIndex &o);

    ~ModuleIndex();

    std::shared_ptr<ModuleIndex> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<ModuleIndex>(doCopy(self));
    }

    Path canonicalPath(DomItem &) const override
    {
        return Paths::moduleIndexPath(uri(), majorVersion());
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    QSet<QString> exportNames(DomItem &self) const;

    QList<DomItem> exportsWithNameAndMinorVersion(DomItem &self, QString name,
                                                  int minorVersion) const;

    QString uri() const { return m_uri; }
    int majorVersion() const { return m_majorVersion; }
    QList<Path> sources() const;

    QList<int> minorVersions() const
    {
        QMutexLocker l(mutex());
        return m_moduleScope.keys();
    }
    ModuleScope *ensureMinorVersion(int minorVersion);
    void mergeWith(std::shared_ptr<ModuleIndex> o);
    void addQmltypeFilePath(Path p)
    {
        QMutexLocker l(mutex());
        if (!m_qmltypesFilesPaths.contains(p))
            m_qmltypesFilesPaths.append(p);
    }

    QList<Path> qmldirsToLoad(DomItem &self);
    QList<Path> qmltypesFilesPaths() const
    {
        QMutexLocker l(mutex());
        return m_qmltypesFilesPaths;
    }
    QList<Path> qmldirPaths() const
    {
        QMutexLocker l(mutex());
        return m_qmldirPaths;
    }
    QList<Path> directoryPaths() const
    {
        QMutexLocker l(mutex());
        return m_directoryPaths;
    }
    QList<DomItem> autoExports(DomItem &self) const;

private:
    QString m_uri;
    int m_majorVersion;

    QList<Path> m_qmltypesFilesPaths;
    QList<Path> m_qmldirPaths;
    QList<Path> m_directoryPaths;
    QMap<int, ModuleScope *> m_moduleScope;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMMODULEINDEX_P_H

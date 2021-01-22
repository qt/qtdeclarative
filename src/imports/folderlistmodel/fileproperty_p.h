/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef FILEPROPERTY_P_H
#define FILEPROPERTY_P_H

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

#include <QFileInfo>
#include <QDateTime>

class FileProperty
{
public:
    FileProperty(const QFileInfo &info) :
        mFileName(info.fileName()),
        mFilePath(info.filePath()),
        mBaseName(info.baseName()),
        mSuffix(info.completeSuffix()),
        mSize(info.size()),
        mIsDir(info.isDir()),
        mIsFile(info.isFile()),
        mLastModified(info.lastModified()),
        mLastRead(info.lastRead())
    {
    }
    ~FileProperty()
    {}

    inline QString fileName() const { return mFileName; }
    inline QString filePath() const { return mFilePath; }
    inline QString baseName() const { return mBaseName; }
    inline qint64 size() const { return mSize; }
    inline QString suffix() const { return mSuffix; }
    inline bool isDir() const { return mIsDir; }
    inline bool isFile() const { return mIsFile; }
    inline QDateTime lastModified() const { return mLastModified; }
    inline QDateTime lastRead() const { return mLastRead; }

    inline bool operator !=(const FileProperty &fileInfo) const {
        return !operator==(fileInfo);
    }
    bool operator ==(const FileProperty &property) const {
        return ((mFileName == property.mFileName) && (isDir() == property.isDir()));
    }

private:
    QString mFileName;
    QString mFilePath;
    QString mBaseName;
    QString mSuffix;
    qint64 mSize;
    bool mIsDir;
    bool mIsFile;
    QDateTime mLastModified;
    QDateTime mLastRead;
};
#endif // FILEPROPERTY_P_H

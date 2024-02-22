// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "directoryvalidator.h"

#include <QFileInfo>

DirectoryValidator::DirectoryValidator(QObject *parent) :
    QObject(parent)
{
}

QString DirectoryValidator::path() const
{
    return mPath;
}

void DirectoryValidator::setPath(const QString &path)
{
    if (path == mPath)
        return;

    const bool wasValid = isValid();
    const QString oldErrorMessage = mErrorMessage;

    mPath = path;
    mErrorMessage.clear();

    QFileInfo fileInfo(mPath);
    if (!fileInfo.exists()) {
        mErrorMessage = QLatin1String("Directory does not exist");
    } else {
        if (!fileInfo.isDir()) {
            mErrorMessage = QLatin1String("Not a directory");
        }
    }

    if (isValid() != wasValid)
        emit validChanged();

    if (mErrorMessage != oldErrorMessage)
        emit errorMessageChanged();

    emit pathChanged();
}

bool DirectoryValidator::isValid() const
{
    return mErrorMessage.isEmpty();
}

QString DirectoryValidator::errorMessage() const
{
    return mErrorMessage;
}

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIRECTORYVALIDATOR_H
#define DIRECTORYVALIDATOR_H

#include <QObject>

class DirectoryValidator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)

public:
    explicit DirectoryValidator(QObject *parent = nullptr);

    QString path() const;
    void setPath(const QString &path);

    bool isValid() const;
    QString errorMessage() const;

signals:
    void pathChanged();
    void validChanged();
    void errorMessageChanged();

private:
    void updateValid();

    QString mPath;
    QString mErrorMessage;
};

#endif // DIRECTORYVALIDATOR_H

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "birthdayparty.h"

Person *BirthdayParty::host() const
{
    return m_host;
}

void BirthdayParty::setHost(Person *c)
{
    m_host = c;
}

QQmlListProperty<Person> BirthdayParty::guests()
{
    return {this, &m_guests};
}

qsizetype BirthdayParty::guestCount() const
{
    return m_guests.count();
}

Person *BirthdayParty::guest(qsizetype index) const
{
    return m_guests.at(index);
}

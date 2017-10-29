/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "duplicatedfilemodel.hpp"
#include "FileTransfer/syfftprotocolreceiver.hpp"
#include "UserDiscovery/user.hpp"
#include "UserDiscovery/users.hpp"

DuplicatedFileModel::DuplicatedFileModel(
    QSharedPointer<SyfftProtocolDuplicatedFile> request, PeersList *peersList,
    QObject *parent)
        : QObject(parent), m_request(request), m_peersList(peersList)
{
    // Connect the handler to update the sender information if changed
    connect(m_peersList, &PeersList::peerUpdated, this,
            [this](const QString &uuid) {
                if (uuid == m_request->senderUuid()) {
                    updateSenderInformation();
                }
            });

    // Retrigger the signal if the connection is aborted
    connect(m_request.data(), &SyfftProtocolDuplicatedFile::connectionAborted,
            this, &DuplicatedFileModel::connectionAborted);

    // Cache the sender information
    updateSenderInformation();
}

QString DuplicatedFileModel::filepath() const
{
    return m_request->currentFile().absolutePath();
}

QString DuplicatedFileModel::filename() const
{
    return m_request->receivedFile().name();
}

QString DuplicatedFileModel::oldFileSize() const
{
    return sizeToHRFormat(
        static_cast<quint64>(m_request->currentFile().size()));
}

QString DuplicatedFileModel::newFileSize() const
{
    return sizeToHRFormat(m_request->receivedFile().size());
}

QString DuplicatedFileModel::oldFileDate() const
{
    return m_request->currentFile().lastModified().toString(
        Qt::SystemLocaleLongDate);
}

QString DuplicatedFileModel::newFileDate() const
{
    return m_request->receivedFile().lastModified().toString(
        Qt::SystemLocaleLongDate);
}

void DuplicatedFileModel::keepExisting(bool all) { m_request->keep(all); }

void DuplicatedFileModel::replaceExisting(bool all) { m_request->replace(all); }

void DuplicatedFileModel::keepBoth(bool all) { m_request->keepBoth(all); }

void DuplicatedFileModel::updateSenderInformation()
{
    const UserInfo &info = m_peersList->peer(m_request->senderUuid());
    if (!info.valid()) {
        return;
    }

    m_names = info.names();
    emit senderNamesUpdated();
}

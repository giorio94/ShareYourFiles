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

#include "transferrequestmodel.hpp"
#include "FileTransfer/syfftprotocolreceiver.hpp"
#include "UserDiscovery/user.hpp"
#include "UserDiscovery/users.hpp"

#include <QUrl>

TransferRequestModel::TransferRequestModel(
    QSharedPointer<SyfftProtocolSharingRequest> request, LocalUser *localUser,
    PeersList *peersList, QObject *parent)
        : QObject(parent),
          m_request(request),
          m_peersList(peersList),
          m_requestUser(false),
          m_iconSet(false)
{
    // Get the reception preferences
    const UserInfo &info = m_peersList->activePeer(m_request->senderUuid());
    if (!info.valid()) {
        return;
    }
    ReceptionPreferences preferences = info.preferences();
    if (preferences.useDefaults()) {
        preferences = localUser->info().preferences();
    }

    // If the default is accept, do it
    if (preferences.action() == ReceptionPreferences::Action::Accept) {
        m_request->accept(preferences.fullPath(info.names()), QString());
        return;
    }

    // If the default is reject, do it
    if (preferences.action() == ReceptionPreferences::Action::Reject) {
        m_request->reject(QString());
        return;
    }

    // Otherwise prepare the request to the user
    m_requestUser = true;
    m_dataPath = preferences.path();
    m_folderUser = preferences.folderUser();
    m_folderDate = preferences.folderDate();

    // Connect the handler to update the sender information if changed
    connect(m_peersList, &PeersList::peerUpdated, this,
            [this](const QString &uuid) {
                if (uuid == m_request->senderUuid()) {
                    updateSenderInformation();
                }
            });

    // Retrigger the signal if the connection is aborted
    connect(request.data(), &SyfftProtocolSharingRequest::connectionAborted,
            this, &TransferRequestModel::connectionAborted);

    // Cache the sender information
    updateSenderInformation();
}

bool TransferRequestModel::anonymous() const
{
    return m_request->senderUuid() == User::ANONYMOUS_UUID;
}

quint32 TransferRequestModel::filesNumber() const
{
    return m_request->totalFiles();
}

QString TransferRequestModel::filesSize() const
{
    return sizeToHRFormat(m_request->totalSize());
}

QString TransferRequestModel::message() const { return m_request->message(); }

void TransferRequestModel::accept(const QString &dataPath, bool folderUser,
                                  bool folderDate, const QString &message,
                                  bool always)
{
    ReceptionPreferences preferences(ReceptionPreferences::Action::Accept,
                                     QDir().absoluteFilePath(dataPath),
                                     folderUser, folderDate);

    // Save the preferences if necessary
    if (always) {
        m_peersList->setReceptionPreferences(m_request->senderUuid(),
                                             preferences);
    }

    // Accept the request
    m_request->accept(preferences.fullPath(m_names), message);
}

void TransferRequestModel::reject(const QString &message, bool always)
{
    // Save the preferences if necessary
    if (always) {
        m_peersList->setReceptionPreferences(
            m_request->senderUuid(),
            ReceptionPreferences(ReceptionPreferences::Action::Reject, "",
                                 false, false));
    }

    // Reject the request
    m_request->reject(message);
}

QString TransferRequestModel::urlToPath(const QString &url)
{
    return QUrl(url).toLocalFile();
}

void TransferRequestModel::updateSenderInformation()
{
    const UserInfo &info = m_peersList->activePeer(m_request->senderUuid());
    if (!info.valid()) {
        return;
    }

    m_names = info.names();
    m_iconSet = info.icon().set();
    m_iconPath = info.icon().path();
    emit senderInformationUpdated();
}

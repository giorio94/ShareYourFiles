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

#include "transferresponsemodel.hpp"
#include "UserDiscovery/users.hpp"

TransferResponseModel::TransferResponseModel(const QString &uuid,
                                             PeersList *peersList,
                                             bool accepted,
                                             const QString &message,
                                             QObject *parent)
        : QObject(parent),
          m_uuid(uuid),
          m_accepted(accepted),
          m_message(message),
          m_peersList(peersList)
{
    // Connect the handler to update the sender information if changed
    connect(m_peersList, &PeersList::peerUpdated, this,
            [this](const QString &uuid) {
                if (uuid == m_uuid) {
                    updateSenderInformation();
                }
            });

    // Cache the sender information
    updateSenderInformation();
}

void TransferResponseModel::updateSenderInformation()
{
    const UserInfo &info = m_peersList->peer(m_uuid);
    if (!info.valid()) {
        return;
    }

    m_names = info.names();
    m_iconSet = info.icon().set();
    m_iconPath = info.icon().path();
    emit senderInformationUpdated();
}

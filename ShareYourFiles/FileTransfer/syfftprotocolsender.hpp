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

#ifndef SYFFTPROTOCOLSENDER_HPP
#define SYFFTPROTOCOLSENDER_HPP

#include "syfftprotocolcommon.hpp"

class FileInTransferReader;
class TransferList;

///
/// \brief The SyfftProtocolSender class provides an implementation of the
/// sending side of the SYFFT protocol.
///
/// \see SyfftProtocolCommon
///
class SyfftProtocolSender : public SyfftProtocolCommon
{
    Q_OBJECT

public:
    ///
    /// \brief The Mode enum describes the current status of a peer.
    ///
    enum class PeerStatus {

        Offline, ///< \brief The peer is currently not active.
        Online   ///< \brief The peer is currently active.
    };
    Q_ENUM(PeerStatus)

    ///
    /// \brief Constructs a new instance of SYFFT Protocol Sender.
    /// \param localUuid the UUID representing the local user.
    /// \param peerUuid the UUID representing the peer user.
    /// \param address IPv4 address on which the peer's server is listening.
    /// \param port TCP port on which the peer's server is listening.
    /// \param peerMode the current status of the peer.
    /// \param parent the parent of the current object.
    ///
    explicit SyfftProtocolSender(const QString &localUuid,
                                 const QString &peerUuid, quint32 address,
                                 quint16 port, PeerStatus peerStatus,
                                 QObject *parent = Q_NULLPTR);

    ///
    /// \brief Returns the current status of the peer.
    ///
    PeerStatus peerStatus() const
    {
        QMutexLocker lk(&m_mutex);
        return m_peerStatus;
    }

    ///
    /// \brief Builds the list of files to be shared and initializes a new
    /// connection.
    /// \param files the object representing the files to be shared.
    /// \param message optional message to be sent to the peer (at most
    /// MAX_MSG_LEN characters).
    ///
    void sendFiles(const TransferList &files,
                   const QString &message = QString());

    ///
    /// \brief Updates the status of the peer.
    /// \param peerStatus the new peer status.
    ///
    void updatePeerStatus(PeerStatus peerStatus);

    ///
    /// \brief Updates the address and the port associated to the peer.
    /// \param address the new IPv4 address of the peer.
    /// \param port the new TCP port of the peer.
    ///
    void updatePeerAddress(quint32 address, quint16 port);

signals:
    ///
    /// \brief Signal emitted when the status of the peer changes.
    /// \param status the new status of the peer.
    ///
    void peerStatusChanged(PeerStatus status);

    ///
    /// \brief Signal emitted if the peer accepts the sharing request.
    /// \param message the message attached by the peer.
    ///
    void accepted(const QString &message);

    ///
    /// \brief Signal emitted if the peer rejects the sharing request.
    /// \param message the message attached by the peer.
    ///
    void rejected(const QString &message);

private slots:
    ///
    /// \brief Connects the current protocol instance to the peer.
    ///
    void connectToPeer();

    ///
    /// \brief Function executed when some data is ready to be read from
    /// the socket.
    ///
    void readData() override;

private:
    ///
    /// \brief Starts the transfer of the next file.
    ///
    void transferNextFile();

    ///
    /// \brief Sends data chunks composing the current file.
    ///
    void sendDataChunks();

    ///
    /// \brief Function executed when an HELLO command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool helloCommand();

    ///
    /// \brief Function executed when an ACCEPT command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool acceptCommand();

    ///
    /// \brief Function executed when a REJECT command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool rejectCommand();

    ///
    /// \brief Function executed when a COMMIT command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool commitCommand();

    ///
    /// \brief Function executed when a ROLLBK command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool rollbkCommand();

    ///
    /// \brief Function executed when a STOP command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool stopCommand();

    ///
    /// \brief Function executed when a CLOSE command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool closeCommand();

    ///
    /// \brief Sets the status the peer is currently in, in a thread-safe
    /// manner.
    /// \param status the status to be set.
    ///
    void setPeerStatus(PeerStatus status)
    {
        QMutexLocker lk(&m_mutex);
        m_peerStatus = status;
    }

private:
    /// \brief The current status of the peer (mutex required).
    PeerStatus m_peerStatus;

    quint32 m_peerAddress; ///< \brief The IPv4 address associated to the peer.
    quint16 m_peerPort;    ///< \brief The TCP port associated to the peer.

    /// \brief The message sent following the SHARE command.
    QString m_shareMsg;
};

#endif // SYFFTPROTOCOLSENDER_HPP

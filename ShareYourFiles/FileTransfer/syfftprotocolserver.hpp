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

#ifndef SYFFTPROTOCOLSERVER_HPP
#define SYFFTPROTOCOLSERVER_HPP

#include <QObject>
#include <QPointer>

class SyfftProtocolReceiver;
class QTcpServer;

///
/// \brief The SyfftProtocolServer class provides the server side implementation
/// of the SYFFT Protocol.
///
/// The SYFFT (Share Your Files Files Transfer) Protocol is the core protocol
/// used to actually transfer data between users of the SYF application.
///
/// When a new connection is received by the server, a brand new instance of
/// SyfftProtocolReceiver is created to encapsulate the connected socket and
/// to perform all the necessary actions, such as completing the connection
/// handshake phase and to receive the actual files. The generated instance
/// is then advertised through the connectionRequested() signal. In case the
/// server is terminated (i.e. the instance is destroyed), all the attached
/// SyfftProtocolReceiver instances still connected are aborted.
///
class SyfftProtocolServer : public QObject
{
    Q_OBJECT

public:
    /// \brief The port value used to signal an error.
    static const quint16 INVALID_PORT = 0xFFFF;

    ///
    /// \brief Builds a new instance of the server.
    /// \param localUuid the UUID associated to the local user.
    /// \param parent the parent of the current object.
    ///
    explicit SyfftProtocolServer(const QString &localUuid, QObject *parent = 0);

    ///
    /// \brief Starts listening for requests.
    /// \param ipv4Address the address chosen for listening.
    /// \return the port chosen for listening.
    /// (or SyfftProtocolServer::INVALID_PORT in case of error).
    ///
    quint16 start(quint32 ipv4Address);

    ///
    /// \brief Destroys the current instance.
    ///
    ~SyfftProtocolServer();

signals:

    ///
    /// \brief Signal emitted when the SYFFT Protocol server is started.
    /// \return port the port chosen for listening.
    ///
    void started(quint16 port);

    ///
    /// \brief Signal emitted when the SYFFT Protocol server is stopped.
    ///
    void stopped();

    ///
    /// \brief Signal emitted when a new connection is attempted by a peer.
    /// \param receiver the receiver instance associated to the connection.
    ///
    void connectionRequested(QSharedPointer<SyfftProtocolReceiver> receiver);

private:
    ///
    /// \brief Function executed when a new connection is ready to be accepted.
    ///
    void newConnection();

private:
    QString m_localUuid; ///< \brief The UUID associated to the local user.
    QPointer<QTcpServer> m_server; ///< \brief The socket used for listening.
};

#endif // SYFFTPROTOCOLSERVER_HPP

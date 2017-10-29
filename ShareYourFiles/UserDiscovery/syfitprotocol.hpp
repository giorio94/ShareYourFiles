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

#ifndef SYFITPROTOCOL_HPP
#define SYFITPROTOCOL_HPP

#include "usericon.hpp"

#include <QObject>
#include <QPointer>

class QTcpServer;
class QTcpSocket;
class QTimer;

///
/// \brief The SyfitProtocolServer class provides the server side implementation
/// of the SYFIT Protocol.
///
/// The SYFIT (Share Your Files Icon Transfer) Protocol is a very simple
/// protocol used to transfer the icon representing an user to the other peers
/// using the SYF application.
///
/// When a new connection is received by the server, it proceeds by sending to
/// the peer a four bytes unsigned integer in little endian order representing
/// the length of the data, followed by the actual icon data (a stream of bytes
/// forming a valid image file in the UserIcon::ICON_FORMAT format); finally,
/// the connection is closed.
///
class SyfitProtocolServer : public QObject
{
    Q_OBJECT

public:
    /// \brief The port value used to signal an error.
    static const quint16 INVALID_PORT = 0xFFFF;

    ///
    /// \brief Constructs a new instance of the server side SYFIT protocol.
    /// \param iconData the actual data to be transferred when requested.
    /// \param parent the parent of the current object.
    ///
    explicit SyfitProtocolServer(const QByteArray &iconData,
                                 QObject *parent = Q_NULLPTR);

    ///
    /// \brief Destroys the current instance.
    ///
    ~SyfitProtocolServer();

public slots:

    ///
    /// \brief Starts listening for requests.
    /// \param ipv4Address the address chosen for listening.
    /// \return the port chosen for listening
    /// (or SyfitProtocolServer::INVALID_PORT in case of error).
    ///
    quint16 start(quint32 ipv4Address);

private:
    ///
    /// \brief Function executed every time a new connection is established.
    ///
    void serveClient();

private:
    /// \brief The socket used for listening.
    QPointer<QTcpServer> m_server;
    /// \brief The length of the data to be transferred (in little endian
    /// order).
    quint32 m_iconLength;
    /// \brief The actual data to be transferred.
    QScopedPointer<QByteArray> m_iconData;
};


///
/// \brief The SyfitProtocolClient class provides the client side implementation
/// of the SYFIT Protocol.
///
/// The SYFIT (Share Your File Icon Transfer) Protocol is a very simple protocol
/// used to transfer the icon representing an user to the other peers using the
/// SYF application.
///
/// The client side of the protocol works by attempting a connection to the
/// SYFIT server specified; then, the expected length of the icon is initially
/// read through the socket, followed by the stream of bytes representing the
/// image. The received data is finally used to try to build a new UserIcon
/// instance and, in case of success, the finished() signal is emitted. If an
/// error occurs at any step, on the other hand, a timer is set to retry the
/// request after the specified time, which doubles at every failure.
///
class SyfitProtocolClient : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a new instance of the client side SYFIT protocol.
    /// \param confPath the base path where configuration files are stored.
    /// \param uuid the UUID identifying the user.
    /// \param parent the parent of the current object.
    ///
    explicit SyfitProtocolClient(const QString &confPath, const QString &uuid,
                                 QObject *parent = Q_NULLPTR);

public slots:
    ///
    /// \brief Updates the cached fields and starts the connection if not yet
    /// running.
    /// \param serverAddress the IPv4 address of the SyfitProtocolServer.
    /// \param serverPort the TCP port of the SyfitProtocolServer.
    /// \param iconHash the expected SHA-1 hash of the icon.
    ///
    void updateAndStart(quint32 serverAddress, quint16 serverPort,
                        const QByteArray &iconHash);

signals:
    ///
    /// \brief Signal emitted when the transfer terminates correctly.
    /// \param icon The instance representing the transferred icon.
    ///
    void finished(UserIcon icon);

private:
    ///
    /// \brief Function executed every time some bytes are ready to be read.
    ///
    void readData();
    ///
    /// \brief Function used to manage an error during the transfer.
    /// \param message the description of the error.
    ///
    void manageError(const QString &message);

    ///
    /// \brief Starts a new connection to the SYFIT server.
    ///
    void startConnection();
    ///
    /// \brief Aborts the current connection.
    ///
    void abortConnection();

private:
    /// \brief The base path where configuration files are stored.
    QString m_confPath;
    QString m_uuid; ///< \brief The UUID identifying the user.

    /// \brief The socket used for the connection.
    QPointer<QTcpSocket> m_socket;
    /// \brief The expected SHA-1 hash representing the icon.
    QScopedPointer<QByteArray> m_expectedHash;

    /// \brief The IPv4 address of the SyfitProtocolServer.
    quint32 m_serverAddress;
    quint16 m_serverPort; ///< \brief The TCP port of the SyfitProtocolServer.

    quint32 m_expectedLength; ///< \brief The expected length of the icon.
    /// \brief The buffer storing the image data already read.
    QScopedPointer<QByteArray> m_buffer;

    /// \brief The timer used to stop too long connections.
    QPointer<QTimer> m_timerTimeout;
    /// \brief Time to be waited in case of error before retrying the
    /// connection.
    int m_retryTime;

    /// \brief Maximum duration of a connection (in ms).
    static const int TIMEOUT = 5000;
    /// \brief Time to be waited in the case of the first error (in ms).
    static const int INITIAL_RETRY_TIME = 15000;
};

#endif // SYFITPROTOCOL_HPP

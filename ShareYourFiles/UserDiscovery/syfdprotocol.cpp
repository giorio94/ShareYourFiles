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

#include "syfdprotocol.hpp"
#include "syfddatagram.hpp"

#include <Logger.h>

#include <QHostAddress>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>

// Static variables definition
const QHostAddress SyfdProtocol::SYFD_ADDRESS = QHostAddress("239.255.101.10");

// Register SyfdDatagram to the qt meta type system
static MetaTypeRegistration<SyfdDatagram> datagramRegisterer("SyfdDatagram");


///
/// The instance is created by instantiating the sender and receiver sockets.
/// The former is bound to the IPv4 address specified by the parameter to be
/// used as source address (the port choice is left to the operating system).
/// The latter, on the other hand, is bound to the multicast address and port
/// used by the protocol, in order to be ready to receive the datagrams.
/// Furthermore, both are attached to the interface given a parameter.
///
/// In case some errors occur during the initialization, an invalid instance
/// of the protocol is returned and a message is attached to the log.
///
SyfdProtocol::SyfdProtocol(const NetworkEntriesList::Entry &entry,
                           QObject *parent)
        : QObject(parent),
          m_valid(false),
          m_status(Status::Stopped),
          m_mode(Enums::OperationalMode::Offline),
          m_sender(new QUdpSocket(this)),
          m_receiver(new QUdpSocket(this)),
          m_timer(new QTimer(this)),
          m_datagram(new SyfdDatagram()),
          m_datagramBuffer(new QByteArray()),
          m_errorCount(0)
{
    LOG_INFO() << "SyfdProtocol: initialization...";

    // Get the network interface to be used and check if it is valid
    QNetworkInterface iface = QNetworkInterface::interfaceFromName(entry.first);
    if (!NetworkEntriesList::validNetworkInterface(iface)) {
        LOG_ERROR() << "SyfdProtocol: invalid interface"
                    << iface.humanReadableName();
        return;
    }

    // Get the IP address to be used and check if it is valid
    QHostAddress localAddress(entry.second);
    if (!NetworkEntriesList::validHostAddress(localAddress)) {
        LOG_ERROR() << "SyfdProtocol: invalid address"
                    << localAddress.toString();
        return;
    }


    // Sender socket initialization
    // Bind to the local address (set source address for outgoing multicast
    // datagrams)
    if (!m_sender->bind(localAddress)) {
        LOG_ERROR() << "SyfdProtocol: failed to bind the sender socket";
        return;
    };

    // Socket options (multicast only on LAN and allow multicast loopback)
    m_sender->setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(1));
    m_sender->setSocketOption(QAbstractSocket::MulticastLoopbackOption,
                              QVariant(1));
    m_sender->setMulticastInterface(iface); // Outgoing interface

    m_localAddress = m_sender->localAddress().toIPv4Address();
    m_localPort = m_sender->localPort();
    LOG_INFO() << "SyfdProtocol: sender initialized correctly, local address"
               << qUtf8Printable(m_sender->localAddress().toString()) << "@"
               << m_sender->localPort();


    // Receiver socket initialization
    // Bind to the SYFDProtocol port;
    if (!m_receiver->bind(QHostAddress::AnyIPv4, SyfdProtocol::SYFD_PORT,
                          QUdpSocket::ShareAddress |
                              QUdpSocket::ReuseAddressHint)) {
        LOG_ERROR() << "SyfdProtocol: failed to bind the receiver socket";
        return;
    }

    // Join the multicast group
    if (!m_receiver->joinMulticastGroup(SyfdProtocol::SYFD_ADDRESS, iface)) {
        LOG_ERROR() << "SyfdProtocol: failed to join the multicast group"
                    << m_receiver->errorString();
        return;
    };

    LOG_INFO() << "SyfdProtocol: receiver initialized correctly, listening on"
               << qUtf8Printable(SyfdProtocol::SYFD_ADDRESS.toString()) << "@"
               << m_receiver->localPort();


    // Initialization completed
    LOG_INFO() << "SyfdProtocol: initialization completed";
    m_valid = true;
}

///
/// In case the protocol is running, it is politely stopped
/// by executing the stop() method before releasing the resources.
///
SyfdProtocol::~SyfdProtocol()
{
    if (m_valid && m_status == Status::Started) {
        stop();
    }
}

///
/// The protocol is started by connecting the handler executed
/// when a datagram is received and starting the timer in charge to
/// periodically send the local datagram (if online mode) and to age
/// the cached peers.
///
/// This function can be executed only if the protocol instance is
/// valid (guaranteed by an assertion) and does nothing if the protocol
/// is already started.
///
void SyfdProtocol::start(Enums::OperationalMode mode,
                         const SyfdDatagram &datagram)
{
    LOG_INFO() << "SyfdProtocol: starting...";

    // Check validity
    LOG_ASSERT_X(m_valid, "SyfdProtocol: trying to start an invalid instance");
    if (m_status == Status::Started) {
        LOG_WARNING() << "SyfdProtocol: already started";
        return;
    }

    // Discard all pending datagrams (if any) to allow
    // readyRead signal to be emitted again
    while (m_receiver->hasPendingDatagrams())
        m_receiver->receiveDatagram(0);

    // S&S connection: receiver socket error handling
    connect(m_receiver,
            static_cast<void (QUdpSocket::*)(QAbstractSocket::SocketError)>(
                &QUdpSocket::error),
            this, [this]() {
                LOG_WARNING() << "SyfdProtocol: receiver socket error:"
                              << m_receiver->errorString();
            });

    // S&S connection: datagram received
    connect(m_receiver, &QUdpSocket::readyRead, this,
            &SyfdProtocol::receiveDatagram);

    // S&S connection: datagram sender timer
    connect(m_timer, &QTimer::timeout, this,
            &SyfdProtocol::sendBufferedDatagram);

    // Update the buffered datagram
    updateDatagram(datagram);

    LOG_INFO() << "SyfdProtocol: started";
    m_status = SyfdProtocol::Status::Started;
    emit started();

    // Move to online mode if necessary
    m_mode = Enums::OperationalMode::Offline;
    setMode(mode);
}

///
/// The protocol is stopped by disconnecting the handler executed
/// when a datagram is received, stopping the timer in charge to
/// periodically send the local datagram (if online mode) and to age
/// the cached peers, and sending a quit SyfdDatagram (i.e. with the
/// quit flag set) if the mode was online.
///
/// This function can be executed only if the protocol instance is
/// valid (guaranteed by an assertion) and does nothing if the protocol
/// is already stopped.
///
void SyfdProtocol::stop()
{
    LOG_INFO() << "SyfdProtocol: stopping...";

    // Check validity
    LOG_ASSERT_X(m_valid, "SyfdProtocol: trying to stop an invalid instance");
    if (m_status == Status::Stopped) {
        LOG_WARNING() << "SyfdProtocol: already stopped";
        return;
    }

    // Go offline before stopping the protocol
    setMode(Enums::OperationalMode::Offline);

    // Signal slot disconnections
    disconnect(m_timer, nullptr, nullptr, nullptr);
    disconnect(m_receiver, nullptr, nullptr, nullptr);
    disconnect(m_sender, nullptr, nullptr, nullptr);

    LOG_INFO() << "SyfdProtocol: stopped";
    m_status = SyfdProtocol::Status::Stopped;
    emit stopped();
}

///
/// This function allows to switch between offline mode (the SyfdDatagram
/// representing the local user is not advertised and therefore will not
/// be detected by the other users) and online mode (it is advertised).
///
/// In case the chosen mode is equal to the old one, nothing is done.
///
void SyfdProtocol::setMode(Enums::OperationalMode mode, bool emitSignal)
{
    // Same mode as before
    if (m_mode == mode)
        return;

    // If the protocol is not running, it is impossible to change the mode
    if (m_status == SyfdProtocol::Status::Stopped) {
        LOG_ERROR()
            << "SyfdProtocol: impossible going online, protocol stopped";
    }

    // State: online, restart sending the datagram
    if (mode == Enums::OperationalMode::Online) {
        // In case the datagram is not valid, abort the change
        if (m_datagramBuffer->isEmpty()) {
            LOG_ERROR()
                << "SyfdProtocol: failed going online, invalid datagram";
            emit modeChanged(Enums::OperationalMode::Offline);
            return;
        }

        // Start the timer
        m_timer->start(SyfdProtocol::SYDF_INTERVAL);
    }

    // State: offline, stop sending the datagram after having
    // advertised the quit one.
    else {
        LOG_INFO() << "SyfdProtocol: going offline...";
        sendQuitDatagram();
        m_timer->stop();
    }

    m_errorCount = 0;
    m_mode = mode;
    LOG_INFO() << "SyfdProtocol: mode changed to"
               << qUtf8Printable(enum2str(m_mode));

    if (emitSignal) {
        emit modeChanged(mode);
    }
}

///
/// This function updates the buffered datagram advertised through the network.
/// In case the parameter is not valid, the buffered datagram is cleared
///
void SyfdProtocol::updateDatagram(const SyfdDatagram &datagram)
{
    // Store the obtained datagram instance and clean the buffered array
    *m_datagram = datagram;
    m_datagramBuffer->clear();

    // If the datagram is valid, buffer it for dispatch
    if (m_datagram->valid()) {
        m_datagramBuffer->append(m_datagram->toByteArray());
    }

    LOG_INFO() << "SyfdProtocol: local datagram updated";
}

///
/// Function used both by sendBufferedDatagram() and sendQuitDatagram()
/// to actually send the datagram. In case of error, a message is
/// attached to the log, the error count is incremented and, if the error
/// threshold is reached, the operational mode is changed to offline and
/// the error() signal emitted.
///
/// \see sendBufferedDatagram()
/// \see sendQuitDatagram()
///
void SyfdProtocol::sendDatagram(const QByteArray &datagram)
{
    if (m_sender->writeDatagram(datagram, SyfdProtocol::SYFD_ADDRESS,
                                SyfdProtocol::SYFD_PORT) != datagram.length()) {
        LOG_WARNING() << "SyfdProtocol: error while sending datagram"
                      << m_sender->errorString();

        // Increment the error count
        if (++m_errorCount == SyfdProtocol::ERROR_THRESHOLD) {
            // Go offline and emit the error signal
            LOG_ERROR() << "SyfdProtocol: error condition detected";
            setMode(Enums::OperationalMode::Offline);
            emit error();
        }
        return;
    }

    // Reset the error count
    m_errorCount = 0;
}

///
/// Every time this function is executed (when the timer timeouts),
/// the buffered SyfdDatagram representing the local user is sent
/// through the Local Area Network. In case the buffered datagram is
/// not valid (i.e. the buffer is empty), the mode is switched to Offline.
///
void SyfdProtocol::sendBufferedDatagram()
{
    // If the buffered datagram is valid, sent it
    if (!m_datagramBuffer->isEmpty()) {
        sendDatagram(*m_datagramBuffer);
    }

    // Otherwise go offline
    else {
        LOG_ERROR() << "SyfdProtocol: invalid datagram detected for output";
        setMode(Enums::OperationalMode::Offline);
    }
}

///
/// This function advertises a special SyfdDatagram characterized
/// by the quit flag set, which tells the other peers that this
/// user is going to disconnect (to make detection faster than
/// through the aging time).
///
void SyfdProtocol::sendQuitDatagram()
{
    // Send the quit datagram only if the current instance is valid and
    // not too errors already occurred
    if (m_datagram->valid() && m_errorCount < SyfdProtocol::ERROR_THRESHOLD) {
        LOG_INFO() << "SyfdProtocol: sending quit SYFD datagram...";

        m_datagram->setFlagQuit();
        QByteArray buffer(m_datagram->toByteArray());
        m_datagram->clearFlagQuit();

        // Send it
        sendDatagram(buffer);
        m_sender->flush();
    }
}

///
/// This function is executed every time a datagram is ready to be read.
/// Some checks are initially performed to guarantee that it has the
/// expected size (according to the SyfdDatagram specifications), then
/// the datagram is received and discarded in case it is the one sent
/// by the local socket (multicast datagrams are looped back).
/// The raw array of bytes is then converted to a SyfdDatagram and, if
/// valid, it is used to update the list of known peers.
///
void SyfdProtocol::receiveDatagram()
{
    // Repeat until there are pending datagrams
    while (m_receiver->hasPendingDatagrams()) {

        // Check if the size is compatible with expected values
        if (m_receiver->pendingDatagramSize() <
                SyfdDatagram::MIN_DATAGRAM_SIZE ||
            m_receiver->pendingDatagramSize() >
                SyfdDatagram::MAX_DATAGRAM_SIZE) {

            // Discard the datagram since it has the wrong size
            QNetworkDatagram datagram = m_receiver->receiveDatagram(0);
            if (datagram.isValid()) {
                LOG_WARNING()
                    << "SyfdProtocol: wrong sized datagram received from"
                    << qUtf8Printable(datagram.senderAddress().toString())
                    << "@" << datagram.senderPort();
            } else {
                LOG_WARNING() << "SyfdProtocol: wrong sized datagram received";
            }

            continue;
        }

        // Read the whole datagram
        QNetworkDatagram datagram = m_receiver->receiveDatagram();
        if (!datagram.isValid()) {
            LOG_WARNING() << "SyfdProtocol: invalid datagram received";
            continue;
        }

        // Discard my own datagrams looped back
        if (datagram.senderAddress().toIPv4Address() == m_localAddress &&
            datagram.senderPort() == m_localPort) {
            continue;
        }

        // Datagram processing
        SyfdDatagram syfdDatagram(datagram.data());
        if (!syfdDatagram.valid()) {
            LOG_WARNING() << "SyfdProtocol: invalid datagram received";
            continue;
        }

        emit datagramReceived(syfdDatagram);
    }
}

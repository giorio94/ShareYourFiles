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

#include "syfftprotocolcommon.hpp"
#include "Common/common.hpp"
#include "fileintransfer.hpp"
#include "transferinfo.hpp"

#include <Logger.h>

#include <QDataStream>
#include <QElapsedTimer>
#include <QTcpSocket>
#include <QTimer>

// Static variables definition
const QString SyfftProtocolCommon::UNKNOWN_UUID("Unknown");
const quint64 SyfftProtocolCommon::MAX_BUFFER_SIZE =
    FileInTransfer::MAX_CHUNK_SIZE * 8;
QAtomicInteger<quint32> SyfftProtocolCommon::m_counter;

// Register SyfftProtocolCommon::Status to the qt meta type system
static MetaTypeRegistration<SyfftProtocolCommon::Status>
    statusRegisterer("Status");
// Register TransferInfo to the qt meta type system
static MetaTypeRegistration<TransferInfo>
    transferInfoRegisterer("TransferInfo");

///
/// The instance is initialized by generating a new id through the counter,
/// by copying the various parameters to the members and by setting the
/// necessary options and handlers for the socket. The timers used for the
/// tick() signal emission and as a timeout are also initialized.
///
SyfftProtocolCommon::SyfftProtocolCommon(const QString &localUuid,
                                         const QString &peerUuid,
                                         QTcpSocket *socket, QObject *parent)
        : QObject(parent),
          m_id(++m_counter),
          m_localUuid(localUuid),
          m_peerUuid(peerUuid),
          m_status(Status::New),
          m_transferInfo(new TransferInfo()),
          m_currentFile(0xFFFFFFFF),
          m_socket(socket),
          m_stream(new QDataStream(m_socket)),
          m_elapsedTimer(new QElapsedTimer()),
          m_transferTimer(new QElapsedTimer()),
          m_pauseTimer(new QElapsedTimer()),
          m_preventUserTogglePause(false)
{
    // Take ownership of the socket
    m_socket->setParent(this);

    // Set the options
    m_socket->setReadBufferSize(SyfftProtocolCommon::MAX_BUFFER_SIZE);
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));
    m_stream->setVersion(QDataStream::Version::Qt_5_0);
    m_stream->setByteOrder(QDataStream::ByteOrder::LittleEndian);

    // Connect to the handler in case of error
    connect(m_socket,
            static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(
                &QTcpSocket::error),
            this, [this](QAbstractSocket::SocketError error) {

                // If the connection has been paused and the peer closed the
                // connection, exit the pause mode and read the remaining data
                if (m_status == Status::PausedByUser &&
                    error ==
                        QAbstractSocket::SocketError::RemoteHostClosedError) {

                    m_preventUserTogglePause = false;
                    togglePauseMode(true);

                    // Emit again the signal in case the error is not solved
                    QTimer::singleShot(0, m_socket, [this, error]() {
                        emit m_socket->error(error);
                    });
                    return;
                }

                // Otherwise, print the error and abort the connection
                if (m_status != Status::Aborted && m_status != Status::Closed) {
                    manageError(m_socket->errorString());
                }
            });

    // Connect to the handler for bytes ready to be read from the socket
    connect(m_socket, &QTcpSocket::readyRead, this,
            &SyfftProtocolCommon::readData);

    // Connect to the handler for socket disconnection
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        // The status must be closing, otherwise the connection has been aborted
        if (m_status == Status::Closing) {

            LOG_INFO() << qUtf8Printable(logSyfftId()) << "connection closed";
            setStatus(Status::Closed);

            QMutexLocker lk(&m_mutex);
            m_transferInfo->m_elapsedTime = m_elapsedTimer->elapsed();
            lk.unlock();

            // Emit the signals after having released the locks
            emit statusChanged(Status::Closed);
            emit closed();
        }
    });

    LOG_INFO() << qUtf8Printable(logSyfftId()) << "instance created";
}

///
/// The instance is deleted by aborting the active connection (if any) and
/// by letting the destructors to destroy all the members.
///
SyfftProtocolCommon::~SyfftProtocolCommon()
{
    abortConnection();
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "instance destroyed";
}

///
/// Before returning the requested object, the function updates the elapsed
/// time and recomputes the current speed (in case the connection is InTransfer
/// status). The lock is acquired to guarantee that the information are not
/// modified during the update and the copy.
///
TransferInfo SyfftProtocolCommon::transferInfo()
{
    QMutexLocker lk(&m_mutex);

    // Connection not established
    if (m_status == Status::New || m_status == Status::Closed ||
        m_status == Status::Aborted) {
        return *m_transferInfo;
    }

    // Update the total elapsed time
    m_transferInfo->m_elapsedTime = m_elapsedTimer->elapsed();

    // Transfer in progress
    if (m_status == Status::InTransfer) {
        m_transferInfo->m_transferTime += m_transferTimer->restart();
        m_transferInfo->recomputeCurrentSpeed();
    }

    // Connection paused
    if (m_status == Status::PausedByUser || m_status == Status::PausedByPeer) {
        m_transferInfo->m_pausedTime += m_pauseTimer->restart();
    }

    return *m_transferInfo;
}

///
/// The pause mode is modified, according to the specified request, by the
/// togglePauseMode() method, which is invoked through a timer in order to
/// execute it in the thread owning the current object
///
void SyfftProtocolCommon::changePauseMode(bool enterPauseMode)
{
    // Use a timer to execute the function in the thread owning the this object
    QTimer::singleShot(0, this, [this, enterPauseMode]() {

        // If already in the requested mode, just return
        if ((enterPauseMode && m_status == Status::PausedByUser) ||
            (!enterPauseMode && m_status != Status::PausedByUser)) {
            return;
        }

        // Do the actual operation
        togglePauseMode(true);
    });
}

///
/// The active connection is terminated through the abortConnection() method
/// which is invoked indirectly to guarantee that it is executed by the same
/// thread owning the current object: this means that, when the current
/// function returns, the connection may have not yet been terminated.
///
void SyfftProtocolCommon::terminateConnection()
{
    // Use a timer to execute abortConnection() from the thread owning this
    // object
    QTimer::singleShot(0, this, &SyfftProtocolCommon::abortConnection);
}

///
/// The function, in case the connection is established, requests the socket
/// disconnection from the remote host and changes the status to Closing.
/// When the socket is actually disconnected, the status will move to Closed
/// and the closed() signal will be emitted. In this way, when that signal is
/// emitted, all the data has been correctly sent to the peer.
///
void SyfftProtocolCommon::closeConnection()
{
    // Connection already closed: nothing to do
    if (m_status == Status::New || m_status == Status::Closing ||
        m_status == Status::Closed || m_status == Status::Aborted) {
        return;
    }

    // Set the closing status
    setStatus(Status::Closing);

    // Emit the statusChanged signal
    emit statusChanged(Status::Closing);

    // If the socket has already been disconnected, just re-trigger the signal
    if (m_socket->state() == QTcpSocket::UnconnectedState) {
        emit m_socket->disconnected();
        return;
    }

    // Send the Close command and disconnect from the peer.
    if (m_socket->isValid() &&
        m_socket->state() == QTcpSocket::ConnectedState) {
        *m_stream << static_cast<CommandType>(Command::CLOSE);
    }
    m_socket->disconnectFromHost();
}

///
/// The function, in case the connection is established, sends the ABORT
/// command, tries to flush the data still to be sent and then aborts the
/// communication channel. Since it is possible that not all the buffer is
/// flushed, it could happen that the ABORT command is not received by the peer.
/// The connection status is then changed accordingly and the statusChanged()
/// and the aborted() signals are emitted.
///
void SyfftProtocolCommon::abortConnection()
{
    // Connection already closed: nothing to do
    if (m_status == Status::New || m_status == Status::Closed ||
        m_status == Status::Aborted) {
        return;
    }

    // Set immediately the state to prevent loops if a failure occurs
    // while sending the abort code
    setStatus(Status::Aborted);

    // Update the transfer statistics
    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_fileInTransfer = QString();
    m_transferInfo->m_elapsedTime = m_elapsedTimer->elapsed();
    m_transferInfo->recomputeCurrentSpeed(true);

    m_transferInfo->m_skippedFiles += m_transferInfo->remainingFiles();
    m_transferInfo->m_skippedBytes += m_transferInfo->remainingBytes();

    // Delete the file in transfer instance and mark the current file as
    // TransferFailed (if any)
    if (m_currentFile < m_transferInfo->totalFiles()) {
        m_files[static_cast<int>(m_currentFile)].setStatus(
            FileInfo::Status::TransferFailed);
    }
    m_fileInTransfer.reset();
    lk.unlock();

    // Send the abort code (if the socket is valid and connected)
    if (m_socket->isValid() &&
        m_socket->state() == QTcpSocket::ConnectedState) {
        m_stream->resetStatus();
        *m_stream << static_cast<CommandType>(Command::ABORT);
        m_socket->flush();
    }

    // Abort the socket
    m_socket->abort();

    LOG_ERROR() << qUtf8Printable(logSyfftId()) << "connection aborted";

    // Emit the signals
    emit statusChanged(Status::Aborted);
    emit aborted();
}

///
/// The error message is printed to the log and the connection is then aborted.
///
void SyfftProtocolCommon::manageError(const QString &message)
{
    LOG_ERROR() << qUtf8Printable(logSyfftId()) << qUtf8Printable(message);

    // Abort the faulty connection
    abortConnection();
}

///
/// The function, after having verified that the connection has already been
/// established, sends the pause command to the peer user. The status is then
/// updated and the statusChanged() signal is emitted.
///
void SyfftProtocolCommon::togglePauseMode(bool userRequested)
{
    // If the connection is not established, just return
    if (m_status == Status::New || m_status == Status::Aborted ||
        m_status == Status::Closing || m_status == Status::Closed) {
        return;
    }

    // If user requested and prevented, just return
    if (userRequested && m_preventUserTogglePause) {
        return;
    }

    Status pauseStatus =
        userRequested ? Status::PausedByUser : Status::PausedByPeer;

    // If it is requested by the local user, send the PAUSE command
    if (userRequested) {
        // Do not try to send the command if the socket has already been closed
        if (m_socket->isValid() &&
            m_socket->state() == QTcpSocket::ConnectedState) {

            m_stream->resetStatus();
            *m_stream << static_cast<CommandType>(Command::PAUSE);
        }
    }

    // If already paused, reset the previous status
    if (m_status == pauseStatus) {

        LOG_ASSERT_X(!m_oldStatusStack.isEmpty(),
                     qUtf8Printable(logSyfftId() + "empty stack"));
        Status oldStatus = m_oldStatusStack.pop();
        setStatus(oldStatus);
        emit statusChanged(oldStatus);

        // If no more paused, update the transfer information and restart the
        // transfer
        if (m_oldStatusStack.isEmpty()) {
            QMutexLocker lk(&m_mutex);
            m_transferInfo->m_pausedTime += m_pauseTimer->restart();
            m_transferTimer->start();
            lk.unlock();
        }

        // Emit the signals to read the buffered data and to restart
        // transferring a file (if necessary). Use a timer to emit
        // them the next time the event loop will be entered
        QTimer::singleShot(0, this, [this]() {
            emit m_socket->readyRead();
            emit m_socket->bytesWritten(0);
        });
    }

    // Otherwise save the previous status and go to pause
    else {
        // If not yet paused, update the transfer information
        if (m_oldStatusStack.isEmpty()) {
            QMutexLocker lk(&m_mutex);
            if (m_status == Status::InTransfer) {
                m_transferInfo->m_transferTime += m_transferTimer->restart();
                m_transferInfo->recomputeCurrentSpeed(true);
            }
            m_pauseTimer->start();
            lk.unlock();
        }

        m_oldStatusStack.push(m_status);
        setStatus(pauseStatus);

        emit statusChanged(pauseStatus);
    }
}

///
/// The function advances the current file counter and then it checks if all
/// the files has already been transferred. In this case, the status is changed
/// to TransferCompleted, the statusChanged() signal is emitted and the
/// connection is closed.
///
bool SyfftProtocolCommon::moveToNextFile()
{
    m_currentFile++;

    // Already transferred all files
    QMutexLocker lk(&m_mutex);
    if (m_currentFile == m_transferInfo->totalFiles()) {
        // Update the transfer information and the status
        m_transferInfo->m_fileInTransfer = QString();
        m_transferInfo->m_transferTime += m_transferTimer->elapsed();
        m_transferInfo->recomputeCurrentSpeed(true);

        m_status = Status::TransferCompleted;
        lk.unlock();

        LOG_INFO() << qUtf8Printable(logSyfftId()) << "transfer completed";

        // Emit the signals
        emit statusChanged(Status::TransferCompleted);
        emit transferCompleted();

        // Start closing the connection
        closeConnection();
        return false;
    }

    m_transferInfo->m_fileInTransfer =
        m_files.at(static_cast<int>(m_currentFile)).filePath();
    return true;
}

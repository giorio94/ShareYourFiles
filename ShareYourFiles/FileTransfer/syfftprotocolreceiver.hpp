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

#ifndef SYFFTPROTOCOLRECEIVER_HPP
#define SYFFTPROTOCOLRECEIVER_HPP

#include "syfftprotocolcommon.hpp"

///
/// \brief The SyfftProtocolReceiver class provides an implementation of the
/// receiving side of the SYFFT protocol.
///
/// \see SyfftProtocolCommon
///
class SyfftProtocolReceiver : public SyfftProtocolCommon
{
    Q_OBJECT

public:
    ///
    /// \brief The DuplicatedFileAction enum represents the possible actions
    /// that to be taken when a file already exists.
    ///
    enum class DuplicatedFileAction {
        Replace,  ///< \brief Replace the current file with the one received.
        Keep,     ///< \brief Keep the current file.
        KeepBoth, ///< \brief Keep both files (the received one with a suffix).
        Ask       ///< \brief Ask the user for the action to be performed.
    };
    Q_ENUM(DuplicatedFileAction)

    ///
    /// \brief Constructs a new instance of SYFFT Protocol Receiver.
    /// \param localUuid the UUID representing the local user.
    /// \param socket the connected socket to be used for the communication.
    /// \param parent the parent of the current object.
    ///
    explicit SyfftProtocolReceiver(const QString &localUuid, QTcpSocket *socket,
                                   QObject *parent = Q_NULLPTR);

    ///
    /// \brief Starts the handshake to complete the connection.
    /// \param shareHandler the object designated to accept or reject the
    /// sharing request.
    /// \param shareHandlerSlot the method designated to accept or reject the
    /// sharing request.
    /// \param duplicateHandler the object designated to decide the action in
    /// case of duplicated files.
    /// \param duplicateHandlerSlot the method designated to decide the action
    /// in case of duplicated files.
    ///
    void acceptConnection(QObject *shareHandler, const char *shareHandlerSlot,
                          QObject *duplicateHandler,
                          const char *duplicateHandlerSlot);

private slots:
    ///
    /// \brief Function executed when some data is ready to be read from
    /// the control channel.
    ///
    void readData() override;

private:
    ///
    /// \brief Function executed when an HELLO command is received-
    /// \return true in case of success or false if some error occurs.
    ///
    bool helloCommand();

    ///
    /// \brief Function executed when an ACK command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool ackCommand();

    ///
    /// \brief Function executed when a SHARE command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool shareCommand();

    ///
    /// \brief Function executed when an ITEM command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool itemCommand();

    ///
    /// \brief Function executed when a START command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool startCommand();

    ///
    /// \brief Function executed when a SKIP command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool skipCommand();

    ///
    /// \brief Function executed when a CHUNK command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool chunkCommand();

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
    /// \brief Function executed when a CLOSE command is received.
    /// \return true in case of success or false if some error occurs.
    ///
    bool closeCommand();

    ///
    /// \brief Accepts the sharing request.
    /// \param path the path where received files will be stored.
    /// \param message an optional message to the peer.
    ///
    void acceptSharingRequest(const QString &path, const QString &message);

    ///
    /// \brief Rejects the sharing request.
    /// \param message an optional message to the peer.
    ///
    void rejectSharingRequest(const QString &message);

    ///
    /// \brief Accepts the reception of the current file.
    ///
    void acceptFileTransfer();

    ///
    /// \brief Rejects the reception of the current file.
    ///
    void rejectFileTransfer();

    ///
    /// \brief Performs the chosen action regarding the duplicated file.
    /// \param action the action to be carried on.
    ///
    void performDFAction(DuplicatedFileAction action);

private:
    /// \brief The object designated to accept or reject the sharing request.
    QObject *m_shareHandler;
    /// \brief The method designated to accept or reject the sharing request.
    QByteArray m_shareHandlerSlot;

    /// \brief The object designated to manage duplicated files.
    QObject *m_duplicatedHandler;
    /// \brief The method designated to manage duplicated files.
    QByteArray m_duplicatedHandlerSlot;

    /// \brief The action to be performed when a duplicated file is detected.
    DuplicatedFileAction m_defaultDFAction;

    /// \brief The message received following the SHARE command.
    QString m_shareMsg;
};


///
/// \brief The SyfftProtocolSharingRequest class represents a sharing request
/// received through the SYFFT protocol.
///
/// This utility class contains all the elements characterizing a sharing
/// request: the total number of files and their total size, the complete
/// list containing the details about each file and the message attached by
/// the peer. The class, furthermore, contains to methods that allow the user
/// to either accept or reject the request, optionally specifying a textual
/// response.
///
class SyfftProtocolSharingRequest : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief The type used to represent an iterator for the FileInfo list.
    ///
    typedef QVector<FileInfo>::ConstIterator FileInfoIterator;

    ///
    /// \brief Builds a new instance of this class.
    /// \param senderUuid the identifier of the sender.
    /// \param totalFiles the total number of files to be transferred.
    /// \param totalSize the total size of the files to be transferred.
    /// \param files the list of files to be transferred.
    /// \param message the message attached to the sharing request.
    /// \param parent the parent of the current object.
    ///
    explicit SyfftProtocolSharingRequest(const QString &senderUuid,
                                         quint32 totalFiles, quint64 totalSize,
                                         const QVector<FileInfo> &files,
                                         const QString &message,
                                         QObject *parent = Q_NULLPTR)
            : QObject(parent),
              m_senderUuid(senderUuid),
              m_totalFiles(totalFiles),
              m_totalSize(totalSize),
              m_files(files),
              m_message(message),
              m_chosen(false)
    {
    }

    ///
    /// \brief Destroys the current instance.
    ///
    /// In case a decision has not be taken, the file transfer is rejected.
    ///
    ~SyfftProtocolSharingRequest() { reject(); }

    /// \brief Returns the identifier of the sender.
    QString senderUuid() const { return m_senderUuid; }

    /// \brief Returns the total number of files to be transferred.
    quint32 totalFiles() const { return m_totalFiles; }
    /// \brief Returns the total size of the files to be transferred.
    quint64 totalSize() const { return m_totalSize; }

    /// \brief Returns a constant iterator to the first element of the list of
    /// files to be transferred.
    FileInfoIterator filesBegin() const { return m_files.constBegin(); }

    /// \brief Returns a constant iterator to the past the end element of the
    /// list of files to be transferred.
    FileInfoIterator filesEnd() const { return m_files.constEnd(); }

    /// \brief Returns the message attached to the sharing request.
    QString message() const { return m_message; }

    ///
    /// \brief Accepts the sharing request.
    /// \param path the path where received files will be stored.
    /// \param message an optional message to the peer.
    ///
    void accept(const QString &path, const QString &message = QString())
    {
        if (!m_chosen) {
            m_chosen = true;
            emit accepted(path, message);
        }
    }

    ///
    /// \brief Rejects the sharing request.
    /// \param message an optional message to the peer.
    ///
    void reject(const QString &message = QString())
    {
        if (!m_chosen) {
            m_chosen = true;
            emit rejected(message);
        }
    }

signals:
    ///
    /// \brief Signal emitted if the sharing request is accepted.
    /// \param path the path where received files will be stored.
    /// \param message an optional message to the peer (at most MAX_MSG_LEN
    /// characters).
    ///
    void accepted(const QString &path, const QString &message);

    ///
    /// \brief Signal emitted if the sharing request is rejected.
    /// \param message an optional message to the peer (at most MAX_MSG_LEN
    /// characters). .
    ///
    void rejected(const QString &message);

    ///
    /// \brief Signal emitted if the connection is aborted during the choice.
    ///
    void connectionAborted();

private:
    const QString m_senderUuid; ///< \brief The identifier of the sender.

    /// \brief The total number of files to be transferred.
    quint32 m_totalFiles;
    /// \brief The total size of the files to be transferred.
    quint64 m_totalSize;

    QVector<FileInfo> m_files; ///< \brief The list of files to be transferred.

    QString m_message; ///< \brief The message attached to the sharing request.

    /// \brief Indicates whether the connection has already been
    /// accepted/rejected or not.
    bool m_chosen;
};

///
/// \brief The SyfftProtocolDuplicatedFile class represents a file ready to be
/// received that already exists on disk.
///
/// This utility class contains the information about both the file already on
/// disk and about the one that is going to be received. The class, furthermore,
/// contains to methods that allow the user to decide how to manage the
/// situation: it is possible to keep the old file and reject the transfer,
/// replace the file on disk with the one received, or keep both and add a
/// suffix to the name of the one obtained through the transfer.
///
class SyfftProtocolDuplicatedFile : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief A synonym for SyfftProtocolReceiver::DuplicatedFileAction.
    ///
    typedef SyfftProtocolReceiver::DuplicatedFileAction DuplicatedFileAction;

    ///
    /// \brief Builds a new instance of this class.
    /// \param senderUuid the identifier of the sender.
    /// \param currentFile the information about the file stored on disk.
    /// \param receivedFile the information about the file to be received.
    /// \param parent the parent of the current object.
    ///
    explicit SyfftProtocolDuplicatedFile(const QString &senderUuid,
                                         const QFileInfo &currentFile,
                                         const FileInfo &receivedFile,
                                         QObject *parent = Q_NULLPTR)
            : QObject(parent),
              m_senderUuid(senderUuid),
              m_currentFile(currentFile),
              m_receivedFile(receivedFile),
              m_chosen(false)
    {
    }

    ///
    /// \brief Destroys the current instance.
    ///
    /// In case a decision has not be taken, the current file is kept.
    ///
    ~SyfftProtocolDuplicatedFile() { keep(); }

    /// \brief Returns the identifier of the sender.
    QString senderUuid() const { return m_senderUuid; }

    /// \brief Returns the information about the file stored on disk.
    QFileInfo currentFile() const { return m_currentFile; }
    /// \brief Returns the information about the file to be received.
    FileInfo receivedFile() const { return m_receivedFile; }

    ///
    /// \brief Requests to replace the current file.
    /// \param all if true, sets the current as the default action in this
    /// session.
    ///
    void replace(bool all = false)
    {
        if (!m_chosen) {
            m_chosen = true;
            emit chosen(DuplicatedFileAction::Replace, all);
        }
    }

    ///
    /// \brief Requests to keep the current file.
    /// \param all if true, sets the current as the default action in this
    /// session.
    ///
    void keep(bool all = false)
    {
        if (!m_chosen) {
            m_chosen = true;
            emit chosen(DuplicatedFileAction::Keep, all);
        }
    }

    ///
    /// \brief Requests to keep the current file and to add a suffix to the
    /// received one.
    /// \param all if true, sets the current as the default action in this
    /// session.
    ///
    void keepBoth(bool all = false)
    {
        if (!m_chosen) {
            m_chosen = true;
            emit chosen(DuplicatedFileAction::KeepBoth, all);
        }
    }

signals:
    ///
    /// \brief Signal emitted when the action to be performed is chosen.
    /// \param action the action to be performed.
    /// \param all if true, sets the current as the default action in this
    /// session.
    ///
    void chosen(DuplicatedFileAction action, bool all);

    ///
    /// \brief Signal emitted if the connection is aborted during the choice.
    ///
    void connectionAborted();

private:
    const QString m_senderUuid; ///< \brief The identifier of the sender.

    QFileInfo m_currentFile; ///< \brief The file stored on the disk.
    FileInfo m_receivedFile; ///< \brief The file to be received.

    /// \brief Indicates whether the connection has already been
    /// accepted/rejected or not.
    bool m_chosen;
};

#endif // SYFFTPROTOCOLRECEIVER_HPP

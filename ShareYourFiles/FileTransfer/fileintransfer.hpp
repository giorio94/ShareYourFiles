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

#ifndef FILEINTRANSFER_HPP
#define FILEINTRANSFER_HPP

#include "fileinfo.hpp"

#include <QDir>
#include <QFile>
#include <QSaveFile>

///
/// \brief The FileInTransfer class represents the file currently in transfer by
/// the SYFFT protocol.
///
/// This abstract class provides a common interface for the main operations that
/// must be performed to the on-disk files, both in the sender (reader) and the
/// receiver (writer) sides: opening the file (performed by the constructor),
/// reading from or writing to the file the next chunk of data, committing the
/// performed operations by checking that everything completed correctly or
/// rollbacking them and restoring the previous status. Getter methods are also
/// provided to query information about the file in transfer and its status.
///
class FileInTransfer
{
public:
    /// \brief The maximum amount of bytes allowed to be read or written at
    /// once.
    static const quint64 MAX_CHUNK_SIZE = 8192;

    ///
    /// \brief Deleted default constructor.
    ///
    explicit FileInTransfer() = delete;

    ///
    /// \brief Deleted copy constructor.
    ///
    FileInTransfer(const FileInTransfer &) = delete;

    ///
    /// \brief Deleted assignment operator.
    ///
    FileInTransfer &operator=(const FileInTransfer &) = delete;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    virtual ~FileInTransfer() = default;

    /// \brief Returns whether the file already exists (when the instance is
    /// created).
    bool exists() const { return m_exists; }

    /// \brief Returns whether an error occurred or not.
    bool error() const { return m_error; }

    /// \brief Returns the absolute path of the file.
    QString absolutePath() const { return m_absolutePath; }

    /// \brief Returns the path of the file relative to the base path.
    QString relativePath() const { return m_fileInfo.filePath(); }

    /// \brief Returns the number of bytes not yet transferred.
    quint64 remainingBytes() const { return m_remainingBytes; }

    ///
    /// \brief Returns whether the transfer has already been started or not.
    /// \return true if a chunk has already been processed and false otherwise.
    ///
    bool transferStarted() const { return m_transferStarted; }

    ///
    /// \brief Returns whether the transfer has already been completed or not.
    /// \return true if already committed or rollbacked and false otherwise.
    ///
    bool transferCompleted() const { return m_committed || m_rollbacked; }

    ///
    /// \brief Returns whether the transfer has already been committed.
    /// \return true if the file has been committed successfully and false
    /// otherwise.
    ///
    bool committed() const { return m_committed; }

    ///
    /// \brief Returns whether the transfer has already been rollbacked.
    /// \return true if the file has been rollbacked and false otherwise.
    ///
    bool rollbacked() const { return m_rollbacked; }

    ///
    /// \brief Processes the next chunk of data.
    /// \param buffer the data to be processed.
    /// \return a value indicating whether the data has been correctly processed
    /// or not.
    ///
    virtual bool processNextDataChunk(QByteArray &buffer) = 0;

    ///
    /// \brief Commits the file transfer.
    /// \return true if the operation succeeded and false otherwise (some errors
    /// occurred).
    ///
    virtual bool commit() = 0;

    ///
    /// \brief Rollbacks the file transfer.
    /// \return true if the operation succeeded and false otherwise (already
    /// committed).
    ///
    virtual bool rollback() = 0;

protected:
    ///
    /// \brief Builds a new instance from the parameters.
    /// \param basePath the path the file is relative to.
    /// \param fileInfo the information about the file to be transferred.
    ///
    explicit FileInTransfer(const QDir &basePath, const FileInfo &fileInfo);

protected:
    FileInfo m_fileInfo;    ///< \brief The information about the file.
    QString m_absolutePath; ///< \brief The absolute path of the file.

    /// \brief A value indicating whether an error occurred or not.
    bool m_error;

    /// \brief A value indicating whether the file already exists (when the
    /// instance is created).
    bool m_exists;

    /// \brief The amount of bytes still to be transferred.
    quint64 m_remainingBytes;

    /// \brief A value indicating whether the transfer has already been started.
    bool m_transferStarted;

    /// \brief A value indicating whether the transfer has already been
    /// committed.
    bool m_committed;
    /// \brief A value indicating whether the transfer has already been
    /// rollbacked.
    bool m_rollbacked;
};


///
/// \brief The FileInTransferReader class represents the file currently read
/// from disk and sent to the peer by the SYFFT protocol.
///
/// This class, which extends FileInTransfer, provides the implementation of the
/// methods necessary to guarantee that a file is sent through the protocol, and
/// then committed, only if no error occurred and the file itself has not been
/// modified externally. In particular, both when the instance is created and
/// when the commit is requested, it is checked that the main characteristics
/// of the file have not changed, and otherwise the commit is prevented.
///
class FileInTransferReader : public FileInTransfer
{
public:
    ///
    /// \brief Builds a new instance from the parameters and opens the file.
    /// \param basePath the path the file is relative to.
    /// \param fileInfo the information about the file to be transferred.
    ///
    explicit FileInTransferReader(const QDir &basePath,
                                  const FileInfo &fileInfo);

    ///
    /// \brief Frees the memory used by the instance and closes the file.
    ///
    ~FileInTransferReader() = default;

    ///
    /// \brief Reads the next chunk of data from the file.
    /// \param buffer the byte array where the data will be stored.
    /// \return a value indicating whether the data has been correctly read or
    /// not.
    ///
    bool processNextDataChunk(QByteArray &buffer) override;

    ///
    /// \brief Commits the file transfer.
    /// \return true if all the file has been read correctly and false
    /// otherwise.
    ///
    bool commit() override;

    ///
    /// \brief Rollbacks the file transfer.
    /// \return true if the operation succeeded and false otherwise (already
    /// committed).
    ///
    bool rollback() override;

private:
    ///
    /// \brief Checks if the file has been updated with respect to the cached
    /// data.
    /// \return true if some data changed and false otherwise.
    ///
    bool updated();

private:
    QFile m_file; ///< \brief The instance representing the file.
};


///
/// \brief The FileInTransferWriter class represents the file currently received
/// from the peer by the SYFFT protocol and written to the disk.
///
/// This class, which extends FileInTransfer, provides the implementation of the
/// methods necessary to guarantee that a file received is stored on disk only
/// if the whole transfer succeeds and is committed: this is obtained through
/// the use of QSaveFile, a class which allows writing to a temporary file and
/// moving it to its final destination only during the commit phase.
///
class FileInTransferWriter : public FileInTransfer
{
public:
    ///
    /// \brief Builds a new instance from the parameters.
    /// \param basePath the path the file is relative to.
    /// \param fileInfo the information about the file to be transferred.
    ///
    explicit FileInTransferWriter(const QDir &basePath,
                                  const FileInfo &fileInfo);

    ///
    /// \brief Frees the memory used by the instance and closes the file.
    ///
    ~FileInTransferWriter() = default;

    ///
    /// \brief Writes the next chunk of data to the file.
    /// \param buffer the data to be written.
    /// \return a value indicating whether the data has been correctly written
    /// or not.
    ///
    bool processNextDataChunk(QByteArray &buffer) override;

    ///
    /// \brief Commits the file transfer.
    /// \return true if all the file has been written correctly and false
    /// otherwise.
    ///
    bool commit() override;

    ///
    /// \brief Rollbacks the file transfer.
    /// \return true if the operation succeeded and false otherwise (already
    /// committed).
    ///
    bool rollback() override;

private:
    QSaveFile m_file; ///< \brief The instance representing the file.
};

#endif // FILEINTRANSFER_HPP

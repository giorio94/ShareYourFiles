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

#ifndef FILEINFO_HPP
#define FILEINFO_HPP

#include <QDateTime>

class QDataStream;

///
/// \brief The FileInfo class represents a file to be shared through Share Your
/// Files.
///
/// Every file that is scheduled to be transferred through the SYFFT protocol is
/// characterized by three main elements: its relative path with respect to a
/// common base directory, its size and its last modification time-stamp.
/// The FileInfo class provides a wrapper around these fields, along with the
/// stream operators necessary to read it from and write it to a QDataStream,
/// and therefore to exchange it through the network. Additionally, the class
/// contains also a status field, which represents the current status of the
/// file with respect to the transfer.
///
class FileInfo
{
public:
    ///
    /// \brief The Status enum represents the possible status of a file.
    ///
    enum class Status {
        Scheduled,        ///< \brief The file is scheduled for transfer.
        InTransfer,       ///< \brief The file is in transfer.
        Transferred,      ///< \brief The file has already been transferred.
        TransferRejected, ///< \brief The file has been rejected.
        TransferFailed,   ///< \brief The file transfer failed.
    };

    ///
    /// \brief Generates an invalid instance.
    ///
    explicit FileInfo() : m_valid(false) {}

    ///
    /// \brief Builds a new instance from the parameters.
    /// \param filePath the relative path of the file.
    /// \param size the size of the file (in bytes).
    /// \param lastModified the date and time when the file was last modified.
    ///
    explicit FileInfo(const QString &filePath, quint64 size,
                      const QDateTime &lastModified);

    ///
    /// \brief Generates an instance which is an exact copy of the parameter.
    /// \param other the instance to be copied.
    ///
    FileInfo(const FileInfo &other) = default;

    ///
    /// \brief Copies the data from the parameter to the current instance.
    /// \param other the instance to be copied.
    ///
    FileInfo &operator=(const FileInfo &other) = default;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    ~FileInfo() = default;

    ///
    /// \brief Returns whether the FileInfo instance is valid or not.
    ///
    /// In case of invalid instance, that is when initialized through the
    /// default constructor, the content of the other fields is undefined.
    ///
    bool valid() const { return m_valid; }

    /// \brief Returns the relative path of the file (including the file name).
    QString filePath() const { return m_filePath; }
    /// \brief Returns the name of the file.
    QString name() const { return m_name; }
    /// \brief Returns the relative path of the file (excluding the file name).
    QString path() const { return m_path; }

    /// \brief Returns the size of the file (in bytes).
    quint64 size() const { return m_size; }
    /// \brief Returns the date and time when the file was last modified.
    QDateTime lastModified() const { return m_lastModified; }

    /// \brief Returns the current status of the file.
    Status status() const { return m_status; }

    ///
    /// \brief Updates the status of the file.
    /// \param status the new status to be stored.
    ///
    void setStatus(Status status) { m_status = status; }

    ///
    /// \brief Writes a FileInfo to a QDataStream.
    /// \param stream the stream where data is written to.
    /// \param fileInfo the FileInfo instance to be written.
    /// \return the stream given as parameter to allow concatenations.
    ///
    friend QDataStream &operator<<(QDataStream &stream,
                                   const FileInfo &fileInfo);
    ///
    /// \brief Reads a FileInfo from a QDataStream.
    /// \param stream the stream from where data is read.
    /// \param fileInfo the FileInfo instance to be filled.
    /// \return the stream given as parameter to allow concatenations.
    ///
    friend QDataStream &operator>>(QDataStream &stream, FileInfo &fileInfo);

private:
    bool m_valid; ///< \brief Specifies whether the instance is valid or not.

    QString m_filePath; ///< \brief The relative path (including the file name).
    QString m_name;     ///< \brief The name of the file.
    QString m_path;     ///< \brief The relative path (excluding the file name).
    quint64 m_size;     ///< \brief The size of the file (in bytes).
    /// \brief The date and time when the file was last modified.
    QDateTime m_lastModified;

    Status m_status; ///< \brief The current status of the file.
};

#endif // FILEINFO_HPP

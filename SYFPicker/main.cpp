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

#include <QApplication>

#include <QDataStream>
#include <QLocalSocket>
#include <QMessageBox>
#include <QString>
#include <QStringList>

// The name used by the server to listen for connections
static const QString SERVER_NAME = QString("SYFPickerProtocol");
// The maximum time allowed for the operations (in milliseconds)
static const int TIMEOUT = 5000;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Get the list of arguments (the list of file names)
    QStringList arguments = QCoreApplication::arguments();
    if (arguments.count() <= 1) {
        // No file or directory names specified, just return
        return 0;
    }

    // Try to establish the connection to ShareYourFiles
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME, QLocalSocket::WriteOnly);

    if (!socket.waitForConnected(TIMEOUT)) {
        // Connection failed
        QString title(QObject::tr(TARGET));
        QString message(QObject::tr("Impossible to establish the connection to"
                                    " ShareYourFiles.\nCheck if the application"
                                    " is correctly running and retry later."));
        QMessageBox::critical(Q_NULLPTR, title, message);
        return -1;
    }

    // Build the stream to send the data
    QDataStream stream(&socket);
    stream.setVersion(QDataStream::Version::Qt_5_0);
    stream.setByteOrder(QDataStream::ByteOrder::LittleEndian);

    // Send the number of strings
    stream << static_cast<quint32>(arguments.count() - 1);

    // Send each path, converted in UTF8 format
    for (int i = 1; i < arguments.count(); i++) {
        QByteArray path = arguments.at(i).toUtf8();
        stream << path;
    }

    if (!socket.waitForBytesWritten(TIMEOUT)) {
        // Failed flushing the data
        QString title(QObject::tr(TARGET));
        QString message(
            QObject::tr("Failed sending the data to ShareYourFiles.\n"
                        "Check if the application is correctly running"
                        " and retry later."));
        QMessageBox::critical(Q_NULLPTR, title, message);
        return -1;
    }

    // Disconnect from the server
    socket.disconnectFromServer();

    return 0;
}

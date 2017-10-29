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

#include "Common/common.hpp"
#include "Common/networkentrieslist.hpp"
#include "FileTransfer/syfftprotocolreceiver.hpp"
#include "FileTransfer/syfftprotocolsender.hpp"
#include "FileTransfer/syfpprotocol.hpp"
#include "FileTransfer/transferlist.hpp"
#include "UserDiscovery/user.hpp"
#include "UserDiscovery/users.hpp"
#include "shareyourfiles.hpp"

#include "Gui/Wrappers/peersselectormodel.hpp"
#include "Gui/Wrappers/settingsmodel.hpp"
#include "Gui/Wrappers/transfersmodel.hpp"

#include <ConsoleAppender.h>
#include <Logger.h>
#include <RollingFileAppender.h>

#include <QApplication>
#include <QFontDatabase>
#include <QPointer>
#include <QStandardPaths>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>

// Function declarations
static void initLogger(const QString &logPath);
static void initSystemTrayIcon();
static void initializeModeAction(QMenu *systemTrayMenu);
static void initializeInterfaceSubmenu(QMenu *systemTrayMenu);
static QActionGroup *initializeInterfaceActionGroup(QMenu *subMenu);
static void initializeSettingsAction(QMenu *systemTrayMenu);
static void initializeTransfersAction(QMenu *systemTrayMenu);
static void initializeQuitAction(QMenu *systemTrayMenu);
static void initializeAboutActions(QMenu *systemTrayMenu);
static void initializeSystemTrayMessages();

static void peersSelector(const QStringList &paths);
static void startTransfer(const QString &uuid, const TransferList &transferList,
                          const QString &message);
static void setConnectionMessages(SyfftProtocolCommon *instance, bool sender);


/// \brief The name of the application
static const QString APP_NAME = QObject::tr("Share Your Files");

/// \brief The index of the settings window inside the mainEngine object.
static const int IDX_SETTINGS = 0;
/// \brief The index of the transfers window inside the mainEngine object.
static const int IDX_TRANSFERS = 1;
/// \brief The index of the about window inside the mainEngine object.
static const int IDX_ABOUT = 2;


/// \brief The object representing the system tray icon.
static QPointer<QSystemTrayIcon> systemTrayIcon;
/// \brief The object representing the system tray menu.
static QPointer<QMenu> systemTrayMenu;
/// \brief The object including all the network entries actions.
static QPointer<QActionGroup> entriesActionGroup;

/// \brief The QML engine used for the settings and transfers windows.
static QPointer<QQmlApplicationEngine> mainEngine;
/// \brief The model containing all the active transfers.
static QPointer<TransfersModel> transfersModel;

/// \brief Associates the operational mode to the corresponding string.
static QMap<Enums::OperationalMode, QString> modeStringMap;
/// \brief Associates the operational mode to the corresponding icon.
static QMap<Enums::OperationalMode, QIcon> modeIconMap;
/// \brief The icon displayed when no network entry is available.
static QIcon errorIcon;

///
/// \brief The entry point of Share Your Files.
///
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(":/Resources/IconGreen.svg"));

    // Build the base configuration and data paths
    QString confPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
        "/ShareYourFiles";

    // Initialize the logger
    initLogger(dataPath);

    // Create the ShareYourFiles instance
    if (!ShareYourFiles::createInstance(confPath, dataPath)) {
        LOG_ERROR() << "Share Your Files: initialization failed";
        QString message(QObject::tr("Impossible to start the application.\n") +
                        ShareYourFiles::instance()->errorMessage());
        QMessageBox::critical(Q_NULLPTR, APP_NAME, message);
        ShareYourFiles::destroyInstance();
        return -1;
    }

    // Connect the slot to destroy the data structures before the termination
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {

        qApp->closeAllWindows();
        if (systemTrayMenu) {
            systemTrayMenu->deleteLater();
        }
        if (systemTrayIcon) {
            systemTrayIcon->deleteLater();
        }

        ShareYourFiles::destroyInstance();
    });

    // Initialize the QML main engine
    QQuickStyle::setStyle("Material");
    mainEngine = new QQmlApplicationEngine(ShareYourFiles::instance());

    // Create the settings window
    SettingsModel *model =
        new SettingsModel(ShareYourFiles::instance()->localUser(),
                          ShareYourFiles::instance()->peersList(), mainEngine);
    mainEngine->rootContext()->setContextProperty("settings", model);
    mainEngine->load(QUrl(QStringLiteral("qrc:/Qml/Settings.qml")));

    // Create the transfers window
    transfersModel =
        new TransfersModel(ShareYourFiles::instance()->localUser(),
                           ShareYourFiles::instance()->peersList(), mainEngine);
    mainEngine->rootContext()->setContextProperty("transfers", transfersModel);
    mainEngine->load(QUrl(QStringLiteral("qrc:/Qml/Transfers.qml")));

    // Create the about window
    mainEngine->rootContext()->setContextProperty("version", QString(VERSION));
    mainEngine->load(QUrl(QStringLiteral("qrc:/Qml/About.qml")));


    // Initialize the System Tray Icon
    initSystemTrayIcon();

    // Connect the signal to start a new transfer when requested
    QObject::connect(ShareYourFiles::instance()->syfpProtocolInstance(),
                     &SyfpProtocolServer::pathsReceived, mainEngine,
                     peersSelector);


    // Connect the signal to start a new reception when requested
    QObject::connect(ShareYourFiles::instance()->localUser(),
                     &LocalUser::connectionRequested, mainEngine,
                     [](QSharedPointer<SyfftProtocolReceiver> receiver) {

                         transfersModel->addSyfftInstance(receiver);
                         setConnectionMessages(receiver.data(), false);

                         // Show the transfers window
                         mainEngine->rootObjects()
                             .at(IDX_TRANSFERS)
                             ->setProperty("visible", true);
                     });

    // Enter the event loop
    return app.exec();
}

///
/// \brief Shows the QML window to choose the recipients of the transfer.
/// \param paths the base paths used to build a TransferList instance.
///
static void peersSelector(const QStringList &paths)
{
    // Build a new transfer list from the list of paths
    TransferList transferList(paths);
    if (transferList.totalFiles() == 0) {
        return;
    }

    // Open the selector window
    QQmlApplicationEngine *engine = new QQmlApplicationEngine(mainEngine);
    PeersSelectorModel *model = new PeersSelectorModel(
        transferList.totalFiles(), sizeToHRFormat(transferList.totalBytes()),
        ShareYourFiles::instance()->peersList(), engine);

    engine->rootContext()->setContextProperty("peersSelector", model);
    engine->load(QUrl(QStringLiteral("qrc:/Qml/PeersSelector.qml")));

    // Connect the signal executed when the decision is taken
    QObject::connect(model, &PeersSelectorModel::selectionCompleted,
                     [engine, model, transferList](bool confirmed) {

                         // If the transfer has been confirmed, open a
                         // connection for each
                         // selected item
                         if (confirmed) {
                             QString message = model->message().trimmed();
                             foreach (const QString &uuid,
                                      model->selectedItems()) {
                                 startTransfer(uuid, transferList, message);
                             }

                             // Show the transfers window
                             mainEngine->rootObjects()
                                 .at(IDX_TRANSFERS)
                                 ->setProperty("visible", true);
                         }

                         engine->deleteLater();
                     });
}

///
/// \brief Starts a transfer to a given peer.
/// \param uuid the identifier of the recipient of the transfer.
/// \param transferList the object representing the files to be shared.
/// \param message the optional message to be attached to the request.
///
static void startTransfer(const QString &uuid, const TransferList &transferList,
                          const QString &message)
{
    // Get a new sender instance
    QSharedPointer<SyfftProtocolSender> sender =
        ShareYourFiles::instance()->peersList()->newSyfftInstance(
            uuid, ShareYourFiles::instance()->localUser()->mode() ==
                      Enums::OperationalMode::Offline);
    if (sender.isNull()) {
        return;
    }

    // Add the sender instance to the list
    transfersModel->addSyfftInstance(sender);
    setConnectionMessages(sender.data(), true);

    // Start the connection
    sender->sendFiles(transferList, message);
}

///
/// \brief Initializes CuteLogger.
/// \param logPath the path where the log files are stored.
///
static void initLogger(const QString &logPath)
{
#ifdef QT_DEBUG
    const char *format =
        "[%{TypeOne}] %{time}{yyyy-MM-dd HH:mm:ss.zzz} "
        "<%{threadid} - %{Function}@%{file}:%{line}> %{message}\n";
#else
    const char *format =
        "[%{TypeOne}] %{time}{yyyy-MM-dd HH:mm:ss.zzz} - %{message}\n";
#endif

    ConsoleAppender *consoleAppender = new ConsoleAppender;
    consoleAppender->setFormat(format);
    cuteLogger->registerAppender(consoleAppender);

    // Try to make the directory where the log files are stored
    if (QDir(logPath).mkpath(".")) {
        FileAppender *fileAppender =
            new FileAppender(logPath + "/ShareYourFiles.log");
        fileAppender->setFormat(format);
        cuteLogger->registerAppender(fileAppender);
    } else {
        LOG_ERROR() << "ShareYourFiles: impossible to create the log path"
                    << logPath;
    }

    LOG_INFO() << "Logger initialization completed";
}

///
/// \brief Initializes the system tray icon and the associated menu.
///
static void initSystemTrayIcon()
{
    // Initialize the operational mode maps
    modeStringMap[Enums::OperationalMode::Online] = QObject::tr("online");
    modeStringMap[Enums::OperationalMode::Offline] = QObject::tr("offline");
    modeIconMap[Enums::OperationalMode::Online] =
        QIcon(":/Resources/IconGreen.svg");
    modeIconMap[Enums::OperationalMode::Offline] =
        QIcon(":/Resources/IconYellow.svg");
    errorIcon = QIcon(":/Resources/IconRed.svg");

    // System tray menu
    systemTrayMenu = new QMenu();

    initializeModeAction(systemTrayMenu);
    initializeInterfaceSubmenu(systemTrayMenu);

    systemTrayMenu->addSeparator();
    initializeSettingsAction(systemTrayMenu);
    initializeTransfersAction(systemTrayMenu);

    systemTrayMenu->addSeparator();
    initializeAboutActions(systemTrayMenu);

    systemTrayMenu->addSeparator();
    initializeQuitAction(systemTrayMenu);

    // System tray icon
    systemTrayIcon = new QSystemTrayIcon();
    systemTrayIcon->setIcon(
        modeIconMap.value(ShareYourFiles::instance()->localUser()->mode()));
    systemTrayIcon->setContextMenu(systemTrayMenu);
    systemTrayIcon->show();

    // Show the welcome message
    systemTrayIcon->showMessage(
        APP_NAME, APP_NAME + QObject::tr(" is now running in background."));

    initializeSystemTrayMessages();
}

///
/// \brief Initializes the "Go offline - online" action of the system tray menu.
/// \param systemTrayMenu the menu to which the action is attached.
///
static void initializeModeAction(QMenu *systemTrayMenu)
{
    // Get the text to be shown depending on the current status
    LocalUser *localUser = ShareYourFiles::instance()->localUser();
    Enums::OperationalMode other =
        localUser->mode() == Enums::OperationalMode::Online
            ? Enums::OperationalMode::Offline
            : Enums::OperationalMode::Online;
    QString modeActiontext = QObject::tr("&Go ") + modeStringMap.value(other);

    QAction *modeAction = new QAction(modeActiontext, systemTrayMenu);

    // Connect the handler to react to the action click
    QObject::connect(modeAction, &QAction::triggered, localUser, [localUser]() {

        // In case some receptions are ongoing, ask whether terminate them
        // or abort the mode change to offline
        int receptions = transfersModel->ongoingReceptions();
        if (localUser->mode() == Enums::OperationalMode::Online &&
            receptions > 0) {

            QString message =
                QString::number(receptions) +
                ((receptions == 1) ? QObject::tr(" reception is")
                                   : QObject::tr(" receptions are")) +
                QObject::tr(
                    " still ongoing.\nWould you like to go offline anyway?");
            if (QMessageBox::warning(Q_NULLPTR, APP_NAME, message,
                                     QMessageBox::StandardButton::Yes |
                                         QMessageBox::StandardButton::No) !=
                QMessageBox::StandardButton::Yes) {
                return;
            }
        }

        if (!localUser->setMode(localUser->mode() ==
                                        Enums::OperationalMode::Online
                                    ? Enums::OperationalMode::Offline
                                    : Enums::OperationalMode::Online)) {
            QMessageBox::critical(
                Q_NULLPTR, APP_NAME,
                QObject::tr("Failed changing the operational mode.\nVerify the "
                            "network configuration."));
        }
    });

    // Connect the handler to update the text and the icon when the mode changes
    QObject::connect(
        localUser, &LocalUser::modeChanged, modeAction,
        [modeAction](Enums::OperationalMode mode) {
            Enums::OperationalMode other =
                mode == Enums::OperationalMode::Online
                    ? Enums::OperationalMode::Offline
                    : Enums::OperationalMode::Online;
            modeAction->setText(QObject::tr("&Go ") +
                                modeStringMap.value(other));
            systemTrayIcon->setIcon(
                ShareYourFiles::instance()->networkEntriesList()->empty()
                    ? errorIcon
                    : modeIconMap.value(mode));
        });

    // Connect the slot to disable the action if no valid network entry exists
    QObject::connect(
        ShareYourFiles::instance()->networkEntriesList(),
        &NetworkEntriesList::networkInterfacesChanged, modeAction,
        [modeAction]() {
            bool empty =
                ShareYourFiles::instance()->networkEntriesList()->empty();

            modeAction->setEnabled(!empty);
            systemTrayIcon->setIcon(
                empty ? errorIcon
                      : modeIconMap.value(
                            ShareYourFiles::instance()->localUser()->mode()));
        });

    systemTrayMenu->addAction(modeAction);
}

///
/// \brief Initializes the "Select interface" system tray sub-menu.
/// \param systemTrayMenu the menu to which the sub-menu is attached.
///
static void initializeInterfaceSubmenu(QMenu *systemTrayMenu)
{
    // Create the sub-menu and add it to the main menu.
    QMenu *subMenu =
        new QMenu(QObject::tr("Select &Interface"), systemTrayMenu);
    systemTrayMenu->addMenu(subMenu);

    // Create the new actions and add them to the menu
    entriesActionGroup = initializeInterfaceActionGroup(subMenu);
    subMenu->addActions(entriesActionGroup->actions());

    // Connect the handler to update the menu when the interfaces change
    QObject::connect(
        ShareYourFiles::instance()->networkEntriesList(),
        &NetworkEntriesList::networkInterfacesChanged, subMenu, [subMenu]() {
            // Delete previous actions if any
            if (!entriesActionGroup.isNull()) {
                subMenu->clear();
                entriesActionGroup->deleteLater();
            }

            // Create the new actions and add them to the menu
            entriesActionGroup = initializeInterfaceActionGroup(subMenu);
            subMenu->addActions(entriesActionGroup->actions());

            // Disable the menu if no entry is displayed
            subMenu->setEnabled(!entriesActionGroup->actions().isEmpty());
        });
}

///
/// \brief Initializes the action group grouping the available interfaces.
/// \param subMenu the menu set as parent of the action group.
///
static QActionGroup *initializeInterfaceActionGroup(QMenu *subMenu)
{
    // Create the action group which includes the entries
    QActionGroup *entriesGroup = new QActionGroup(subMenu);

    // Get the information about the interfaces
    NetworkEntriesList *entryList =
        ShareYourFiles::instance()->networkEntriesList();
    const NetworkEntriesList::Entry &current =
        ShareYourFiles::instance()->currentNetworkEntry();

    // Build all the actions
    foreach (const NetworkEntriesList::Entry &entry, entryList->entries()) {
        QAction *action =
            new QAction(NetworkEntriesList::toString(entry), entriesGroup);
        action->setData(QVariant::fromValue(entry));
        action->setCheckable(true);
        action->setChecked(entry == current);

        // Connect the slot to update the selected action when a change occurs
        QObject::connect(
            ShareYourFiles::instance(), &ShareYourFiles::networkEntryChanged,
            action, [action](const NetworkEntriesList::Entry &entry) {
                action->setChecked(
                    action->data().value<NetworkEntriesList::Entry>() == entry);
            });
    }

    // Connect the slot executed when a new interface is chosen
    QObject::connect(
        entriesGroup, &QActionGroup::triggered, ShareYourFiles::instance(),
        [](QAction *selectedAction) {
            // Reset the previously selected action
            emit ShareYourFiles::instance()->networkEntryChanged(
                ShareYourFiles::instance()->currentNetworkEntry());

            int transfers = transfersModel->ongoingTransfers();

            // If some transfers are still ongoing, ask before changing
            if (transfers > 0) {
                QString message =
                    QString::number(transfers) +
                    ((transfers == 1) ? QObject::tr(" transfer is")
                                      : QObject::tr(" transfers are")) +
                    QObject::tr(" still ongoing.\nWould you like to change the "
                                "interface anyway?");
                if (QMessageBox::warning(Q_NULLPTR, APP_NAME, message,
                                         QMessageBox::StandardButton::Yes |
                                             QMessageBox::StandardButton::No) !=
                    QMessageBox::StandardButton::Yes) {
                    return;
                }
            }

            if (!ShareYourFiles::instance()->changeNetworkEntry(
                    selectedAction->data()
                        .value<NetworkEntriesList::Entry>())) {

                QMessageBox::critical(
                    Q_NULLPTR, APP_NAME,
                    QObject::tr("Failed changing the network interface."));
            }
        });

    return entriesGroup;
}

///
/// \brief Initializes the "Settings" action of the system tray menu.
/// \param systemTrayMenu the menu to which the action is attached.
///
static void initializeSettingsAction(QMenu *systemTrayMenu)
{
    QAction *settingsAction =
        new QAction(QObject::tr("&Settings"), systemTrayMenu);
    QObject::connect(settingsAction, &QAction::triggered, mainEngine, []() {
        mainEngine->rootObjects()
            .at(IDX_SETTINGS)
            ->setProperty("visible", true);
    });
    systemTrayMenu->addAction(settingsAction);
}

///
/// \brief Initializes the "Transfers" action of the system tray menu.
/// \param systemTrayMenu the menu to which the action is attached.
///
static void initializeTransfersAction(QMenu *systemTrayMenu)
{
    QAction *transfersAction =
        new QAction(QObject::tr("&Transfers"), systemTrayMenu);
    QObject::connect(transfersAction, &QAction::triggered, mainEngine, []() {
        mainEngine->rootObjects()
            .at(IDX_TRANSFERS)
            ->setProperty("visible", true);
    });
    systemTrayMenu->addAction(transfersAction);
}

///
/// \brief Initializes the "About" actions of the system tray menu.
/// \param systemTrayMenu the menu to which the action is attached.
///
static void initializeAboutActions(QMenu *systemTrayMenu)
{
    // About action
    QAction *aboutAction = new QAction(QObject::tr("&About"), systemTrayMenu);
    QObject::connect(aboutAction, &QAction::triggered, mainEngine, []() {
        mainEngine->rootObjects().at(IDX_ABOUT)->setProperty("visible", true);
    });
    systemTrayMenu->addAction(aboutAction);

    // AboutQt action
    QAction *aboutQtAction =
        new QAction(QObject::tr("About Q&t"), systemTrayMenu);
    QObject::connect(aboutQtAction, &QAction::triggered, mainEngine,
                     []() { QMessageBox::aboutQt(Q_NULLPTR, APP_NAME); });
    systemTrayMenu->addAction(aboutQtAction);
}

///
/// \brief Initializes the "Quit" actions of the system tray menu.
/// \param systemTrayMenu the menu to which the action is attached.
///
static void initializeQuitAction(QMenu *systemTrayMenu)
{
    QAction *quitAction = new QAction(QObject::tr("&Quit"), systemTrayMenu);
    QObject::connect(quitAction, &QAction::triggered, systemTrayMenu, []() {

        int transfers = transfersModel->ongoingTransfers();
        if (transfers == 0) {
            QCoreApplication::quit();
        }

        // If some transfers are still ongoing, ask before quitting
        else {
            QString message =
                QString::number(transfers) +
                ((transfers == 1) ? QObject::tr(" transfer is")
                                  : QObject::tr(" transfers are")) +
                QObject::tr(" still ongoing.\nWould you like to quit anyway?");
            if (QMessageBox::warning(Q_NULLPTR, APP_NAME, message,
                                     QMessageBox::StandardButton::Yes |
                                         QMessageBox::StandardButton::No) ==
                QMessageBox::StandardButton::Yes) {

                QCoreApplication::quit();
            }
        }
    });
    systemTrayMenu->addAction(quitAction);
}

///
/// \brief Initializes the slots used to display system tray icon messages when
/// something happens (the local user or a peer changes the operational mode).
///
static void initializeSystemTrayMessages()
{
    // Show a message when the operational mode changes
    QObject::connect(ShareYourFiles::instance()->localUser(),
                     &LocalUser::modeChanged, systemTrayIcon,
                     [](Enums::OperationalMode mode) {
                         systemTrayIcon->showMessage(
                             APP_NAME, QObject::tr("You are now ") +
                                           modeStringMap.value(mode) + ".");
                     });

    // Show a message when a new peer is detected
    QObject::connect(
        ShareYourFiles::instance()->peersList(), &PeersList::peerAdded,
        systemTrayIcon, [](const QString &uuid) {
            UserInfo info =
                ShareYourFiles::instance()->peersList()->activePeer(uuid);

            if (info.valid()) {
                systemTrayIcon->showMessage(
                    APP_NAME, info.names() + QObject::tr(" is now online."));
            }
        });

    // Show a message when a peer quits
    QObject::connect(
        ShareYourFiles::instance()->peersList(), &PeersList::peerExpired,
        systemTrayIcon, [](const QString &uuid) {
            UserInfo info = ShareYourFiles::instance()->peersList()->peer(uuid);

            if (info.valid()) {
                systemTrayIcon->showMessage(
                    APP_NAME, info.names() + QObject::tr(" is now offline."));
            }
        });

    // Show a message when a duplicated name is detected
    QObject::connect(
        ShareYourFiles::instance()->peersList(),
        &PeersList::duplicatedNameDetected, systemTrayIcon, []() {
            systemTrayIcon->showMessage(
                APP_NAME,
                QObject::tr("Another user with your name has been detected.\n"
                            "Change it to make you more recognisable."),
                QSystemTrayIcon::MessageIcon::Warning);
        });
}

///
/// \brief Initializes the slots used to display stystem tray icon messages when
/// a transfer is connected or aborted.
/// \param instance the object representing the transfer.
/// \param sender whether the local user is the sender or the receiver.
///
static void setConnectionMessages(SyfftProtocolCommon *instance, bool sender)
{
    // Show a message when a transfer is completed
    QObject::connect(
        instance, &SyfftProtocolCommon::transferCompleted, systemTrayIcon,
        [instance, sender]() {
            UserInfo info = ShareYourFiles::instance()->peersList()->peer(
                instance->peerUuid());
            if (info.valid()) {
                systemTrayIcon->showMessage(
                    APP_NAME, (sender ? QObject::tr("Transfer to ")
                                      : QObject::tr("Reception from ")) +
                                  info.names() + QObject::tr(" completed."));
            }
        });

    // Show a message when a transfer is aborted
    QObject::connect(
        instance, &SyfftProtocolCommon::aborted, systemTrayIcon,
        [instance, sender]() {
            UserInfo info = ShareYourFiles::instance()->peersList()->peer(
                instance->peerUuid());
            if (info.valid()) {
                systemTrayIcon->showMessage(
                    APP_NAME, (sender ? QObject::tr("Transfer to ")
                                      : QObject::tr("Reception from ")) +
                                  info.names() + QObject::tr(" aborted."));
            }
        });
}

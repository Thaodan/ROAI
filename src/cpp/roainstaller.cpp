/**
 * \copyright   Copyright © 2012 QuantumBytes inc.
 *
 *              For more information, see https://www.quantum-bytes.com/
 *
 * \section LICENSE
 *
 *              This file is part of Relics of Annorath Installer.
 *
 *              Relics of Annorath Installer is free software: you can redistribute it and/or modify
 *              it under the terms of the GNU General Public License as published by
 *              the Free Software Foundation, either version 3 of the License, or
 *              any later version.
 *
 *              Relics of Annorath Installer is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *              GNU General Public License for more details.
 *
 *              You should have received a copy of the GNU General Public License
 *              along with Relics of Annorath Installer.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \brief       Handels the update process for the Installer and related files
 *
 * \file    	roainstaller.cpp
 *
 * \note
 *
 * \version 	1.0
 *
 * \author  	Manuel Gysin <manuel.gysin@quantum-bytes.com>
 *
 * \date        2012/12/01 23:10:00 GMT+1
 *
 */

/******************************************************************************/
/*                                                                            */
/*    Others includes                                                         */
/*                                                                            */
/******************************************************************************/
#include "../h/roainstaller.h"

/******************************************************************************/
/*                                                                            */
/*    Constructor/Deconstructor                                               */
/*                                                                            */
/******************************************************************************/
ROAInstaller::ROAInstaller(QObject *parent) :
    QObject(parent)
{
    // Set download phase for later
    downloadPhase = 0;

    // Create settings object with old name
    userSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Quantum Bytes GmbH", "Relics of Annorath");
    //userSettings->beginGroup("Relics of Annorath");

    // Check path and remove old installation or outdated libs
    installPathOld = userSettings->value("Relics of Annorath/installLocation","none").toString();

    // Cleanup old files from old releases
    if(installPathOld != "none")
    {
        // Check for ending slash if not add it
        if(!installPathOld.endsWith("/"))
        {
            installPathOld += "/";
        }

        // Clean files
        cleanupObsoletFiles();

        // Remove old dirs
        removeDirWithContent(installPathOld + "lib");
        removeDirWithContent(installPathOld + "imageformats");
        removeDirWithContent(installPathOld + "sounds");
        removeDirWithContent(installPathOld + "downloads");

        // Rename old dirs
        QDir dir;
        dir.rename( installPathOld + "files/", installPathOld + "game/");

        // Overwrite old setting
        userSettings->setValue("Relics of Annorath/installLocation", "none");

        // Set real installationpath
        installationPath = installPathOld;
    }

    // Delete settings and create new ones in new context
    delete userSettings;

    userSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), "Relics of Annorath Launcher"); // This must the in the launcher namespace

    // Check for installation path
    if(installPathOld == "none")
    {
        installationPath = userSettings->value("installLocation", "none").toString();
    }

    if(installationPath == "none")
    {
        blockMode = true;
    }
    else
    {
        // Check for ending slash if not add it
        if(!installationPath.endsWith("/"))
            installationPath += "/";

        // Set this path to settings
        userSettings->setValue("installLocation", installationPath);

        blockMode = false;
    }
}

ROAInstaller::~ROAInstaller()
{
    delete userSettings;
}

/******************************************************************************/
/*                                                                            */
/*    Public methods                                                          */
/*                                                                            */
/******************************************************************************/

void ROAInstaller::install()
{
    // Set mode to default
    installationMode = "default";

    // Show initial page
    mainWidget = new RoaMainWidget(installationPath);
    mainWidget->show();

    connect(mainWidget,SIGNAL(readyToInstall()),this,SLOT(startInstallation()));

#ifdef Q_OS_LINUX
    // Play sound
    //sound = new QSound( ":/sounds/CallForHeroes.wav" );
    //sound->setLoops(-1);
    //sound->play();
#endif

}

void ROAInstaller::update()
{
    if(!blockMode)
    {
        // Set installation mode
        installationMode = "update";

        // Show initial page
        mainWidget = new RoaMainWidget(installationPath);
        mainWidget->setCustomContentId(4);
        mainWidget->show();

        // Check for needed dirs and create them if needed
        checkDirectories();

        // Get file list for verification, after this point everything is handled with slots
        getRemoteFileList();
    }
    else
    {
        QMessageBox::warning(NULL,tr("Update failed"), tr("Please reinstall/repair the Relics of Annorath client!"));
    }
}

void ROAInstaller::verify()
{
    if(!blockMode)
    {
        // Set installation mode
        installationMode = "verify";

        // Check for needed dirs and create them if needed
        checkDirectories();

        // Get file list for verification, after this point everything is handled with slots
        getRemoteFileList();
    }
    else
    {
        QMessageBox::warning(NULL,tr("Verify failed"), tr("Please reinstall/repair the Relics of Annorath client!"));
    }
}

void ROAInstaller::repair()
{
    /* When we call this something bad happens with the installation
     * First we try to recover the installationpath if it is broken
     * Secondly we remove all game content
     * Third we verify all left files for consistence
     */

    installationMode = "repair";

    // Installpath is fine
    if(!blockMode)
    {
        // Remove game content
        if(removeDirWithContent(installationPath + "game"))
        {
            // Verify
            checkDirectories();
            getRemoteFileList();
        }
        else
        {
            QMessageBox::warning(NULL,tr("Repair failed"), tr("Please remove the \"game\" folder under ") + installationPath + tr("!"));
        }
    }
    else
    {
        /* We try to discover the installation path, normaly the launcher should do this on each run
         * For this we get the path of binary
         */
        QString expectedPath =  QApplication::applicationDirPath();
        installationPath = "";

        // Get path parts
        /// \todo Check if this works for windows too!
        QStringList pathParts = expectedPath.split("/");
        expectedPath = "";

        /* The installer binary is place in <roa dir>/launcher/bin/roainstaller
         * We get the way back to the <roa dir> this means we substracting 2 levels
         * But first we verifing the top two level folders
         */

        if(pathParts.at(pathParts.size() - 1) == "bin" && pathParts.at(pathParts.size() -2) == "bin")
        {
            for(int i = 0; i < pathParts.size() - 2; i++)
            {
                expectedPath = pathParts.at(i) + "/";
            }
        }
        else
        {
            // We are not able to recover, fallback to user selection of the installation directory
            installationPath = QFileDialog::getExistingDirectory(NULL, tr("Select your Relics of Annorath installation"), QApplication::applicationDirPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        }

        if(installationPath != "")
        {
            // Set installation path
            userSettings->value("installLocation", installationPath);

            // Remove game content
            if(removeDirWithContent(installationPath + "game"))
            {
                // Verify
                checkDirectories();
                getRemoteFileList();
            }
            else
            {
                QMessageBox::warning(NULL,tr("Repair failed"), tr("Please remove the \"game\" folder under ") + installationPath + tr("!"));
            }
        }
        else
        {
            QMessageBox::warning(NULL,tr("Repair failed"), tr("Please select a valid instalallation directory or reinstall the application!"));
        }
    }
}

void ROAInstaller::uninstall()
{
    if(!blockMode)
    {
        // Check for user confirmation
        if(QMessageBox::Cancel == QMessageBox::warning(NULL,tr("Uninstallation"), tr("All files in the directory ") + installationPath + tr(" are going to be deleted!"), QMessageBox::Ok, QMessageBox::Cancel))
        {
            QApplication::quit();
        }
        else
        {
            if(removeDirWithContent(installationPath ))
            {
                QMessageBox::information(NULL,tr("Client uninstalled successfully"), tr("Client uninstalled successfully!"));
            }
            else
            {
                QMessageBox::warning(NULL,tr("Client uninstalled failed"), tr("Client uninstalled failed! Please remove left files per hand!"));
            }
        }
    }
    else
    {
        QMessageBox::warning(NULL,tr("Uninstallation failed"), tr("Please reinstall/repair the Relics of Annorath client!"));
    }
}

/******************************************************************************/
/*                                                                            */
/*    Private methods                                                         */
/*                                                                            */
/******************************************************************************/

bool ROAInstaller::removeDirWithContent(QString _dir)
{
    // Thanks to John for the code part -> http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
    bool result = true;

    QDir dir(_dir);

    if (dir.exists(_dir))
    {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            if (info.isDir())
            {
                result = removeDirWithContent(info.absoluteFilePath());
            }
            else
            {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }

        result = dir.rmdir(_dir);
    }

    return result;
}

void ROAInstaller::cleanupObsoletFiles()
{
#ifdef Q_OS_LINUX
    QFile::remove(installPathOld + "ROALauncher");
    QFile::remove(installPathOld + "ROALauncher.sh");
#endif

#ifdef Q_OS_WIN32
#ifdef Q_OS_WIN64
    QFile::remove(installPathOld + "annorath.ico");
    QFile::remove(installPathOld + "install.ico");
    QFile::remove(installPathOld + "libeay32.dll");
    QFile::remove(installPathOld + "phonon4.dll");
    QFile::remove(installPathOld + "QtCore4.dll");
    QFile::remove(installPathOld + "QtGui4.dll");
    QFile::remove(installPathOld + "QtNetwork4.dll");
    QFile::remove(installPathOld + "QtWebKit4.dll");
    QFile::remove(installPathOld + "roa_en.qm");
    QFile::remove(installPathOld + "roa_ger.qm");
    QFile::remove(installPathOld + "ROALauncher_x64.exe");
    QFile::remove(installPathOld + "ssleay32.dll");
    QFile::remove(installPathOld + "torrent.dll");
    QFile::remove(installPathOld + "unins000.dat");
    QFile::remove(installPathOld + "unins000.exe");
    QFile::remove(installPathOld + "uninstall.ico");
    QFile::remove(installPathOld + "url.ico");
#else
#endif
#endif
}

void ROAInstaller::checkDirectories()
{
    QStringList dirs;
    dirs << installationPath + "game"
         << installationPath + "game/bin"
         << installationPath + "game/data"
         << installationPath + "game/lib"
         << installationPath + "launcher"
         << installationPath + "launcher/platforms"
        #ifdef Q_OS_LINUX
         << installationPath + "launcher/bin"
         << installationPath + "launcher/lib"
         << installationPath + "launcher/bin/platforms"
         << installationPath + "launcher/bin/imageformats"
        #endif
         << installationPath + "launcher/downloads"
         << installationPath + "launcher/imageformats"
         << installationPath + "launcher/sounds";

    for(int i = 0; i < dirs.size(); i++)
    {
        QDir dir(dirs.at(i));
        if(!dir.exists())
            QDir().mkpath(dirs.at(i));
    }
}

void ROAInstaller::getRemoteFileList()
{
    // Prepare downloading over ssl
    certificates.append(QSslCertificate::fromPath(":/certs/class2.pem"));
    certificates.append(QSslCertificate::fromPath(":/certs/ca.pem"));

    sslConfig.defaultConfiguration();
    sslConfig.setCaCertificates(certificates);

    request.setSslConfiguration(sslConfig);

    connect(&manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slot_downloadFinished(QNetworkReply*)));
    connect(&manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),this, SLOT(slot_getSSLError(QNetworkReply*, const QList<QSslError>&)));

#ifdef Q_OS_LINUX
#ifdef __x86_64__
    request.setUrl(QUrl("https://launcher.annorath-game.com/data/linux_x86_64/launcher/linux_x86_64.txt"));
#else
    request.setUrl(QUrl("https://launcher.annorath-game.com/data/linux_x86/launcher/linux_x86.txt"));
#endif
#endif

#ifdef Q_OS_WIN32
#ifdef Q_OS_WIN64
    request.setUrl(QUrl("https://launcher.annorath-game.com/data/windows_x86_64/launcher/windows_x86_64.txt"));
#else
    request.setUrl(QUrl("https://launcher.annorath-game.com/data/windows_x86/launcher/windows_x86.txt"));
#endif
#endif

    // Start download
    manager.get(request);
}

void ROAInstaller::startInstallation()
{
    // Set installation path
    installationPath = mainWidget->getInstallPath();

#ifdef Q_OS_WIN32
    installationPath = QDir::fromNativeSeparators(installationPath);
#endif

    // Check for ending slash
    if(!installationPath.endsWith("/"))
    {
        installationPath += "/";
    }

    // Set this path to settings
    userSettings->setValue("installLocation", installationPath);

    // Get components
    componentsSelected = mainWidget->getSelectedComponents();

    // Check for needed dirs and create them if needed
    checkDirectories();

    // Get file list for verification
    getRemoteFileList();
}

void ROAInstaller::prepareDownload()
{
    // Open file and read it
    QFile file(installationPath + "launcher/downloads/files.txt");

    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);

        while ( !in.atEnd() )
        {
            QStringList tmp = QString(in.readLine()).split(";");

            // Check if we got valid input
            if(tmp.size() == 2)
            {
                // Check for correct files, do not download not needed data
                if(!checkFileWithHash(tmp.at(0), tmp.at(1)))
                {
                    // Save values
                    fileList.append(tmp.at(0));
                    fileListMD5.append(tmp.at(1));
                }
            }
        }
    }

    // Close file
    file.close();

    // Calculate remaing files
    filesLeft = fileList.size();

    // Get the next file
    getNextFile();
}

bool ROAInstaller::checkFileWithHash(QString _file, QString _hash)
{
    QFile file(installationPath + _file);

    if(file.open(QIODevice::ReadOnly))
    {
        // Get bytes
        QByteArray fileData = file.readAll();

        // Get hash
        QByteArray hash = QCryptographicHash::hash(fileData, QCryptographicHash::Sha256);

        // Compare
        if(QString(hash.toHex()) == _hash)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void ROAInstaller::getNextFile()
{
    if(filesLeft > 0)
    {
        // Set URL and start download
#ifdef Q_OS_LINUX
#ifdef __x86_64__
        request.setUrl(QUrl("https://launcher.annorath-game.com/data/linux_x86_64/" + fileList.at(filesLeft-1)));
#else
        request.setUrl(QUrl("https://launcher.annorath-game.com/data/linux_x86/" + fileList.at(filesLeft-1)));
#endif
#endif

#ifdef Q_OS_WIN32
#ifdef Q_OS_WIN64
        request.setUrl(QUrl("https://launcher.annorath-game.com/data/windows_x86_64/" + fileList.at(filesLeft-1)));
#else
        request.setUrl(QUrl("https://launcher.annorath-game.com/data/windows_x86/" + fileList.at(filesLeft-1)));
#endif
#endif
        manager.get(request);


        // Check mode
        if(installationMode == "default" || installationMode == "update")
        {
            // Update status
            mainWidget->setNewStatus(100/(fileList.size())*(fileList.size()-filesLeft));
            mainWidget->setNewLabelText(tr("Currently downloading: ") + fileList.at(filesLeft-1));
        }

        filesLeft -= 1;
    }
    else
    {
        if(installationMode == "default")
        {
            // Set status to 100
            mainWidget->setNewStatus(100);
            mainWidget->setNewLabelText("Installing additional software...");

            // Install components and creat shortcuts
            installOptionalComponents();

#ifdef Q_OS_LINUX
            // Set last page
            mainWidget->setCustomContentId(5);
#endif
        }
        else if(installationMode == "update")
        {
            /// \todo relaunch the launcher
#ifdef Q_OS_LINUX
            createLinuxShortcut(QDir::homePath() + "/.local/share/applications/Relics of Annorath.desktop");
            createLinuxShortcut(QDir::homePath() + "/Desktop/Relics of Annorath.desktop");
#endif

#ifdef Q_OS_WIN32
            createWindowsShortcuts(QString(qgetenv("APPDATA")) + "/Microsoft/Windows/Start Menu/Programs/Relics of Annorath.lnk");
            createWindowsShortcuts(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0) + "/Relics of Annorath.lnk");
#endif
            mainWidget->setCustomContentId(5);
        }
        else if(installationMode == "verify")
        {
            QMessageBox::information(NULL,tr("Client verified successfully"), tr("Client verified successfully!"));
        }
        else if(installationMode == "repair")
        {
            QMessageBox::information(NULL,tr("Client repaired successfully"), tr("Client repaired successfully!"));
        }
        else
        {
            // Display infobox
            QMessageBox::information(NULL,tr("Unknown installation mode"), tr("Something goes wrong, if you get this message please report it and add an step by step description!"));
        }
    }
}

void ROAInstaller::installOptionalComponents()
{
#ifdef Q_OS_LINUX

    if(componentsSelected.at(3).toInt())
    {
        createLinuxShortcut(QDir::homePath() + "/.local/share/applications/Relics of Annorath.desktop");
    }

    if(componentsSelected.at(4).toInt())
    {
        createLinuxShortcut(QDir::homePath() + "/Desktop/Relics of Annorath.desktop");
    }
#endif

#ifdef Q_OS_WIN32

    if(componentsSelected.at(1).toInt())
    {
        processPaths.append(installationPath + "launcher/downloads/vcredist_x64.exe");
        processArgs.append(" /q");
    }

    if(componentsSelected.at(2).toInt())
    {
        processPaths.append(installationPath + "launcher/downloads/oalinst.exe");
        processArgs.append(" /silent");
    }

    thread = new WindowsProcess();
    startProcess();

#endif
}

#ifdef Q_OS_WIN
void ROAInstaller::startProcess()
{
    if(processPaths.size() > 0)
    {
        // Feed thread
        thread->setProcessEnv(processPaths.at(0), processArgs.at(0));

        // Remove it
        processPaths.removeAt(0);
        processArgs.removeAt(0);

        connect(thread, SIGNAL(finished()),SLOT(slot_processDone()));

        thread->start();
    }
    else
    {
        slot_processDone();
    }
}
#endif

#ifdef Q_OS_LINUX
void ROAInstaller::createLinuxShortcut(QString _path)
{
    // Create menu entry
    QFile menuEntry(_path);
    menuEntry.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream out(&menuEntry);

    out << "[Desktop Entry]\n";
    out << "Encoding=UTF-8\n";
    out << "Version=1.0\n";
    out << "Type=Application\n";
    out << "Terminal=false\n";
    out << "Exec=\"" + installationPath + "launcher/bin/ROALauncher.sh" + "\"\n";
    out << "Name=Relics of Annorath\n";
    out << "Icon=" + installationPath + "launcher/roa.ico" + "\n";

    menuEntry.setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOwner | QFile::ReadUser);

    menuEntry.close();
}

#endif

#ifdef Q_OS_WIN32
void ROAInstaller::createWindowsShortcuts(QString _path)
{
    // Fastes and easiest way to do this for windows system - windows api sucks...
    QFile::link(installationPath + "launcher/ROALauncher.exe", _path );
}
#endif

/******************************************************************************/
/*                                                                            */
/*    Slots                                                                   */
/*                                                                            */
/******************************************************************************/

void ROAInstaller::slot_downloadFinished(QNetworkReply *reply)
{
    QString fileName;

    switch(downloadPhase)
    {
        case 0:
            fileName = "launcher/downloads/files.txt";
            break;
        case 1:
            fileName = fileList.at(filesLeft);
            break;
    }

    // Open the file to write to
    QFile file(installationPath + fileName);
    file.open(QIODevice::WriteOnly);

    // Open a stream to write into the file
    QDataStream stream(&file);

    // Get the size of the torrent
    int size = reply->size();

    // Get the data of the torrent
    QByteArray temp = reply->readAll();

    // Write the file
    stream.writeRawData(temp, size);

    // Set exe permissions
    file.setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOwner | QFile::ReadUser);

    // Close the file
    file.close();

    // If phase 0 take future steps
    switch(downloadPhase)
    {
        case 0:
            prepareDownload();
            downloadPhase = 1;
            break;
        case 1:
            getNextFile();
            break;
    }
}

void ROAInstaller::slot_getSSLError(QNetworkReply* reply, const QList<QSslError> &errors)
{
    QSslError sslError = errors.first();

    if(sslError.error() == 11 )
    {
        if(installationMode == "default")
        {
            //reply->ignoreSslErrors();
            mainWidget->setNewLabelText(reply->errorString());
        }
        else
        {
            /// \todo Add info box
        }
    }
}

#ifdef Q_OS_WIN32
void ROAInstaller::slot_processDone()
{
    if(processPaths.size() > 0)
    {
        startProcess();
    }
    else
    {
        if(componentsSelected.at(3).toInt())
        {
            createWindowsShortcuts(QString(qgetenv("APPDATA")) + "/Microsoft/Windows/Start Menu/Programs/Relics of Annorath.lnk");
        }
        if(componentsSelected.at(4).toInt())
        {
            createWindowsShortcuts(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0) + "/Relics of Annorath.lnk");
        }

        // Set last page
        mainWidget->setCustomContentId(5);
    }
}
#endif

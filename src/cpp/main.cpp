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
 * \file    	main.cpp
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
/*    Qt includes                                                             */
/*                                                                            */
/******************************************************************************/
//#include <QtPlugin>
#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>

/******************************************************************************/
/*                                                                            */
/*    Others includes                                                         */
/*                                                                            */
/******************************************************************************/
#include "../h/roainstaller.h"

/**
 * \brief The main loop-
 *
 * The main class of the appliaction.
 *
 * \param[in] argc Count of arguments.
 * \param[in] *argv Array with the arguments.
 *
 * \return Returns QAppliaction state.
 *
*/
int main(int argc, char *argv[])
{
    //Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);

    QApplication a(argc, argv);
    
    // Set appliaction properties
    a.setApplicationName("Relics of Annorath Installer");
    a.setApplicationDisplayName("Relics of Annorath Installer"); // This seems to have no affect yet, wait for qt5 realese version

    a.setApplicationVersion("001.000.000");

    a.setOrganizationName("QuantumBytes inc.");
    a.setOrganizationDomain("quantum-bytes.com");

    // Add custom font
    QFontDatabase::addApplicationFont(":/font/ModernAntiqua.ttf");

    // Start the installer
    ROAInstaller installer;

    /* Decide if we need to do fully installation, updat or deinstallation
     *
     * Default (no argument): Full client installation
     * Arg: update: Update the client installation
     * Arg: verify: Verify the client installation
     * Arg: repair: Repair all client files, remove game content
     * Arg: uninstall: Remove client and game content
     *
     */

    QString text(QObject::tr("Valid arguments:\n"
                    "   Default (no argument) - Full client installation"
                    "   update - Update the client installation\n"
                    "   verify - Verify the client installation\n"
                    "   repair - Try to repair a broken installation\n"
                    "   uninstall - Remove client and game content- WARNING IF THE DIRECOTRY CONTAINS OTHER FILES THEN FROM ROA, THESE ARE ALSO DELETE!\n"
                    "   \n"
                    "Sample: roainstaller update"));

    if(argc == 1)
    {
        installer.install();

        return a.exec();
    }
    else if(argc == 2)
    {
        QString action = argv[1];

        /// \todo Keep the /slient for compatiblity
        if(action == "update" || action == "/silent")
        {
            installer.update();
            return a.exec();
        }
        else if(action == "verify")
        {
            installer.verify();
            return a.exec();
        }
        else if(action == "repair")
        {
            installer.repair();
            return a.exec();
        }
        else if(action == "uninstall")
        {
            installer.uninstall();
            a.quit();
        }
        else
        {
            QMessageBox::information(NULL,QObject::tr("Invalid argument"), QObject::tr("Wrong argument found!\n\n") + text);
        }
    }
    else
    {
        QMessageBox::information(NULL,QObject::tr("Invalid argument amount"), QObject::tr("To much arguments!\n\n") + text);
    }
}

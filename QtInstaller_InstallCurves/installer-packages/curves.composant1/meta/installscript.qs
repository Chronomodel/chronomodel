/**************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
**
** $QT_END_LICENSE$
**
**************************************************************************/



    function Component()
    {
    	installer.setDefaultPageVisible(QInstaller.Introduction, true);
    	installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, true);
        installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
        
      /*  installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
        installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
        */
    }

    Component.prototype.createOperations = function()
    {
    
        component.createOperations();
    }
/*
    Component.prototype.installationFinishedPageIsShown = function()
    {
        try {
 
            if (installer.isInstaller() && installer.status == QInstaller.Success) {
                installer.addWizardPageItem( component, "ReadMeCheckBoxForm", QInstaller.InstallationFinished );
            }
        } catch(e) {
            console.log(e);
        }
    }


    Component.prototype.installationFinished = function()
    {
        try {
            if (installer.isInstaller() && installer.status == QInstaller.Success) {
                var isReadMeCheckBoxChecked = component.userInterface( "ReadMeCheckBoxForm" ).readMeCheckBox.checked;
                if (isReadMeCheckBoxChecked) {
                    QDesktopServices.openUrl("file:///" + installer.value("TargetDir") + "/README.txt");
                }
            }
        } catch(e) {
            console.log(e);
        }
    }

*/
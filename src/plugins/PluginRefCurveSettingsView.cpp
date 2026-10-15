/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "PluginRefCurveSettingsView.h"

#include <QApplication>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>


PluginRefCurveSettingsView::PluginRefCurveSettingsView(PluginAbstract* plugin, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mPlugin(plugin)
{
    mRefCurvesLab = new QLabel(tr("Available Reference Curves") + " :", this);
    mRefCurvesLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mRefCurvesLab->setToolTip(PluginAbstract::AppPluginLibrary());

    mRefCurvesList = new QListWidget(this);
    mRefCurvesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mRefCurvesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mRefCurvesList->setAlternatingRowColors(true);

    mAddRefCurveBut = new QPushButton(tr("Add"), this);
    mDeleteRefCurveBut = new QPushButton(tr("Delete"), this);

    connect(mAddRefCurveBut, &QPushButton::clicked, this, &PluginRefCurveSettingsView::addRefCurve);
    connect(mDeleteRefCurveBut, &QPushButton::clicked, this, &PluginRefCurveSettingsView::deleteRefCurve);
    connect(mRefCurvesList, &QListWidget::itemSelectionChanged, this, &PluginRefCurveSettingsView::updateSelection);

    QGridLayout* layout = new QGridLayout();
    layout->setContentsMargins(0, 10, 10, 10);
    layout->addWidget(mRefCurvesLab, 0, 0, 1, 2);
    layout->addWidget(mRefCurvesList, 1, 0, 1, 2);
    layout->addWidget(mAddRefCurveBut, 2, 0);
    layout->addWidget(mDeleteRefCurveBut, 2, 1);

    setLayout(layout);

    // Store the list of existing files
    QString calibPath = mPlugin->getRefsPath();
    QDir calibDir(calibPath);
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for (int i=0; i<files.size(); ++i) {
        if(files[i].suffix().toLower() == mPlugin->getRefExt())
            mFilesOrg.insert(files[i].fileName(), files[i].absoluteFilePath());
    }
    mFilesNew = mFilesOrg;
    updateRefsList();
    updateSelection();
}

PluginRefCurveSettingsView::~PluginRefCurveSettingsView()
{
}

void PluginRefCurveSettingsView::updateRefsList()
{
    mRefCurvesLab->setToolTip(mPlugin->getRefsPath());

    mRefCurvesList->clear();
    QMapIterator<QString, QString> iter(mFilesNew);
    while (iter.hasNext()) {
        iter.next();
        QListWidgetItem* item = new QListWidgetItem(iter.key());
        const RefCurve &curve = mPlugin->mRefCurves[iter.key().toLower()];
        if (curve.mDataMean.isEmpty())
            item->setForeground(Qt::red);

        mRefCurvesList->addItem(item);
    }
}

void PluginRefCurveSettingsView::updateFilesInFolder()
{
    QString calibPath = mPlugin->getRefsPath();
    QDir cPath (calibPath);
    // Creation of the plugin file directory, if it does not exist
    if (!cPath.exists()) {
        cPath.mkpath(calibPath);
    }
    if (!cPath.exists()) {
        QMessageBox::warning(qApp->activeWindow(), tr("Error"), tr("Impossible to create the plugin path %1").arg(calibPath)) ;
        return;
    }
   // Delete removed curves
    QMapIterator<QString, QString> iter(mFilesOrg);
    iter = QMapIterator<QString, QString>(mFilesOrg);
    while (iter.hasNext()){
        iter.next();
        if (!mFilesNew.contains(iter.key())) {
            if (QMessageBox::question(qApp->activeWindow(), tr("Warning"),
                                      tr("You are about to delete a reference curve : %1. All data using this curve (in all your projects) will be invalid until you specify another curve for each one. Do you really want to delete this curve?").arg(iter.key())) == QMessageBox::Yes) {
               QString filepath = calibPath + "/" + iter.key();
               if (QFile::remove(filepath))
                   mFilesOrg.remove(iter.key());

            }
        }
    }

    iter = QMapIterator<QString, QString>(mFilesNew);
    while (iter.hasNext()) {
        iter.next();
        // The file name already existed, but we have a new path : replace it !
        if (mFilesOrg.contains(iter.key()) && iter.value() != mFilesOrg.value(iter.key())) {
            if (QMessageBox::question(qApp->activeWindow(), tr("Warning"), tr("Do you really want to replace existing %1").arg(iter.key())) == QMessageBox::Yes) {
                const QString filepath = calibPath + "/" + iter.key();
                mFilesOrg[iter.key()] = filepath;
            }
        }
        // The file does not exist : copy it.
        else if (!mFilesOrg.contains(iter.key())) {
            const QString filepath = calibPath + "/" + iter.key();
            if (QFile::copy(iter.value(), filepath))
                mFilesOrg.insert(iter.key(), iter.value());

        }
    }

    mPlugin->loadRefDatas();
}

void PluginRefCurveSettingsView::addRefCurve(){
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(),
                                                tr("Open File"),
                                                "",
                                                tr("Reference curve (*.%1)").arg(mPlugin->getRefExt() ));


    QFileInfo fileInfo(path);
    if (!path.isEmpty()) {
        if (fileInfo.fileName().contains(';') || fileInfo.fileName().contains(',')) {
            QMessageBox message(QMessageBox::Critical,
                                qApp->applicationName() + " " + qApp->applicationVersion(),
                                QObject::tr("File names must not contain any \",\" or \";\" "),
                                QMessageBox::Ok,
                                qApp->activeWindow());
            message.exec();

        } else {
            mFilesNew.insert(fileInfo.fileName(), fileInfo.absoluteFilePath());
            updateFilesInFolder();
            updateRefsList();

            emit listRefCurveChanged();
        }
    }
}

void PluginRefCurveSettingsView::deleteRefCurve()
{
    // copy of the selected file, because mRefCurvesList is cleared with updateRefsList()
    QList<QString> selectedFile;

    for (auto item : mRefCurvesList->selectedItems())
        selectedFile.append(item->text());

    for (int i=0; i<selectedFile.size(); ++i) {
        const QString filename = selectedFile.at(i);
        mFilesNew.remove(filename);
        updateFilesInFolder();
        updateRefsList(); //Clear and update mRefCurvesList

        emit listRefCurveChanged();
    }
}

/**
 * No more used, it was to open a ref file in a texteditor, or an other programm as Excel
 */
/*void PluginRefCurveSettingsView::openSelectedFile(){
    QList<QListWidgetItem*> selectedItems = mRefCurvesList->selectedItems();
    if(selectedItems.size() > 0){
        QString filename = selectedItems[0]->text();
        QString calibPath = mPlugin->getRefsPath();
        QDesktopServices::openUrl(QUrl("file:///" + calibPath + "/" + filename, QUrl::TolerantMode));
    }
}*/

void PluginRefCurveSettingsView::updateSelection()
{
    QList<QListWidgetItem*> selectedItems = mRefCurvesList->selectedItems();
    mDeleteRefCurveBut->setEnabled(selectedItems.size() > 0);
}

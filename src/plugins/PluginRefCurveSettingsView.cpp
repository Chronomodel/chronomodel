/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "PluginRefCurveSettingsView.h"
#include "ColorPicker.h"

#include <QApplication>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>


PluginRefCurveSettingsView::PluginRefCurveSettingsView(PluginAbstract* plugin, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mPlugin(plugin)
{
    mRefCurvesLab = new QLabel(tr("Available reference curves") + " :" , this);
    mRefCurvesLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mRefCurvesList = new QListWidget(this);
    mRefCurvesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mRefCurvesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mRefCurvesList->setAlternatingRowColors(true);

    mAddRefCurveBut = new QPushButton(tr("Add"), this);
    mDeleteRefCurveBut = new QPushButton(tr("Delete"), this);
    mOpenBut = new QPushButton(tr("Open"), this);
    mOpenBut->setVisible(false);

    connect(mAddRefCurveBut, &QPushButton::clicked, this, &PluginRefCurveSettingsView::addRefCurve);
    connect(mDeleteRefCurveBut, &QPushButton::clicked, this, &PluginRefCurveSettingsView::deleteRefCurve);
  //  connect(mOpenBut, &QPushButton::clicked, this, &PluginRefCurveSettingsView::openSelectedFile);
    connect(mRefCurvesList, &QListWidget::itemSelectionChanged, this, &PluginRefCurveSettingsView::updateSelection);

    QGridLayout* layout = new QGridLayout();
    layout->addWidget(mRefCurvesLab, 0, 0, 1, 2);
    layout->addWidget(mRefCurvesList, 1, 0, 1, 2);
    layout->addWidget(mAddRefCurveBut, 2, 0);
    layout->addWidget(mDeleteRefCurveBut, 2, 1);
    //layout->addWidget(mOpenBut, 2, 2);
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

PluginRefCurveSettingsView::~PluginRefCurveSettingsView(){

}

void PluginRefCurveSettingsView::updateRefsList()
{
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
                QString filepath = calibPath + "/" + iter.key();

            }
        }
        // The file does not exist : copy it.
        if (!mFilesOrg.contains(iter.key())) {
            QString filepath = calibPath + "/" + iter.key();
            if (QFile::copy(iter.value(), filepath))
                mFilesOrg.insert(iter.key(),iter.value());

        }
    }

    mPlugin->loadRefDatas();
}

void PluginRefCurveSettingsView::addRefCurve(){
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(),
                                                tr("Open File"),
                                                "",
                                                tr("Reference curve (*.%1)").arg(mPlugin->getRefExt() ));

    if (!path.isEmpty()) {
        QFileInfo fileInfo(path);
        mFilesNew.insert(fileInfo.fileName(), fileInfo.absoluteFilePath());
        updateFilesInFolder();
        updateRefsList();

        emit listRefCurveChanged();
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
    mOpenBut->setEnabled(selectedItems.size() > 0);
}

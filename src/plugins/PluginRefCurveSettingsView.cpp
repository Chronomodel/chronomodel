#include "PluginRefCurveSettingsView.h"
#include "PluginAbstract.h"
#include "ColorPicker.h"
#include <QtWidgets>


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
        const RefCurve& curve = mPlugin->mRefCurves[iter.key().toLower()];
        if(curve.mDataMean.isEmpty())
            item->setForeground(Qt::red);
        
        mRefCurvesList->addItem(item);
    }
}

void PluginRefCurveSettingsView::updateFilesInFolder()
{
    QString calibPath = mPlugin->getRefsPath();
   // Delete removed curves
    QMapIterator<QString, QString> iter(mFilesOrg);
    iter = QMapIterator<QString, QString>(mFilesOrg);
    while(iter.hasNext()){
        iter.next();
        if (!mFilesNew.contains(iter.key())) {
            if (QMessageBox::question(qApp->activeWindow(), tr("Warning"), tr("You are about to delete a reference curve : ") + " " + iter.key() + ". All data using this curve (in all your projects) will be invalid until you specify another curve for each one. Do you really want to delete this curve?") == QMessageBox::Yes) {
               QString filepath = calibPath + "/" + iter.key();
                if(QFile::remove(filepath))
                   mFilesOrg.remove(iter.key());
                
            }
        }
    }
    
    iter = QMapIterator<QString, QString>(mFilesNew);
    while(iter.hasNext()){
        iter.next();
        // The file name already existed, but we have a new path : replace it !
        if (mFilesOrg.contains(iter.key()) && iter.value() != mFilesOrg.value(iter.key())) {
            if (QMessageBox::question(qApp->activeWindow(), tr("Warning"), tr("Do you really want to replace existing") + " " + iter.key()) == QMessageBox::Yes) {
                QString filepath = calibPath + "/" + iter.key();
                //if(QFile::remove(filepath) && QFile::copy(iter.value(), filepath)){
                //}
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
                                                tr("Reference curve") + " (*." + mPlugin->getRefExt() + ")");
    
    if(!path.isEmpty())
    {
        QFileInfo fileInfo(path);
        mFilesNew.insert(fileInfo.fileName(), fileInfo.absoluteFilePath());
        updateFilesInFolder();
        updateRefsList();
    }
}

void PluginRefCurveSettingsView::deleteRefCurve(){
    QList<QListWidgetItem*> selectedItems = mRefCurvesList->selectedItems();
    for(int i=0; i<selectedItems.size(); ++i){
        QString filename = selectedItems.at(i)->text();
        mFilesNew.remove(filename);
        updateFilesInFolder();
        updateRefsList();
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

void PluginRefCurveSettingsView::updateSelection(){
    QList<QListWidgetItem*> selectedItems = mRefCurvesList->selectedItems();
    mDeleteRefCurveBut->setEnabled(selectedItems.size() > 0);
    mOpenBut->setEnabled(selectedItems.size() > 0);
}

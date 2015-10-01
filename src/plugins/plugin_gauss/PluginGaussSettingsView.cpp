#include "PluginGaussSettingsView.h"
#if USE_PLUGIN_GAUSS

#include "PluginGauss.h"
#include "ColorPicker.h"
#include <QtWidgets>


PluginGaussSettingsView::PluginGaussSettingsView(PluginGauss* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags){
    // Store the list ofe existing files
    QString calibPath = ((PluginGauss*)mPlugin)->getRefsPath();
    mRefCurvesLab = new QLabel(tr("Available reference curves") + " :" , this);
    mRefCurvesLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mRefCurvesList = new QListWidget(this);
    mRefCurvesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mRefCurvesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mRefCurvesList->setAlternatingRowColors(true);
    mAddRefCurveBut = new QPushButton(tr("Add"), this);
    mDeleteRefCurveBut = new QPushButton(tr("Delete"), this);
    
    connect(mAddRefCurveBut, SIGNAL(clicked()), this, SLOT(addRefCurve()));
    connect(mDeleteRefCurveBut, SIGNAL(clicked()), this, SLOT(deleteRefCurve()));
    
    // Store the list ofe existing files
   // QString calibPath = ((PluginGauss*)mPlugin)->getRefsPath();
    QDir calibDir(calibPath);
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for(int i=0; i<files.size(); ++i){
        if(files[i].suffix().toLower() == "csv"){
            mFilesOrg.insert(files[i].fileName(), files[i].absoluteFilePath());
            mRefCurvesList->addItem(files[i].fileName());
        }
    }
    mFilesNew = mFilesOrg;
    updateRefsList();
    
    QGridLayout* layout = new QGridLayout();
    layout->addWidget(mRefCurvesLab, 0, 0, 1, 2);
    layout->addWidget(mRefCurvesList, 1, 0, 1, 2);
    layout->addWidget(mAddRefCurveBut, 2, 0);
    layout->addWidget(mDeleteRefCurveBut, 2, 1);
    setLayout(layout);
}

PluginGaussSettingsView::~PluginGaussSettingsView(){
    
}

void PluginGaussSettingsView::updateRefsList(){
    mRefCurvesList->clear();
    QMapIterator<QString, QString> iter(mFilesNew);
    while(iter.hasNext()){
        iter.next();
        mRefCurvesList->addItem(iter.key());
    }
}

void PluginGaussSettingsView::onAccepted()
{
    QString calibPath = ((PluginGauss*)mPlugin)->getRefsPath();
   // QMessageBox::information(qApp->activeWindow(), tr("Warning getRefsPath : "),calibPath);
    // Delete removed curves
    QMapIterator<QString, QString> iter(mFilesOrg);
    iter = QMapIterator<QString, QString>(mFilesOrg);
    while(iter.hasNext()){
        iter.next();
        if(!mFilesNew.contains(iter.key())){
            if(QMessageBox::question(qApp->activeWindow(), tr("Warning"), tr("You are about to delete a reference curve : ") + " " + iter.key() + ". All data using this curve (in all your projects) will be invalid until you specify another curve for each one. Do you really want to delete this curve?") == QMessageBox::Yes){
                if(QFile::remove(iter.value())){
                   // qDebug() << "deleted : " << iter.value();
                   // QMessageBox::information(qApp->activeWindow(), tr("Warning delete : "),iter.value());
                }
            }
        }
    }
    
    iter = QMapIterator<QString, QString>(mFilesNew);
    while(iter.hasNext()){
        iter.next();
        // The file name already existed, but we have a new path : replace it !
        if(mFilesOrg.contains(iter.key()) && iter.value() != mFilesOrg.value(iter.key())){
            if(QMessageBox::question(qApp->activeWindow(), tr("Warning"), tr("Do you really want to replace existing") + " " + iter.key()) == QMessageBox::Yes){
                QString filepath = calibPath + "/" + iter.key();
                if(QFile::remove(filepath) && QFile::copy(iter.value(), filepath)){
                  //  qDebug() << "overwritted : " << filepath;
                  //  QMessageBox::information(qApp->activeWindow(), tr("Warning overwritted to : "),filepath);
                }
            }
        }
        // The file does not exist : copy it.
        if(!mFilesOrg.contains(iter.key())){
            QString filepath = calibPath + "/" + iter.key();
            if(QFile::copy(iter.value(), filepath)){
                //qDebug() << "copied : " << filepath;
                //QMessageBox::information(qApp->activeWindow(), tr("Warning copy to : "),filepath);
            }
        }
    }
    
    ((PluginGauss*)mPlugin)->loadRefDatas();
}

void PluginGaussSettingsView::addRefCurve(){
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(),
                                                tr("Open File"),
                                                "",
                                                tr("Custom reference curve (*.csv)"));
    
    if(!path.isEmpty())
    {
        QFileInfo fileInfo(path);
        mFilesNew.insert(fileInfo.fileName(), fileInfo.absoluteFilePath());
        updateRefsList();
    }
}

void PluginGaussSettingsView::deleteRefCurve(){
    QList<QListWidgetItem*> selectedItems = mRefCurvesList->selectedItems();
    for(int i=0; i<selectedItems.size(); ++i){
        QString filename = selectedItems[i]->text();
        mFilesNew.remove(filename);
    }
    updateRefsList();
}
#endif

#include "PluginMagSettingsView.h"
#if USE_PLUGIN_AM

#include "PluginMag.h"
#include <QtWidgets>


PluginMagSettingsView::PluginMagSettingsView(PluginMag* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags)
{
    mRefCurvesLab = new QLabel(tr("Available reference curves") + " :", this);
    mRefCurvesLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mRefCurvesList = new QListWidget(this);
    mRefCurvesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mRefCurvesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mRefCurvesList->setAlternatingRowColors(true);
    mAddRefCurveBut = new QPushButton(tr("Add file"), this);
    //mDeleteRefCurveBut = new QPushButton(tr("Delete"), this);

    QObject::connect(mAddRefCurveBut, &QPushButton::clicked, this, &PluginMagSettingsView::addRefCurve);
    //connect(mDeleteRefCurveBut, SIGNAL(clicked()), this, SLOT(deleteRefCurve()));

    // Store the list of existing files
    QString calibPath = ((PluginMag*)mPlugin)->getRefsPath();
    QDir calibDir(calibPath);
    QFileInfoList files = calibDir.entryInfoList(QStringList(), QDir::Files);
    for(int i=0; i<files.size(); ++i){
        if(files[i].suffix().toLower() == "ref"){
            mFilesOrg.insert(files[i].fileName(), files[i].absoluteFilePath());
            mRefCurvesList->addItem(files[i].fileName());
        }
    }
    mFilesNew = mFilesOrg;
    updateRefsList();

    QGridLayout* layout = new QGridLayout();
    layout->addWidget(mRefCurvesLab, 0, 0, 1, 2);
    layout->addWidget(mRefCurvesList, 1, 0, 1, 2);
    layout->addWidget(mAddRefCurveBut, 2, 0,1,2);
    //layout->addWidget(mDeleteRefCurveBut, 2, 1);
    setLayout(layout);

}

PluginMagSettingsView::~PluginMagSettingsView()
{
    
}

void PluginMagSettingsView::updateRefsList()
{
    mRefCurvesList->clear();
    QMapIterator<QString, QString> iter(mFilesNew);
    while(iter.hasNext()){
        iter.next();
        if(((PluginMag*)mPlugin)->mRefDatas.contains(iter.key().toLower())) {
           mRefCurvesList->addItem(iter.key());
        }
        else mRefCurvesList->addItem( iter.key()+" --> IS NOT A VALID FILE" );

    }
}

//void PluginMagSettingsView::onAccepted()
void PluginMagSettingsView::updateFloder()
{
    QString calibPath = ((PluginMag*)mPlugin)->getRefsPath();

    // Delete removed curves
    QMapIterator<QString, QString> iter(mFilesOrg);
    iter = QMapIterator<QString, QString>(mFilesOrg);
    while(iter.hasNext()){
        iter.next();
        if(!mFilesNew.contains(iter.key())){
            if(QMessageBox::question(qApp->activeWindow(), tr("Warning"), tr("You are about to delete a reference curve : ") + " " + iter.key() + ". All data using this curve (in all your projects) will be invalid until you specify another curve for each one. Do you really want to delete this curve?") == QMessageBox::Yes){
                if(QFile::remove(iter.value())){
                    qDebug() << "deleted : " << iter.value();
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
                    qDebug() << "overwritted : " << filepath;
                }
            }
        }
        // The file does not exist : copy it.
        if(!mFilesOrg.contains(iter.key())){
            QString filepath = calibPath + "/" + iter.key();
            if(QFile::copy(iter.value(), filepath)){
                qDebug() << "copied : " << filepath;
            }
        }
    }

    ((PluginMag*)mPlugin)->loadRefDatas();
}

void PluginMagSettingsView::addRefCurve()
{
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(),
                                                tr("Open File"),
                                                "",
                                                tr("ref. reference curve (*.ref)"));

    if(!path.isEmpty())
    {
        QFileInfo fileInfo(path);
        mFilesNew.insert(fileInfo.fileName(), fileInfo.absoluteFilePath());
        updateRefsList();
    }
    updateFloder();
}

void PluginMagSettingsView::deleteRefCurve()
{
    QList<QListWidgetItem*> selectedItems = mRefCurvesList->selectedItems();
    for(int i=0; i<selectedItems.size(); ++i){
        QString filename = selectedItems[i]->text();
        mFilesNew.remove(filename);
    }
    updateRefsList();
}

#endif

#include "MCMCProgressDialog.h"
#include "MCMCLoopMain.h"
#include "AppSettings.h"
#include <QtWidgets>


MCMCProgressDialog::MCMCProgressDialog(MCMCLoopMain* loop, QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mLoop(loop)
{
    setWindowTitle(tr("MCMC in progress..."));
    
    // -----------
    setFont(AppSettings::font());
    mLabel1 = new QLabel(this);
    mLabel2 = new QLabel(this);
    
    mProgressBar1 = new QProgressBar(this);
    mProgressBar1->setMinimum(0);
    mProgressBar1->setMaximum(0);
    mProgressBar1->setMinimumWidth(100 * AppSettings::widthUnit());
    
    
    // ----------
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    mCancelBut = buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    
    // ----------
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(mLabel1);
    layout->addWidget(mProgressBar1);
    layout->addWidget(buttonBox);
    setLayout(layout);
    
    setMinimumWidth(100 * AppSettings::widthUnit());
    
    // -----------
    
    connect(mCancelBut, &QPushButton::clicked, this, &MCMCProgressDialog::cancelMCMC);    
    
    connect(mLoop, &MCMCLoopMain::finished, this, &MCMCProgressDialog::setFinishedState);
    
    connect(mLoop, &MCMCLoopMain::stepChanged, this, &MCMCProgressDialog::setTitle1);
    connect(mLoop, &MCMCLoopMain::stepProgressed, this, &MCMCProgressDialog::setProgress1);
}

MCMCProgressDialog::~MCMCProgressDialog()
{

}

int MCMCProgressDialog::startMCMC()
{
    mLoop->start(QThread::HighPriority);// NormalPriority);
    return exec();
}

void MCMCProgressDialog::cancelMCMC()
{
    mLoop->requestInterruption();
}

void MCMCProgressDialog::setTitle1(const QString& message, int minProgress, int maxProgress)
{
    mLabel1->setText(message);
    mProgressBar1->setMinimum(minProgress);
    mProgressBar1->setMaximum(maxProgress);
}

void MCMCProgressDialog::setProgress1(int value)
{
    mProgressBar1->setValue(value);
}

void MCMCProgressDialog::setTitle2(const QString& message, int minProgress, int maxProgress)
{
    mLabel2->setText(message);
    mProgressBar2->setMinimum(minProgress);
    mProgressBar2->setMaximum(maxProgress);
}

void MCMCProgressDialog::setProgress2(int value)
{
    mProgressBar2->setValue(value);
}

void MCMCProgressDialog::setFinishedState()
{
    mCancelBut->setEnabled(false);
    accept();
}

void MCMCProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        e->ignore();
    } else {
        QDialog::keyPressEvent(e);
    }
}

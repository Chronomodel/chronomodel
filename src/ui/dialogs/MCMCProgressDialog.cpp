#include "MCMCProgressDialog.h"
#include "MCMCLoopMain.h"
#include <QtWidgets>


MCMCProgressDialog::MCMCProgressDialog(MCMCLoopMain* loop, QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mLoop(loop)
{
    setWindowTitle(tr("MCMC in progress..."));
    
    // -----------
    
    mLabel1 = new QLabel();
    mLabel2 = new QLabel();
    
    mProgressBar1 = new QProgressBar();
    mProgressBar1->setMinimum(0);
    mProgressBar1->setMaximum(0);
    
    mProgressBar2 = new QProgressBar();
    mProgressBar2->setMinimum(0);
    mProgressBar2->setMaximum(0);
    
    // ----------
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    mCancelBut = buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    
    // ----------
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mLabel1);
    layout->addWidget(mProgressBar1);
    layout->addWidget(buttonBox);
    setLayout(layout);
    
    setMinimumWidth(700);
    
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
    if(e->key() == Qt::Key_Escape){
        e->ignore();
    }else{
        QDialog::keyPressEvent(e);
    }
}

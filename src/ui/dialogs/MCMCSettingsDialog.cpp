#include "MCMCSettingsDialog.h"
#include "Collapsible.h"
#include "Label.h"
#include "Button.h"
#include "LineEdit.h"
#include <QtWidgets>


MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags),
mWidth(600),
mMargin(5),
mLineH(20),
mButW(80),
mButH(25)
{
    setWindowTitle(tr("MCMC Options"));
    
    mAdvancedWidget = new QWidget();
    
    mNumProcLab = new Label(tr("Chains") + " :", this);
    mNumIterLab = new Label(tr("Run Iterations / Chains") + " :", mAdvancedWidget);
    mNumBurnLab = new Label(tr("Burn Iterations") + " :", mAdvancedWidget);
    mMaxBatchesLab = new Label(tr("Max Adapt Batches") + " :", mAdvancedWidget);
    mIterPerBatchLab = new Label(tr("Iterations / Batch") + " :", mAdvancedWidget);
    mDownSamplingLab = new Label(tr("Iterations to keep / Chains") + " :", mAdvancedWidget);
    
    mNumProcEdit = new LineEdit(this);
    mNumIterEdit = new LineEdit(mAdvancedWidget);
    mNumBurnEdit = new LineEdit(mAdvancedWidget);
    mMaxBatchesEdit = new LineEdit(mAdvancedWidget);
    mIterPerBatchEdit = new LineEdit(mAdvancedWidget);
    mDownSamplingEdit = new LineEdit(mAdvancedWidget);
    
    QIntValidator* positiveValidator = new QIntValidator();
    
    positiveValidator->setBottom(1);
    mNumProcEdit->setValidator(positiveValidator);
    mNumIterEdit->setValidator(positiveValidator);
    mNumBurnEdit->setValidator(positiveValidator);
    mMaxBatchesEdit->setValidator(positiveValidator);
    mIterPerBatchEdit->setValidator(positiveValidator);
    mDownSamplingEdit->setValidator(positiveValidator);

    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));

    mAdvanced = new Collapsible(tr("Advanced"), this);
    mAdvanced->setWidget(mAdvancedWidget, 6*mMargin + 5*mLineH);
    connect(mAdvanced, SIGNAL(collapsing(int)), this, SLOT(adaptSize()));
    
    adaptSize();
}

MCMCSettingsDialog::~MCMCSettingsDialog()
{

}

void MCMCSettingsDialog::setSettings(const MCMCSettings& settings)
{
    mNumProcEdit->setText(QString::number(settings.mNumProcesses));
    mNumIterEdit->setText(QString::number(settings.mNumRunIter));
    mNumBurnEdit->setText(QString::number(settings.mNumBurnIter));
    mMaxBatchesEdit->setText(QString::number(settings.mMaxBatches));
    mIterPerBatchEdit->setText(QString::number(settings.mIterPerBatch));
    mDownSamplingEdit->setText(QString::number(settings.mDownSamplingFactor));
}

MCMCSettings MCMCSettingsDialog::getSettings()
{
    MCMCSettings settings;
    settings.mNumProcesses = mNumProcEdit->text().toLongLong();
    settings.mNumRunIter = mNumIterEdit->text().toLongLong();
    settings.mNumBurnIter = mNumBurnEdit->text().toLongLong();
    settings.mMaxBatches = mMaxBatchesEdit->text().toLongLong();
    settings.mIterPerBatch = mIterPerBatchEdit->text().toLongLong();
    settings.mDownSamplingFactor = mDownSamplingEdit->text().toLong();
    return settings;
}

void MCMCSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    p.fillRect(rect(), QColor(220, 220, 220));
}

void MCMCSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void MCMCSettingsDialog::updateLayout()
{
    int m = mMargin;
    int w = width();
    int h = height();
    int w1 = 180;
    int w2 = w - 3*m - w1;
    
    mNumProcLab->setGeometry(m, m, w1, mLineH);
    mNumProcEdit->setGeometry(2*m + w1, m, w2, mLineH);
    
    mOkBut->setGeometry(w - 2*m - 2*mButW, h - m - mButH, mButW, mButH);
    mCancelBut->setGeometry(w - m - mButW, h - m - mButH, mButW, mButH);
    
    w2 = w - 5*m - w1;
    
    int i = 0;
    mNumBurnLab->setGeometry(m, m + i * (m + mLineH), w1, mLineH); ++i;
    mNumIterLab->setGeometry(m, m + i * (m + mLineH), w1, mLineH); ++i;
    mMaxBatchesLab->setGeometry(m, m + i * (m + mLineH), w1, mLineH); ++i;
    mIterPerBatchLab->setGeometry(m, m + i * (m + mLineH), w1, mLineH); ++i;
    mDownSamplingLab->setGeometry(m, m + i * (m + mLineH), w1, mLineH); ++i;
    
    i = 0;
    mNumBurnEdit->setGeometry(2*m + w1, m + i * (m + mLineH), w2, mLineH); ++i;
    mNumIterEdit->setGeometry(2*m + w1, m + i * (m + mLineH), w2, mLineH); ++i;
    mMaxBatchesEdit->setGeometry(2*m + w1, m + i * (m + mLineH), w2, mLineH); ++i;
    mIterPerBatchEdit->setGeometry(2*m + w1, m + i * (m + mLineH), w2, mLineH); ++i;
    mDownSamplingEdit->setGeometry(2*m + w1, m + i * (m + mLineH), w2, mLineH); ++i;
    
    mAdvanced->setGeometry(m, 2*m + mLineH, w - 2*m, mAdvanced->height());
}

void MCMCSettingsDialog::adaptSize()
{
    QSize s(mWidth, 4*mMargin + 1*mLineH + mButH + mAdvanced->height());
    setFixedSize(s);
}

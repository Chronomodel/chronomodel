#include "MCMCSettingsDialog.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "Painting.h"
#include "QtUtilities.h"
#include <QtWidgets>


MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("MCMC Options"));
    
    mSeedsLab = new Label(tr("Seeds") + ": ", this);
    mSeedsEdit = new LineEdit(this);
    
    mNumProcEdit = new LineEdit(this);
    mNumBurnEdit = new LineEdit(this);
    mMaxBatchesEdit = new LineEdit(this);
    mNumIterEdit = new LineEdit(this);
    mDownSamplingEdit = new LineEdit(this);
    
    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(10);
    
    mIterPerBatchSpin = new QSpinBox(this);
    mIterPerBatchSpin->setRange(100, 10000);
    mIterPerBatchSpin->setSingleStep(100);
    
    mNumProcEdit->setValidator(positiveValidator);
    mNumIterEdit->setValidator(positiveValidator);
    mNumBurnEdit->setValidator(positiveValidator);
    mMaxBatchesEdit->setValidator(positiveValidator);
    mDownSamplingEdit->setValidator(positiveValidator);
    
    mNumBurnEdit->setAlignment(Qt::AlignCenter);
    mNumIterEdit->setAlignment(Qt::AlignCenter);
    mNumBurnEdit->setAlignment(Qt::AlignCenter);
    mMaxBatchesEdit->setAlignment(Qt::AlignCenter);
    mDownSamplingEdit->setAlignment(Qt::AlignCenter);

    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    setFixedSize(600, 240);
}

MCMCSettingsDialog::~MCMCSettingsDialog()
{

}

void MCMCSettingsDialog::setSettings(const MCMCSettings& settings)
{
    mNumProcEdit->setText(QString::number(settings.mNumChains));
    mNumIterEdit->setText(QString::number(settings.mNumRunIter));
    mNumBurnEdit->setText(QString::number(settings.mNumBurnIter));
    mMaxBatchesEdit->setText(QString::number(settings.mMaxBatches));
    mIterPerBatchSpin->setValue(settings.mNumBatchIter);
    mDownSamplingEdit->setText(QString::number(settings.mThinningInterval));
    mSeedsEdit->setText(intListToString(settings.mSeeds));
}

MCMCSettings MCMCSettingsDialog::getSettings()
{
    MCMCSettings settings;
    settings.mNumChains = mNumProcEdit->text().toLongLong();
    settings.mNumRunIter = mNumIterEdit->text().toLongLong();
    settings.mNumBurnIter = mNumBurnEdit->text().toLongLong();
    settings.mMaxBatches = mMaxBatchesEdit->text().toLongLong();
    settings.mNumBatchIter = mIterPerBatchSpin->value();
    settings.mThinningInterval = mDownSamplingEdit->text().toLong();
    settings.mSeeds = stringListToIntList(mSeedsEdit->text());
    
    return settings;
}

void MCMCSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    float lineH = 20;
    
    QPainter p(this);
    p.fillRect(rect(), QColor(220, 220, 220));
    
    p.setPen(QColor(50, 50, 50));
    QFont font = p.font();
    font.setWeight(QFont::Bold);
    p.setFont(font);
    
    p.drawText(0, 0, width(), 30, Qt::AlignCenter, tr("MCMC Settings"));
    
    font.setWeight(QFont::Normal);
    font.setPointSizeF(pointSize(11));
    p.setFont(font);
    
    p.drawText(0, 40, width()/2, lineH, Qt::AlignVCenter | Qt::AlignRight, tr("Number of chains") + " :");
    
    p.setBrush(QColor(235, 115, 100));
    p.drawRect(mBurnRect);
    
    p.setBrush(QColor(250, 180, 90));
    p.drawRect(mAdaptRect);
    
    p.setBrush(QColor(130, 205, 110));
    p.drawRect(mAquireRect);
    
    p.setBrush(QColor(255, 255, 255, 100));
    p.drawRect(mBatch1Rect);
    p.drawRect(mBatchInterRect);
    p.drawRect(mBatchNRect);
    
    p.drawText(mBurnRect.adjusted(0, 0, 0, -mBurnRect.height() + lineH), Qt::AlignCenter, tr("1 - BURN"));
    p.drawText(mAdaptRect.adjusted(0, 0, 0, -mAdaptRect.height() + lineH), Qt::AlignCenter, tr("2 - ADAPT"));
    p.drawText(mAquireRect.adjusted(0, 0, 0, -mAquireRect.height() + lineH), Qt::AlignCenter, tr("3 - ACQUIRE"));
    
    p.drawText(mBurnRect.adjusted(0, 1*lineH, 0, -mBurnRect.height() + 2*lineH), Qt::AlignCenter, tr("Iterations") + " :");
    p.drawText(mAquireRect.adjusted(0, 1*lineH, 0, -mAquireRect.height() + 2*lineH), Qt::AlignCenter, tr("Iterations") + " :");
    p.drawText(mAquireRect.adjusted(0, 3*lineH, 0, -mAquireRect.height() + 4*lineH), Qt::AlignCenter, tr("Thinning interval") + " :");
    
    p.drawText(mBatch1Rect.adjusted(0, 0, 0, -mBatch1Rect.height() + lineH), Qt::AlignCenter, tr("BATCH 1"));
    p.drawText(mBatchInterRect, Qt::AlignCenter, "...");
    p.drawText(mBatchNRect, Qt::AlignCenter, tr("BATCH N"));
    
    p.drawText(mBatch1Rect.adjusted(0, 1*lineH, 0, -mBatch1Rect.height() + 2*lineH), Qt::AlignCenter, tr("Iterations") + " :");
    p.drawText(mAdaptRect.x(), mAdaptRect.y() + lineH + mBatch1Rect.height() + m, mAdaptRect.width()/2, lineH, Qt::AlignVCenter | Qt::AlignRight, tr("Max batches") + " :");
}

void MCMCSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void MCMCSettingsDialog::updateLayout()
{
    int m = 5;
    float top = 65.f;
    float lineH = 20;
    float editW = 100;
    float w = width() - 2*m;
    float h = 115;
    int butW = 80;
    int butH = 25;
    
    mBurnRect = QRectF(m, top, w * 0.2, h);
    mAdaptRect = QRectF(m + mBurnRect.width(), top, w * 0.4, h);
    mAquireRect = QRectF(m + mBurnRect.width() + mAdaptRect.width(), top, w * 0.4, h);
    mBatch1Rect = QRectF(mAdaptRect.x() + m,
                         mAdaptRect.y() + lineH,
                         (mAdaptRect.width() - 4*m) / 3,
                         mAdaptRect.height() - 2*lineH - 2*m);
    mBatchInterRect = mBatch1Rect.adjusted(mBatch1Rect.width() + m, 0, mBatch1Rect.width() + m, 0);
    mBatchNRect = mBatch1Rect.adjusted(2*mBatch1Rect.width() + 2*m, 0, 2*mBatch1Rect.width() + 2*m, 0);
    
    mNumProcEdit->setGeometry(width()/2 + m, 40, editW, lineH);
    mNumBurnEdit->setGeometry(mBurnRect.x() + (mBurnRect.width() - editW)/2, mBurnRect.y() + 2*lineH, editW, lineH);
    mNumIterEdit->setGeometry(mAquireRect.x() + (mAquireRect.width() - editW)/2, mAquireRect.y() + 2*lineH, editW, lineH);
    mDownSamplingEdit->setGeometry(mAquireRect.x() + (mAquireRect.width() - editW)/2, mAquireRect.y() + 4*lineH, editW, lineH);
    mIterPerBatchSpin->setGeometry(mBatch1Rect.x() + m, mBatch1Rect.y() + 2*lineH, mBatch1Rect.width() - 2*m, lineH);
    mMaxBatchesEdit->setGeometry(mAdaptRect.x() + mAdaptRect.width()/2 + m, mAdaptRect.y() + mAdaptRect.height() - m - lineH, editW, lineH);
    
    mSeedsLab->setGeometry(width()/2 - m/2 - editW, height() - 2*m - butH - lineH, editW, lineH);
    mSeedsEdit->setGeometry(width()/2 + m/2, height() - 2*m - butH - lineH, editW, lineH);
    
    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

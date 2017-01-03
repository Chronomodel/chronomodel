#include "MCMCSettingsDialog.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include <QtWidgets>


MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags)
{
    setWindowTitle(tr("MCMC Options"));
    QFont font (QApplication::font());
    mSeedsLab = new Label(tr("Seeds (separated by \";\")") + ": ", this);
    mSeedsEdit = new LineEdit(this);
    mSeedsLab->setFont(font);
    mSeedsEdit->setFont(font);

    mHelp = new HelpWidget(tr("About seeds : each MCMC chain is different from the others because it uses a different seed. By default, seeds are picked randomly. However, you can force the chains to use specific seeds by entering them below. By doing so, you can replicate exactly the same results using the same seeds."), this);
    mHelp->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=47"); // chapter 4.2 MCMC settings
    mHelp->setFont(font);

    mNumProcEdit = new LineEdit(this);
    mNumBurnEdit = new LineEdit(this);
    mNumProcEdit->setFont(font);
    mNumBurnEdit->setFont(font);

    mMaxBatchesEdit = new LineEdit(this);
    mNumIterEdit = new LineEdit(this);
    mDownSamplingEdit = new LineEdit(this);
    mMaxBatchesEdit->setFont(font);
    mNumIterEdit->setFont(font);
    mDownSamplingEdit->setFont(font);

    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(1);
    
    QIntValidator* chainsValidator = new QIntValidator(this);
    chainsValidator->setRange(1, 5);
    
    mIterPerBatchSpin = new QSpinBox(this);
    mIterPerBatchSpin->setRange(100, 10000);
    mIterPerBatchSpin->setSingleStep(100);
    mIterPerBatchSpin->setFont(font);
    
    mNumProcEdit->setValidator(chainsValidator);
    mNumProcEdit->setPlaceholderText(tr("From 1 to 5"));
    
    mNumIterEdit->setValidator(positiveValidator);
    mNumBurnEdit->setValidator(positiveValidator);
    mMaxBatchesEdit->setValidator(positiveValidator);
    mDownSamplingEdit->setValidator(positiveValidator);
    
    mNumBurnEdit->setAlignment(Qt::AlignCenter);
    mNumIterEdit->setAlignment(Qt::AlignCenter);
    mNumBurnEdit->setAlignment(Qt::AlignCenter);
    mMaxBatchesEdit->setAlignment(Qt::AlignCenter);
    mDownSamplingEdit->setAlignment(Qt::AlignCenter);
    
    mLabelLevel = new Label(tr("Mixing level"),this);
    mLevelEdit = new LineEdit(this);
    mLabelLevel->setFont(font);
    mLevelEdit->setFont(font);

    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    connect(mOkBut, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mCancelBut, SIGNAL(clicked()), this, SLOT(reject()));
    
    setFixedSize(600, 340);
}

MCMCSettingsDialog::~MCMCSettingsDialog()
{

}

void MCMCSettingsDialog::setSettings(const MCMCSettings& settings)
{
    const QLocale mLoc=QLocale();
    mNumProcEdit->setText(mLoc.toString(settings.mNumChains));
    mNumIterEdit->setText(mLoc.toString(settings.mNumRunIter));
    mNumBurnEdit->setText(mLoc.toString(settings.mNumBurnIter));
    mMaxBatchesEdit->setText(mLoc.toString(settings.mMaxBatches));
    mIterPerBatchSpin->setValue(settings.mNumBatchIter);
    mDownSamplingEdit->setText(mLoc.toString(settings.mThinningInterval));
    mSeedsEdit->setText(intListToString(settings.mSeeds, ";"));
    
    mLevelEdit->setText(mLoc.toString(settings.mMixingLevel));
}

MCMCSettings MCMCSettingsDialog::getSettings()
{
    const QLocale mLoc = QLocale();
    MCMCSettings settings;
    const unsigned int UN = 1;
    settings.mNumChains = qMax(UN, mNumProcEdit->text().toUInt());

    settings.mNumBurnIter = qMax(UN, mNumBurnEdit->text().toUInt());

    settings.mMaxBatches = qMax(UN, mMaxBatchesEdit->text().toUInt());
    settings.mNumBatchIter = qMax(UN, (unsigned int) mIterPerBatchSpin->value());

    settings.mNumRunIter = qMax((unsigned int) 10, mNumIterEdit->text().toUInt());
    settings.mThinningInterval = qBound(UN, mDownSamplingEdit->text().toUInt(), (unsigned int)floor(settings.mNumRunIter/10) );
    
    settings.mMixingLevel = qBound(0.0001, mLoc.toDouble(mLevelEdit->text()),0.9999);
    
    settings.mSeeds = stringListToIntList(mSeedsEdit->text(), ";");
    
    return settings;
}

void MCMCSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    int m = 5;
    double lineH = 20;
    
    QPainter p(this);
    p.fillRect(rect(), QColor(220, 220, 220));
    
    p.setPen(QColor(50, 50, 50));
    QFont font = p.font();
    font.setWeight(QFont::Bold);
    p.setFont(font);
    
    p.drawText(0, 0, width(), 30, Qt::AlignCenter, tr("MCMC Settings"));
    
    font.setWeight(QFont::Normal);
    //font.setPointSizeF(pointSize(11));
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
    double top = 65.f;
    double lineH = 20;
    double editW = 100;
    double w = width() - 2*m;
    double h = 115;
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
    
    mHelp->setGeometry(m,
                       height() - 3*m - butH - lineH - mHelp->heightForWidth(width() - 2*m),
                       width() - 2*m,
                       mHelp->heightForWidth(width() - 2*m));
    
    mSeedsLab->setGeometry(width()/2 - m/2 - 300, height() - 2*m - butH - lineH, 200, lineH);
    mSeedsEdit->setGeometry(width()/2 + m/2-100, height() - 2*m - butH - lineH, editW, lineH);

    mLabelLevel->setGeometry(width()/2 + m/2+10, height() - 2*m - butH - lineH, editW, lineH);
    mLevelEdit->setGeometry(width()/2 + m/2+120, height() - 2*m - butH - lineH, 50, lineH);
    
    mOkBut->setGeometry(width() - 2*m - 2*butW, height() - m - butH, butW, butH);
    mCancelBut->setGeometry(width() - m - butW, height() - m - butH, butW, butH);
}

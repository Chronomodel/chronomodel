#include "MCMCSettingsDialog.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include <QtWidgets>


MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent, Qt::WindowFlags flags):
QDialog(parent, flags),
  mTotalWidth(110 * AppSettings::widthUnit()),
  mMargin(.3 * AppSettings::heigthUnit()), // marge width
  top(2 * AppSettings::heigthUnit()), // y position of the colored box
  lineH(1.2*AppSettings::heigthUnit()),
  editW(13 * AppSettings::widthUnit()),
  h (10 * AppSettings::heigthUnit()), // size of the colored box
  butW(15*AppSettings::widthUnit()),
  butH(2*AppSettings::heigthUnit())
{
    setWindowTitle(tr("MCMC Settings"));
    setFont(AppSettings::font());
    QFont font (AppSettings::font());
    mSeedsEdit = new LineEdit(this);

    mHelp = new HelpWidget(tr("About seeds : each MCMC chain is different from the others because it uses a different seed. By default, seeds are picked randomly. However, you can force the chains to use specific seeds by entering them below. By doing so, you can replicate exactly the same results using the same seeds."), this);
    mHelp->setLink("https://chronomodel.com/storage/medias/3_chronomodel_user_manual.pdf#page=47"); // chapter 4.2 MCMC settings
    mHelp->setFont(font);

    mNumProcEdit = new LineEdit(this);
    mNumBurnEdit = new LineEdit(this);

    mMaxBatchesEdit = new LineEdit(this);
    mNumIterEdit = new LineEdit(this);
    mDownSamplingEdit = new LineEdit(this);

    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(1);
    
    QIntValidator* chainsValidator = new QIntValidator(this);
    chainsValidator->setRange(1, 5);
    
    mIterPerBatchEdit = new LineEdit(this);
    mIterPerBatchEdit->setValidator(positiveValidator);
    
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
    
   // mLabelLevel = new Label(tr("Mixing level"),this);
    mLevelEdit = new LineEdit(this);

    mOkBut = new Button(tr("OK"), this);
    mCancelBut = new Button(tr("Cancel"), this);
    
    mTestBut = new Button(tr("Quick Test"), this);
#ifdef DEBUG
    mTestBut->setVisible(true);
#else
    mTestBut->setVisible(false);
#endif

    mResetBut = new Button(tr("Restore Defaults"), this);

    const QFontMetrics fm (mResetBut->font());
    mResetBut->setFixedSize(fm.width(mResetBut->text()) + 4*mMargin, butH);

    connect(mOkBut, &Button::clicked, this, &MCMCSettingsDialog::inputControl);
    connect(this, &MCMCSettingsDialog::inputValided, this, &MCMCSettingsDialog::accept);

    connect(mCancelBut, &Button::clicked, this, &MCMCSettingsDialog::reject);
    
    connect(mResetBut, &Button::clicked, this, &MCMCSettingsDialog::reset);
    connect(mTestBut, &Button::clicked, this, &MCMCSettingsDialog::setQuickTest);

    w = mTotalWidth - 2.*mMargin;

    const int fixedHeight = mHelp->heightForWidth(mTotalWidth - 2*mMargin)  + 5*mMargin + butH + lineH + h + top + 20;

    setFixedSize(mTotalWidth, fixedHeight);
}

MCMCSettingsDialog::~MCMCSettingsDialog()
{

}

void MCMCSettingsDialog::setSettings(const MCMCSettings& settings)
{
    const QLocale mLoc = QLocale();
    mNumProcEdit->setText(mLoc.toString(settings.mNumChains));
    mNumIterEdit->setText(mLoc.toString(settings.mNumRunIter));
    mNumBurnEdit->setText(mLoc.toString(settings.mNumBurnIter));
    mMaxBatchesEdit->setText(mLoc.toString(settings.mMaxBatches));
    mIterPerBatchEdit->setText(mLoc.toString(settings.mNumBatchIter));
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
    settings.mNumBatchIter = qMax(UN, (unsigned int) mIterPerBatchEdit->text().toUInt());

    settings.mNumRunIter = qMax((unsigned int) 10, mNumIterEdit->text().toUInt());
    settings.mThinningInterval = qBound(UN, mDownSamplingEdit->text().toUInt(), (unsigned int)floor(settings.mNumRunIter/10) );
    
    settings.mMixingLevel = qBound(0.0001, mLoc.toDouble(mLevelEdit->text()),0.9999);
    
    settings.mSeeds = stringListToIntList(mSeedsEdit->text(), ";");
    
    return settings;
}

void MCMCSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
   // int m  (5);
    
    QPainter p(this);
    p.fillRect(rect(), QColor(220, 220, 220));
    
    p.setPen(Painting::borderDark);
    QFont font = p.font();
    font.setWeight(QFont::Bold);
    p.setFont(font);
    
    font.setWeight(QFont::Normal);
    p.setFont(font);

    p.drawText(0,  2*mMargin  - AppSettings::fontDescent(), width()/2, lineH, Qt::AlignVCenter | Qt::AlignRight, tr("Number of chains") );
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
    
    p.drawText(mBurnRect.adjusted(0, 2*mMargin, 0, -mBurnRect.height() + lineH + 2*mMargin), Qt::AlignCenter, tr("1 - BURN"));
    p.drawText(mBurnRect.adjusted(0, 2*lineH, 0, -mBurnRect.height() + 5*lineH), Qt::AlignCenter, tr("Iterations") );

    p.drawText(mAdaptRect.adjusted(0, 2*mMargin, 0, -mAdaptRect.height() + lineH + 2*mMargin), Qt::AlignCenter, tr("2 - ADAPT"));

    p.drawText(mAquireRect.adjusted(0, 2*mMargin, 0, -mAquireRect.height() + lineH + 2*mMargin), Qt::AlignCenter, tr("3 - ACQUIRE"));
    p.drawText(mAquireRect.adjusted(0, lineH + 4*mMargin, 0, -mAquireRect.height() + 2*lineH + 4*mMargin), Qt::AlignCenter, tr("Iterations") );
    p.drawText(mAquireRect.adjusted(0, 2*lineH + 5*mMargin, 0, -mAquireRect.height() + 6*lineH + 8*mMargin), Qt::AlignCenter, tr("Thinning interval") );
    
    p.drawText(mBatch1Rect.adjusted(0, mMargin, 0, -mBatch1Rect.height() + lineH + mMargin), Qt::AlignCenter, tr("BATCH 1"));
    p.drawText(mBatch1Rect.adjusted(0, lineH + 2*mMargin, 0, -mBatch1Rect.height() + 2*lineH + 2*mMargin), Qt::AlignCenter, tr("Iterations") );

    p.drawText(mBatchInterRect, Qt::AlignCenter, "...");
    p.drawText(mBatchNRect, Qt::AlignCenter, tr("BATCH N"));

    p.drawText(mAdaptRect.x(), mAdaptRect.y() + mAdaptRect.height() - 2*mMargin - lineH, mAdaptRect.width()/2, lineH, Qt::AlignVCenter | Qt::AlignRight, tr("Max batches") );

    const qreal posLine =  mSeedsEdit->y() + mSeedsEdit->height() - 0.3* AppSettings::heigthUnit();
    p.drawText(10 * mMargin, posLine,  tr("Seeds (separated by \";\")"));
    p.drawText(width()/2 + 10 * mMargin , posLine,  tr("Mixing level"));

}

void MCMCSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void MCMCSettingsDialog::updateLayout()
{
    mNumProcEdit->setGeometry(width()/2 + mMargin, 2* mMargin  - AppSettings::fontDescent(), editW, lineH);

    mBurnRect = QRectF(mMargin, top, w * 0.2, h);
    mAdaptRect = QRectF(mMargin + mBurnRect.width(), top, w * 0.4, h);
    mAquireRect = QRectF(mMargin + mBurnRect.width() + mAdaptRect.width(), top, w * 0.4, h);

    mBatch1Rect = QRectF(mAdaptRect.x() + mMargin,
                         mAdaptRect.y() + lineH + 4*mMargin,
                         (mAdaptRect.width() - 4*mMargin) / 3,
                         mAdaptRect.height() - 2*lineH - 8*mMargin);
    mBatchInterRect = mBatch1Rect.adjusted(mBatch1Rect.width() + mMargin, 0, mBatch1Rect.width() + mMargin, 0);
    mBatchNRect = mBatch1Rect.adjusted(2*mBatch1Rect.width() + 2*mMargin, 0, 2*mBatch1Rect.width() + 2*mMargin, 0);
    
    mNumBurnEdit->setGeometry(mBurnRect.x() + (mBurnRect.width() - editW)/2, mBurnRect.y() + 3*lineH + 6*mMargin, editW, lineH);

    mIterPerBatchEdit->setGeometry(mBatch1Rect.x() + mMargin, mBatch1Rect.y() + 2*lineH + 3* mMargin, mBatch1Rect.width() - 2*mMargin, lineH);
    mMaxBatchesEdit->setGeometry(mAdaptRect.x() + mAdaptRect.width()/2 + mMargin, mAdaptRect.y() + mAdaptRect.height() - 2*mMargin - lineH, editW, lineH);

    mNumIterEdit->setGeometry(mAquireRect.x() + (mAquireRect.width() - editW)/2, mAquireRect.y() + 3*lineH + mMargin, editW, lineH);
    mDownSamplingEdit->setGeometry(mAquireRect.x() + (mAquireRect.width() - editW)/2, mAquireRect.y() + 6*lineH + 2*mMargin, editW, lineH);

    mHelp->setGeometry(mMargin,
                       top + h + mMargin,
                       width() - 2*mMargin,
                       mHelp->heightForWidth(width() - 2*mMargin ) );

    QFontMetrics fm(AppSettings::font());

    mSeedsEdit->setGeometry(15* mMargin + fm.width( tr("Seeds (separated by \";\")")), height() - 5*mMargin - butH -lineH, editW, lineH);

    mLevelEdit->setGeometry(width()/2 + 15* mMargin + fm.width( tr("Mixing level")),  height() - 5*mMargin - butH - lineH, editW/1.8, lineH);

    mResetBut->move(mMargin, height() - 2*mMargin - butH );
#ifdef DEBUG
    mTestBut->setGeometry(mResetBut->x() + mResetBut->width() + mMargin, mResetBut->y(), butW, butH);
#endif

    mOkBut->setGeometry(width() - 2*mMargin - 2*butW, mResetBut->y(), butW, butH);
    mCancelBut->setGeometry(width() - mMargin - butW, mResetBut->y() , butW, butH);

}

void MCMCSettingsDialog::inputControl()
{
    bool isValided (true);
    bool ok(true);
    const QLocale mLoc = QLocale();
    QString errorMessage;

    MCMCSettings settings;

    settings.mNumChains = mNumProcEdit->text().toUInt(&ok);
    if (ok == false || settings.mNumChains < 1 ) {
        errorMessage = QObject::tr("The number of chain must be bigger than 0");
        isValided = false;
    }

    settings.mNumBurnIter = mNumBurnEdit->text().toUInt(&ok);
    if (isValided == true && (ok == false || settings.mNumBurnIter < 1) ) {
        errorMessage = QObject::tr("The number of iteration in the burning must be bigger than 0");
        isValided = false;
    }

    settings.mMaxBatches = mMaxBatchesEdit->text().toUInt(&ok);
    if (isValided == true && (ok == false || settings.mMaxBatches < 1) ) {
        errorMessage = QObject::tr("The number of the maximun batches in the adaptation must be bigger than 0");
        isValided = false;
    }

    settings.mNumBatchIter =  mIterPerBatchEdit->text().toUInt(&ok);
    if (isValided == true && settings.mNumBatchIter < 1) {
        errorMessage = QObject::tr("The number of the iteration in one batch of the adaptation must be bigger than 0");
        isValided = false;
    }

    settings.mNumRunIter = mNumIterEdit->text().toUInt(&ok);
    if (isValided == true && (ok == false || settings.mNumRunIter < 50)) {
        errorMessage = QObject::tr("The number of the iteration in one run must be bigger than 50");
        isValided = false;
    }

    settings.mThinningInterval = mDownSamplingEdit->text().toUInt(&ok);
    if (isValided == true && (ok == false || settings.mThinningInterval < 1 ) ) {
        errorMessage = QObject::tr("The thinning interval in one run must be bigger than 1");
        isValided = false;
     }
    if (isValided == true && (ok == false || (settings.mNumRunIter/settings.mThinningInterval) < 40) ) {
        if ((settings.mNumRunIter/40) < 2)
            errorMessage = QObject::tr("With %1 the thinning interval in one run must be 1").arg(mLoc.toString(settings.mNumRunIter));

        else
            errorMessage = QObject::tr("The thinning interval in one run must be smaller than %1").arg(mLoc.toString((unsigned int)floor(settings.mNumRunIter/40)));

        isValided = false;
     }

    settings.mMixingLevel = mLoc.toDouble(mLevelEdit->text(), &ok);
    if (isValided == true && (ok == false || settings.mMixingLevel < 0.0001 || settings.mMixingLevel > 0.9999) ) {
            errorMessage = QObject::tr("The number of the iteration in one run must be bigger than %1  and smaller than %2").arg(mLoc.toString(0.0001), mLoc.toString(0.9999));
            isValided = false;
     }

    if (isValided) {
        settings.mSeeds = stringListToIntList(mSeedsEdit->text(), ";");
        for (auto seed : settings.mSeeds)
            //if (std::isnan(seed) || // bug with MSVC2015_64bit
            if ( seed == 0) {
                errorMessage = QObject::tr("Each seed must be an integer, bigger than 0");
                isValided = false;
            }
    }

    if (isValided)
        emit inputValided();

    else
        QMessageBox::warning(this, tr("Invalid input"),
                                   errorMessage,
                                   QMessageBox::Ok ,
                                   QMessageBox::Ok);

}

void MCMCSettingsDialog::reset()
{
    mNumProcEdit->setText(locale().toString(MCMC_NUM_CHAINS_DEFAULT));
    mNumIterEdit->setText(locale().toString(MCMC_NUM_RUN_DEFAULT));
    mNumBurnEdit->setText(locale().toString(MCMC_NUM_BURN_DEFAULT));
    mMaxBatchesEdit->setText(locale().toString(MCMC_MAX_ADAPT_BATCHES_DEFAULT));
    mIterPerBatchEdit->setText(locale().toString(MCMC_ITER_PER_BATCH_DEFAULT));
    mDownSamplingEdit->setText(locale().toString(MCMC_THINNING_INTERVAL_DEFAULT));
    mSeedsEdit->setText("");

    mLevelEdit->setText(locale().toString(MCMC_MIXING_DEFAULT));
}

void MCMCSettingsDialog::setQuickTest()
{
    mNumProcEdit->setText(locale().toString(1));

    mNumBurnEdit->setText(locale().toString(10));
    mMaxBatchesEdit->setText(locale().toString(10));
    mIterPerBatchEdit->setText(locale().toString(100));
    mNumIterEdit->setText(locale().toString(200));
    mDownSamplingEdit->setText(locale().toString(5));
    mSeedsEdit->setText("");

    mLevelEdit->setText(locale().toString(0.99));
}

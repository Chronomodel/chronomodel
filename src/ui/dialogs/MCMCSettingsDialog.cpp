#include "MCMCSettingsDialog.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include <QtWidgets>


MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent):QDialog(parent),
mTop (int ( 1 * AppSettings::heigthUnit())), // y position of the colored box
mColoredBoxHeigth ( int(4 * AppSettings::heigthUnit())), // size of the colored box 3.5
mBurnBoxWidth (int( 3 *AppSettings::widthUnit())),
mAdaptBoxWidth ( 7 * AppSettings::widthUnit()),
mAcquireBoxWidth (int (3.5 * AppSettings::widthUnit())),
mBottom (3 * AppSettings::heigthUnit()),
mLineH (int (0.5 * AppSettings::heigthUnit())), // heigth of the edit box
mEditW (int (2.5 * AppSettings::widthUnit())),
mButW (int (2.5 *AppSettings::widthUnit())),
mButH ( int (0.5 * AppSettings::heigthUnit())),
mMargin ( (mColoredBoxHeigth - 5*mLineH)/8 ),  // marge width
mTotalWidth (mBurnBoxWidth + mAdaptBoxWidth + mAcquireBoxWidth + 4 * mMargin)
{
    setWindowTitle(tr("MCMC Settings"));
    mMargin = (mColoredBoxHeigth - 5*mLineH)/8;

    mBurnRect = QRectF(mMargin, mTop, mBurnBoxWidth, mColoredBoxHeigth);
    mAdaptRect = QRectF(mBurnRect.x() + mMargin + mBurnBoxWidth, mTop, mAdaptBoxWidth, mColoredBoxHeigth);
    mAquireRect = QRectF(mAdaptRect.x() + mMargin + mAdaptBoxWidth, mTop, mAcquireBoxWidth, mColoredBoxHeigth);

    mBatch1Rect = QRectF(mAdaptRect.x() + mMargin,
                         mAdaptRect.y() + mLineH + 2 * mMargin,
                         (mAdaptRect.width() - 4 * mMargin) / 3,
                         mAdaptRect.height() - 2 * mLineH - 4 * mMargin);

    fm =new QFontMetrics(font(), this);
    
    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(1);

    QIntValidator* chainsValidator = new QIntValidator(this);
    chainsValidator->setRange(1, 5);


    mNumProcLabel = new QLabel(tr("Number of chains"), this);
    mNumProcLabel->setFixedSize(mButW, mButH);
   
    mNumProcEdit = new LineEdit(this);
    mNumProcEdit->setFixedSize(mButW, mButH);
    mNumProcEdit->setValidator(chainsValidator);
    mNumProcEdit->setPlaceholderText(tr("From 1 to 5"));

    // Inside colored boxes
    // 1 - BURN-IN
    mTitleBurnLabel = new QLabel(tr("1 - BURN-IN"), this);
    mTitleBurnLabel->setFixedSize(fm->width(mTitleBurnLabel->text()), mButH);
    mIterBurnLabel = new QLabel(tr("Iterations"), this);
    mIterBurnLabel->setFixedSize(fm->width(mIterBurnLabel->text()), mButH);

    mNumBurnEdit = new LineEdit(this);
    mNumBurnEdit->setAlignment(Qt::AlignCenter);
    mNumBurnEdit->setFixedSize(mButW, mButH);
    mNumBurnEdit->setValidator(positiveValidator);

    // 2- ADAPT
    mIterPerBatchEdit = new LineEdit(this);
    mIterPerBatchEdit->setFixedSize(int (mBatch1Rect.width() - 2 * mMargin), mButH);
    mIterPerBatchEdit->setValidator(positiveValidator);

    mMaxBatchesEdit = new LineEdit(this);
    mMaxBatchesEdit->setFixedSize(mButW, mButH);
    mMaxBatchesEdit->setValidator(positiveValidator);
    mMaxBatchesEdit->setAlignment(Qt::AlignCenter);

    // 3 - ACQUIRE
    mNumIterEdit = new LineEdit(this);
    mNumIterEdit->setFixedSize(mButW, mButH);
    mNumIterEdit->setValidator(positiveValidator);
    mNumIterEdit->setAlignment(Qt::AlignCenter);
    
    mDownSamplingEdit = new LineEdit(this);
    mDownSamplingEdit->setFixedSize(mButW, mButH);
    mDownSamplingEdit->setValidator(positiveValidator);
    mDownSamplingEdit->setAlignment(Qt::AlignCenter);
    
    // On the bottom part
    mHelp = new HelpWidget(tr("About seeds : each MCMC chain is different from the others because it uses a different seed. By default, seeds are picked randomly. However, you can force the chains to use specific seeds by entering them below. By doing so, you can replicate exactly the same results using the same seeds."), this);
    mHelp->setLink("https://chronomodel.com/storage/medias/3_chronomodel_user_manual.pdf#page=47"); // chapter 4.2 MCMC settings

    
    mSeedsLabel = new QLabel(tr("Seeds (separated by \";\")"),this);
    mSeedsLabel->setFixedSize(fm->width(mSeedsLabel->text()), mButH);
    mSeedsEdit = new LineEdit(this);
    mSeedsEdit->setFixedSize(mButW, mButH);
    
    mLevelLabel = new QLabel(tr("Mixing level"),this);
    mLevelLabel->setFixedSize(fm->width(mLevelLabel->text()), mButH);
    mLevelEdit = new LineEdit(this);
    mLevelEdit->setFixedSize(mButW, mButH);

    mOkBut = new Button(tr("OK"), this);
    mOkBut->setFixedSize(mButW, mButH);
    
    mCancelBut = new Button(tr("Cancel"), this);
    mCancelBut->setFixedSize(mButW, mButH);
    
    mTestBut = new Button(tr("Quick Test"), this);
    mTestBut->setFixedSize(fm->width(mTestBut->text()) + 2 * mMargin, mButH);

#ifdef DEBUG
    mTestBut->setFixedSize(fm->width(mTestBut->text()) + 2 * mMargin, mButH);
    mTestBut->setVisible(true);
#else
    mTestBut->setVisible(false);
#endif

    mResetBut = new Button(tr("Restore Defaults"), this);
    mResetBut->setFixedSize(fm->width(mResetBut->text()) + 2 * mMargin, mButH);

    connect(mOkBut, &Button::clicked, this, &MCMCSettingsDialog::inputControl);
    connect(this, &MCMCSettingsDialog::inputValided, this, &MCMCSettingsDialog::accept);

    connect(mCancelBut, &Button::clicked, this, &MCMCSettingsDialog::reject);
    
    connect(mResetBut, &Button::clicked, this, &MCMCSettingsDialog::reset);
    connect(mTestBut, &Button::clicked, this, &MCMCSettingsDialog::setQuickTest);

    const int fixedHeight =  mTop  + mColoredBoxHeigth + mHelp->heightForWidth(mTotalWidth - 2 * mMargin)  + 6 * mMargin + mButH + mLineH;
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
    const int UN = 1;
    settings.mNumChains = qMax(UN, mNumProcEdit->text().toInt());

    settings.mNumBurnIter = qMax(UN, mNumBurnEdit->text().toInt());

    settings.mMaxBatches = qMax(UN, mMaxBatchesEdit->text().toInt());
    settings.mNumBatchIter = qMax(UN,  mIterPerBatchEdit->text().toInt());

    settings.mNumRunIter = qMax(10, mNumIterEdit->text().toInt());
    settings.mThinningInterval = qBound(UN, mDownSamplingEdit->text().toInt(), int (floor(settings.mNumRunIter/10)) );
    
    settings.mMixingLevel = qBound(0.0001, mLoc.toDouble(mLevelEdit->text()),0.9999);
    
    settings.mSeeds = stringListToIntList(mSeedsEdit->text(), ";");
    
    return settings;
}

void MCMCSettingsDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);
    p.fillRect(rect(), QColor(220, 220, 220));
    
    p.setPen(Painting::borderDark);

    QFont font = p.font();
    font.setWeight(QFont::Bold);
    p.setFont(font);
    
    font.setWeight(QFont::Normal);
    p.setFont(font);

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

    p.drawText(mAdaptRect.adjusted(0, 1 * mMargin, 0, -mAdaptRect.height() + mLineH + 1 * mMargin), Qt::AlignCenter, tr("2 - ADAPT"));

    
    p.drawText(mBatch1Rect.adjusted(0, mMargin, 0, -mBatch1Rect.height() + mLineH + mMargin), Qt::AlignCenter, tr("BATCH 1"));
    p.drawText(mBatch1Rect.adjusted(0, mLineH + 2 * mMargin, 0, -mBatch1Rect.height() + 2 * mLineH + 2 * mMargin), Qt::AlignCenter, tr("Iterations") );

    p.drawText(mBatchInterRect, Qt::AlignCenter, "...");
    p.drawText(mBatchNRect, Qt::AlignCenter, tr("BATCH N"));

    p.drawText(int (mAdaptRect.x()), int (mAdaptRect.y() + mAdaptRect.height() - 1 * mMargin - mLineH), int (mAdaptRect.width()/2), mLineH, Qt::AlignVCenter | Qt::AlignRight, tr("Max batches") );


    p.drawText(mAquireRect.adjusted(0, mMargin, 0, -mAquireRect.height() + mLineH + 1 * mMargin), Qt::AlignCenter, tr("3 - ACQUIRE"));
    p.drawText(mAquireRect.adjusted(0, mLineH + 2 * mMargin, 0, -mAquireRect.height() + 2 * mLineH + 2 * mMargin), Qt::AlignCenter, tr("Iterations") );
    p.drawText(int (mAquireRect.x() + (mAcquireBoxWidth - fm->width(tr("Thinning interval")))/2), int (mAquireRect.y() + 3 * mLineH + 4 * mMargin), fm->width(tr("Thinning interval")), mButH, Qt::AlignCenter, tr("Thinning interval") );


}

void MCMCSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void MCMCSettingsDialog::updateLayout()
{
    
    mNumProcLabel->move(width()/2 - mMargin - fm->width(mNumProcLabel->text()), (mTop - mNumProcLabel->height()) /2 );
    mNumProcEdit->move(width()/2 + mMargin, (mTop - mNumProcLabel->height()) /2);

    // setting Edit boxes
    // 1 -BURN
    mTitleBurnLabel->move(int (mBurnRect.x() + (mBurnBoxWidth - fm->width(mTitleBurnLabel->text()))/2), int (mBurnRect.y()) + 1 * mMargin);
    mIterBurnLabel->move(int (mBurnRect.x() + (mBurnBoxWidth - fm->width(mIterBurnLabel->text()))/2), mTitleBurnLabel->y() + mTitleBurnLabel->height() + 1 * mMargin);
    mNumBurnEdit->move(int (mBurnRect.x() + (mBurnBoxWidth - mEditW)/2),  mIterBurnLabel->y() + mIterBurnLabel->height() + 1 * mMargin);

    // 2 - ADAPT
    // mBatch1Rect is defined in the constructor
    mBatchInterRect = mBatch1Rect.adjusted(mBatch1Rect.width() + mMargin, 0, mBatch1Rect.width() + mMargin, 0);
    mBatchNRect = mBatch1Rect.adjusted(2*mBatch1Rect.width() + 2 * mMargin, 0, 2 * mBatch1Rect.width() + 2 * mMargin, 0);

    mIterPerBatchEdit->move(int (mBatch1Rect.x() + mMargin), int (mBatch1Rect.y() + 2 * mLineH + 3 * mMargin));
    mMaxBatchesEdit->move(int (mAdaptRect.x() + mAdaptRect.width()/2 + mMargin), int (mAdaptRect.y() + mAdaptRect.height() - 1 * mMargin - mLineH));

    // 3 - ACQUIRE
    mNumIterEdit->move(int (mAquireRect.x() + (mAcquireBoxWidth - mEditW)/2), int (mAquireRect.y() + 2 * mLineH + 3 * mMargin));
    mDownSamplingEdit->move(int (mAquireRect.x() + (mAcquireBoxWidth - mEditW)/2), int (mAquireRect.y() + 4 * mLineH + 5 * mMargin));

    // Help box
    mHelp->setGeometry(mMargin,
                       mTop + mColoredBoxHeigth + mMargin,
                       width() - 2 * mMargin,
                       mHelp->heightForWidth(width() - 2 * mMargin ) );

   // Bottom Info
    mSeedsLabel->move(AppSettings::widthUnit(), height() - 4 * mMargin - mButH - mLineH);
    mSeedsEdit->move(mSeedsLabel->x() + mSeedsLabel->width() + mMargin, mSeedsLabel->y());

    mLevelLabel->move(width()/2 + 2 * mMargin, mSeedsLabel->y());
    mLevelEdit->move(mLevelLabel->x() + mLevelLabel->width() + mMargin,  mLevelLabel->y());

    mResetBut->move( 2 * mMargin, height() - 2 * mMargin - mButH );
#ifdef DEBUG
    mTestBut->move(mResetBut->x() + mResetBut->width() + mMargin, mResetBut->y());
#endif

    mOkBut->move(width() - 4 * mMargin - mOkBut->width() - mCancelBut->width(), mResetBut->y());
    mCancelBut->move(mOkBut->x() + mOkBut->width() + mMargin, mResetBut->y());

}

void MCMCSettingsDialog::inputControl()
{
    bool isValided (true);
    bool ok(true);
    const QLocale mLoc = QLocale();
    QString errorMessage;

    MCMCSettings settings;

    settings.mNumChains = mNumProcEdit->text().toInt(&ok);
    if (ok == false || settings.mNumChains < 1 ) {
        errorMessage = QObject::tr("The number of chain must be bigger than 0");
        isValided = false;
    }

    settings.mNumBurnIter = mNumBurnEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mNumBurnIter < 1) ) {
        errorMessage = QObject::tr("The number of iteration in the burn-in must be bigger than 0");
        isValided = false;
    }

    settings.mMaxBatches = mMaxBatchesEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mMaxBatches < 1) ) {
        errorMessage = QObject::tr("The number of the maximun batches in the adaptation must be bigger than 0");
        isValided = false;
    }

    settings.mNumBatchIter =  mIterPerBatchEdit->text().toInt(&ok);
    if (isValided == true && settings.mNumBatchIter < 1) {
        errorMessage = QObject::tr("The number of the iteration in one batch of the adaptation must be bigger than 0");
        isValided = false;
    }

    settings.mNumRunIter = mNumIterEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mNumRunIter < 50)) {
        errorMessage = QObject::tr("The number of the iteration in one run must be bigger than 50");
        isValided = false;
    }

    settings.mThinningInterval = mDownSamplingEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mThinningInterval < 1 ) ) {
        errorMessage = QObject::tr("The thinning interval in one run must be bigger than 1");
        isValided = false;
     }
    if (isValided == true && (ok == false || (settings.mNumRunIter/settings.mThinningInterval) < 40) ) {
        if ((settings.mNumRunIter/40) < 2)
            errorMessage = QObject::tr("With %1 the thinning interval in one run must be 1").arg(mLoc.toString(settings.mNumRunIter));

        else
            errorMessage = QObject::tr("The thinning interval in one run must be smaller than %1").arg(mLoc.toString(static_cast<unsigned int>(floor(settings.mNumRunIter/40))));

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

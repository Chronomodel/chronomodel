/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "MCMCSettingsDialog.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "HelpWidget.h"
#include <QtWidgets>


MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent):QDialog(parent),
mTop ( int(3.5 * fontMetrics().height())), // y position of the colored box
mColoredBoxHeigth ( 10 * fontMetrics().height()), // size of the colored box 3.5
mBurnBoxWidth (fontMetrics().boundingRect("0000000000").width()),
mAdaptBoxWidth ( fontMetrics().boundingRect("0000000000").width()),
mAcquireBoxWidth (int (3.5 * fontMetrics().boundingRect("0000000000").width())),
mBottom (3 * fontMetrics().height()),
mLineH (int (1.1* fontMetrics().height())), // heigth of the edit box
mEditW (fontMetrics().boundingRect("0000000000").width()),
mButW (fontMetrics().boundingRect("0000000000").width()),
mButH ( int (1.2 * fontMetrics().height())),
mMarginW ( int( 0.5 * fontMetrics().height() )),  // marge width
mMarginH ( (mColoredBoxHeigth - 5*mLineH)/8 ),  // marge Heigth
mTotalWidth (mBurnBoxWidth + mAdaptBoxWidth + mAcquireBoxWidth + 4 * mMarginW)
{
    setWindowTitle(tr("MCMC Settings"));

    mBurnBoxWidth = int(1.5 * mEditW);
    mAdaptBoxWidth = int(3.5 * mEditW);
    mAcquireBoxWidth = int(1.5 * mEditW);
    mTotalWidth  = mBurnBoxWidth + mAdaptBoxWidth + mAcquireBoxWidth + 4 * mMarginW;

    mBurnRect = QRectF(mMarginW, mTop, mBurnBoxWidth, mColoredBoxHeigth);
    mAdaptRect = QRectF(mBurnRect.x() + mMarginW + mBurnBoxWidth, mTop, mAdaptBoxWidth, mColoredBoxHeigth);
    mAquireRect = QRectF(mAdaptRect.x() + mMarginW + mAdaptBoxWidth, mTop, mAcquireBoxWidth, mColoredBoxHeigth);

    mBatch1Rect = QRectF(mAdaptRect.x() + mMarginW,
                         mAdaptRect.y() + mLineH + 2 * mMarginH,
                         (mAdaptRect.width() - 4 * mMarginW) / 3,
                         mAdaptRect.height() - 2 * mLineH - 4 * mMarginH);

    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(1);

    QIntValidator* chainsValidator = new QIntValidator(this);
    chainsValidator->setRange(1, 5);

    mNumProcLabel = new QLabel(tr("Number of chains"), this);

    mNumProcEdit = new LineEdit(this);
    mNumProcEdit->setFixedSize(mEditW, mButH);
    mNumProcEdit->setValidator(chainsValidator);
    mNumProcEdit->setPlaceholderText(tr("From 1 to 5"));

    // Inside colored boxes
    // 1 - BURN-IN
    mTitleBurnLabel = new QLabel(tr("1 - BURN-IN"), this);
    mTitleBurnLabel->setFixedSize(fontMetrics().boundingRect(mTitleBurnLabel->text()).width(), mButH);
    mIterBurnLabel = new QLabel(tr("Iterations"), this);
    mIterBurnLabel->setFixedSize(fontMetrics().boundingRect(mIterBurnLabel->text()).width(), mButH);

    mNumBurnEdit = new LineEdit(this);
    mNumBurnEdit->setAlignment(Qt::AlignCenter);
    mNumBurnEdit->setFixedSize(mEditW, mButH);
    mNumBurnEdit->setValidator(positiveValidator);

    // 2- ADAPT
    mIterPerBatchEdit = new LineEdit(this);
    mIterPerBatchEdit->setFixedSize(int (mBatch1Rect.width() - 2 * mMarginW), mButH);
    mIterPerBatchEdit->setValidator(positiveValidator);

    mMaxBatchesEdit = new LineEdit(this);
    mMaxBatchesEdit->setFixedSize(mEditW, mButH);
    mMaxBatchesEdit->setValidator(positiveValidator);
    mMaxBatchesEdit->setAlignment(Qt::AlignCenter);

    // 3 - ACQUIRE
    mNumIterEdit = new LineEdit(this);
    mNumIterEdit->setFixedSize(mEditW, mButH);
    mNumIterEdit->setValidator(positiveValidator);
    mNumIterEdit->setAlignment(Qt::AlignCenter);

    mDownSamplingEdit = new LineEdit(this);
    mDownSamplingEdit->setFixedSize(mEditW, mButH);
    mDownSamplingEdit->setValidator(positiveValidator);
    mDownSamplingEdit->setAlignment(Qt::AlignCenter);

    // On the bottom part
    mHelp = new HelpWidget(tr("About seeds : Each MCMC chain is different from the others because it uses a different seed. By default, seeds are picked randomly. However, you can force the chains to use specific seeds by entering them below. By doing so, you can replicate exactly the same results using the same seeds."), this);
    mHelp->setLink("https://chronomodel.com/storage/medias/3_chronomodel_user_manual.pdf#page=52"); // chapter 4.2 MCMC settings

    mSeedsLabel = new QLabel(tr("Seeds (separated by \";\")"),this);
    mSeedsLabel->setFixedSize(fontMetrics().boundingRect(mSeedsLabel->text()).width(), mButH);
    mSeedsEdit = new LineEdit(this);
    mSeedsEdit->setFixedSize(mEditW, mButH);

    mLevelLabel = new QLabel(tr("Mixing level"),this);
    mLevelEdit = new LineEdit(this);

    mLambdaLabel = new QLabel(tr("Lambda level"),this);
    mLambdaEdit = new LineEdit(this);

    mLambdaLabel->setFixedSize(fontMetrics().boundingRect(mLambdaLabel->text()).width(), mButH);
    mLambdaEdit->setFixedSize(mButW, mButH);

    mLambdaLabel->setVisible(true);
    mLambdaEdit->setVisible(true);

#ifdef DEBUG
    mLevelLabel->setFixedSize(fontMetrics().boundingRect(mLevelLabel->text()).width(), mButH);
    mLevelEdit->setFixedSize(mButW, mButH);

    mLevelLabel->setVisible(true);
    mLevelEdit->setVisible(true);


#else
    mLevelLabel->setVisible(false);
    mLevelEdit->setVisible(false);

#endif

    mOkBut = new Button(tr("OK"), this);
    mOkBut->setFixedSize(mButW, mButH);

    mCancelBut = new Button(tr("Cancel"), this);
    mCancelBut->setFixedSize(fontMetrics().boundingRect(mCancelBut->text()).width() + 2 * mMarginW, mButH);

    mTestBut = new Button(tr("Quick Test"), this);
    mTestBut->setFixedSize(fontMetrics().boundingRect(mTestBut->text()).width() + 2 * mMarginW, mButH);

#ifdef DEBUG
    mTestBut->setFixedSize(fontMetrics().horizontalAdvance(mTestBut->text()) + 2 * mMarginW, mButH);
    mTestBut->setVisible(true);
#else
    mTestBut->setVisible(false);
#endif

    mResetBut = new Button(tr("Restore Defaults"), this);
    mResetBut->setFixedSize(fontMetrics().boundingRect(mResetBut->text()).width() + 2 * mMarginW, mButH);

    connect(mOkBut, &Button::clicked, this, &MCMCSettingsDialog::inputControl);
    connect(this, &MCMCSettingsDialog::inputValided, this, &MCMCSettingsDialog::accept);

    connect(mCancelBut, &Button::clicked, this, &MCMCSettingsDialog::reject);

    connect(mResetBut, &Button::clicked, this, &MCMCSettingsDialog::reset);
    connect(mTestBut, &Button::clicked, this, &MCMCSettingsDialog::setQuickTest);

    const int fixedHeight =  mTop  + mColoredBoxHeigth + mHelp->heightForWidth(mTotalWidth - 2 * mMarginW)  + 6 * mMarginH +  mButH + 2*mLineH;
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

    mLambdaEdit->setText(mLoc.toString(ro));
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

    ro = qBound(0.000001, mLoc.toDouble(mLambdaEdit->text()),10.);
    settings.mLambdaLevel = qBound(0.000001, mLoc.toDouble(mLambdaEdit->text()),10.);

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

    p.drawText(mAdaptRect.adjusted(0, 1 * mMarginH, 0, -mAdaptRect.height() + mLineH + 1 * mMarginH), Qt::AlignCenter, tr("2 - ADAPT"));


    p.drawText(mBatch1Rect.adjusted(0, mMarginH, 0, -mBatch1Rect.height() + mLineH + mMarginH), Qt::AlignCenter, tr("BATCH 1"));
    p.drawText(mBatch1Rect.adjusted(0, mLineH + 2 * mMarginH, 0, -mBatch1Rect.height() + 2 * mLineH + 2 * mMarginH), Qt::AlignCenter, tr("Iterations") );

    p.drawText(mBatchInterRect, Qt::AlignCenter, "...");
    p.drawText(mBatchNRect, Qt::AlignCenter, tr("BATCH N"));

    p.drawText(int (mAdaptRect.x()), int (mAdaptRect.y() + mAdaptRect.height() - 1 * mMarginH - mLineH), int (mAdaptRect.width()/2), mLineH, Qt::AlignVCenter | Qt::AlignRight, tr("Max batches") );


    p.drawText(mAquireRect.adjusted(0, mMarginH, 0, -mAquireRect.height() + mLineH + 1 * mMarginH), Qt::AlignCenter, tr("3 - ACQUIRE"));
    p.drawText(mAquireRect.adjusted(0, mLineH + 2 * mMarginH, 0, -mAquireRect.height() + 2 * mLineH + 2 * mMarginH), Qt::AlignCenter, tr("Iterations") );
    p.drawText(int (mAquireRect.x() + (mAcquireBoxWidth - fontMetrics().boundingRect(tr("Thinning")).width())/2), int (mAquireRect.y() + 3 * mLineH + 4 * mMarginH), fontMetrics().boundingRect(tr("Thinning")).width(), mButH, Qt::AlignCenter, tr("Thinning") );


}

void MCMCSettingsDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void MCMCSettingsDialog::updateLayout()
{

    mNumProcLabel->move(width()/2 - mMarginW - fontMetrics().boundingRect(mNumProcLabel->text()).width(), (mTop - mNumProcLabel->height()) /2 );
    mNumProcEdit->move(width()/2 + mMarginW, (mTop - mNumProcLabel->height()) /2);

    // Setting Edit boxes
    // 1 -BURN
    mTitleBurnLabel->move(int (mBurnRect.x() + (mBurnBoxWidth - fontMetrics().boundingRect(mTitleBurnLabel->text()).width())/2), int (mBurnRect.y()) + 1 * mMarginH);
    mIterBurnLabel->move(int (mBurnRect.x() + (mBurnBoxWidth - fontMetrics().boundingRect(mIterBurnLabel->text()).width())/2), mTitleBurnLabel->y() + mTitleBurnLabel->height() + 1 * mMarginH);
    mNumBurnEdit->move(int (mBurnRect.x() + (mBurnBoxWidth - mEditW)/2),  mIterBurnLabel->y() + mIterBurnLabel->height() + 1 * mMarginH);

    // 2 - ADAPT
    // mBatch1Rect is defined in the constructor
    mBatchInterRect = mBatch1Rect.adjusted(mBatch1Rect.width() + mMarginW, 0, mBatch1Rect.width() + mMarginW, 0);
    mBatchNRect = mBatch1Rect.adjusted(2*mBatch1Rect.width() + 2 * mMarginW, 0, 2 * mBatch1Rect.width() + 2 * mMarginW, 0);

    mIterPerBatchEdit->move(int (mBatch1Rect.x() + mMarginW), int (mBatch1Rect.y() + 2 * mLineH + 3 * mMarginH));
    mMaxBatchesEdit->move(int (mAdaptRect.x() + mAdaptRect.width()/2 + mMarginW), int (mAdaptRect.y() + mAdaptRect.height() - 1 * mMarginH - mLineH));

    // 3 - ACQUIRE
    mNumIterEdit->move(int (mAquireRect.x() + (mAcquireBoxWidth - mEditW)/2), int (mAquireRect.y() + 2 * mLineH + 3 * mMarginH));
    mDownSamplingEdit->move(int (mAquireRect.x() + (mAcquireBoxWidth - mEditW)/2), int (mAquireRect.y() + 4 * mLineH + 5 * mMarginH));

    // Help box
    mHelp->setGeometry(mMarginW,
                       mTop + mColoredBoxHeigth + mMarginH,
                       width() - 2 * mMarginW,
                       mHelp->heightForWidth(width() - 2 * mMarginW ) );

   // Bottom Info
    const int margingLeft = (width()/2 - mSeedsLabel->width() - mSeedsEdit->width() - mMarginW)/2;
    mSeedsLabel->move(margingLeft, height() - 4 * mMarginH - mButH - mLineH);
    mSeedsEdit->move(mSeedsLabel->x() + mSeedsLabel->width() + mMarginW, mSeedsLabel->y());
    const int margingRight = (width()/2 -mLevelLabel->width() - mLevelEdit->width() - mMarginW)/2;
 #ifdef DEBUG
    mLevelLabel->move(width()/2 + margingRight, mSeedsLabel->y() -fontMetrics().height() + 5);
    mLevelEdit->move(mLevelLabel->x() + mLevelLabel->width() + mMarginW,  mLevelLabel->y());
#endif
    mLambdaLabel->move(width()/2 + margingRight, mSeedsLabel->y());
    mLambdaEdit->move(mLambdaLabel->x() + mLambdaLabel->width() + mMarginW,  mLambdaLabel->y());

    mResetBut->move( 2 * mMarginW, height() - 2 * mMarginH - mResetBut->height() );
#ifdef DEBUG
    mTestBut->move(mResetBut->x() + mResetBut->width() + mMarginW, mResetBut->y());
#endif

    mOkBut->move(width() - 4 * mMarginW - mOkBut->width() - mCancelBut->width(), mResetBut->y());
    mCancelBut->move(mOkBut->x() + mOkBut->width() + mMarginW, mResetBut->y());

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

    settings.mLambdaLevel = mLoc.toDouble(mLambdaEdit->text(), &ok);
    if (isValided == true && (ok == false) ) {
            errorMessage = QObject::tr("The number ro must be a number");
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
    ro = 1.;
    mLambdaEdit->setText(locale().toString(ro));
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
    ro = 1.;
    mLambdaEdit->setText(locale().toString(ro));
}

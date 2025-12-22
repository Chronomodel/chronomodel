/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
#include "LineEdit.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "HelpWidget.h"

#include <QtWidgets>

MCMCSettingsDialog::MCMCSettingsDialog(QWidget* parent, const bool show_help):QDialog(parent),
    mTop ( int(3.5 * fontMetrics().height())), // y position of the colored box
    mColoredBoxHeigth ( 10 * fontMetrics().height()), // size of the colored box 3.5
    mBurnBoxWidth (fontMetrics().horizontalAdvance("0000000000")),
    mAdaptBoxWidth ( fontMetrics().horizontalAdvance("0000000000")),
    mAcquireBoxWidth (int (3.5 * fontMetrics().horizontalAdvance("0000000000"))),
    mBottom (3 * fontMetrics().height()),
    mLineH (int (1.1* fontMetrics().height())), // heigth of the edit box
    mEditW (fontMetrics().horizontalAdvance("0000000000")),
    mButW (fontMetrics().horizontalAdvance("0000000000")),
    mButH ( int (1.2 * fontMetrics().height())),
    mMarginW ( int( 0.5 * fontMetrics().height() )),  // marge width
    mMarginH ( (mColoredBoxHeigth - 5 * mLineH)/8 ),  // marge Heigth
    mTotalWidth (mBurnBoxWidth + mAdaptBoxWidth + mAcquireBoxWidth + 4 * mMarginW)
{
    setWindowTitle(tr("MCMC Settings"));
    setMouseTracking(true);
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
    chainsValidator->setRange(1, 10);

    mNumProcLabel = new QLabel(tr("Number of chains"), this);

    mNumProcEdit = new LineEdit(this);
    mNumProcEdit->setFixedSize(mEditW, mButH);
    mNumProcEdit->setValidator(chainsValidator);
    mNumProcEdit->setPlaceholderText(tr("From 1 to 10"));
   // mNumProcEdit->setPalette(palette);

    // Inside colored boxes
    // 1 - BURN-IN
    mTitleBurnLabel = new QLabel(tr("1 - BURN-IN"), this);
    mTitleBurnLabel->setFixedSize(fontMetrics().horizontalAdvance(mTitleBurnLabel->text()), mButH);
    mIterBurnLabel = new QLabel(tr("Iterations"), this);
    mIterBurnLabel->setFixedSize(fontMetrics().horizontalAdvance(mIterBurnLabel->text()), mButH);

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
    if (show_help) {
        mHelp = new HelpWidget(tr("About seeds : Each MCMC chain is different from the others because it uses a different seed. By default, seeds are picked randomly. However, you can force the chains to use specific seeds by entering them below. By doing so, you can replicate exactly the same results using the same seeds."), this);
        mHelp->setLink("https://chronomodel.com/storage/medias/83_chronomodel_v32_user_manual_2024_05_13_min.pdf#page=52"); // chapter 4.2 MCMC settings
        mHelp->setVisible(show_help);
    } else {
        mHelp = new HelpWidget(this);
        mHelp->setVisible(show_help);
    }


    mSeedsLabel = new QLabel(tr("Seeds (separated by \";\")"),this);
    mSeedsLabel->setFixedSize(fontMetrics().horizontalAdvance(mSeedsLabel->text()), mButH);
    mSeedsEdit = new LineEdit(this);
    mSeedsEdit->setFixedSize(3*mEditW, mButH);

    mLevelLabel = new QLabel(tr("Mixing level"),this);
    mLevelEdit = new LineEdit(this);

#ifdef DEBUG
    mLevelLabel->setFixedSize(fontMetrics().horizontalAdvance(mLevelLabel->text()), mButH);
    mLevelEdit->setFixedSize(mButW, mButH);

    mLevelLabel->setVisible(true);
    mLevelEdit->setVisible(true);
#else
    mLevelLabel->setVisible(false);
    mLevelEdit->setVisible(false);
#endif

    mOkBut = new QPushButton(tr("OK"), this);

    mCancelBut = new QPushButton(tr("Cancel"), this);

    mTestBut = new QPushButton(tr("Quick Test"), this);

#ifdef DEBUG
   // mTestBut->setFixedSize(fontMetrics().horizontalAdvance(mTestBut->text()) + 2 * mMarginW, mButH);
    mTestBut->setVisible(true);
#else
    mTestBut->setVisible(false);
#endif
    setAttribute(Qt::WA_AlwaysShowToolTips);
    mResetBut = new QPushButton(tr("Restore Defaults"), this);
    mResetBut->setToolTip(tr("Restore default parameter values"));
    mResetBut->setToolTipDuration(1000);
    mResetBut->QWidget::setAttribute(Qt::WA_AlwaysShowToolTips);


    connect(mOkBut, &QPushButton::clicked, this, &MCMCSettingsDialog::inputControl);
    connect(this, &MCMCSettingsDialog::inputValided, this, &MCMCSettingsDialog::accept);
    connect(this, &MCMCSettingsDialog::nothing, this, &MCMCSettingsDialog::reject);
    connect(mCancelBut, &QPushButton::clicked, this, &MCMCSettingsDialog::reject);

    connect(mResetBut, &QPushButton::clicked, this, &MCMCSettingsDialog::reset);
    connect(mTestBut, &QPushButton::clicked, this, &MCMCSettingsDialog::setQuickTest);

    const int fixedHeight =   mTop  + mColoredBoxHeigth + (show_help? mHelp->heightForWidth(mTotalWidth - 2 * mMarginW) : 0.)  + 5 * mMarginH + 2 * mLineH + mOkBut->height() ;
    setFixedSize(mTotalWidth, fixedHeight);

}

MCMCSettingsDialog::~MCMCSettingsDialog()
{

}

void MCMCSettingsDialog::setSettings(const MCMCSettings& settings)
{
    initalSetting = settings;
    const QLocale mLoc = QLocale();
    mNumProcEdit->setText(mLoc.toString(settings.mNumChains));
    mNumIterEdit->setText(mLoc.toString(settings.mIterPerAquisition));
    mNumBurnEdit->setText(mLoc.toString(settings.mIterPerBurn));
    mMaxBatchesEdit->setText(mLoc.toString(settings.mMaxBatches));
    mIterPerBatchEdit->setText(mLoc.toString(settings.mIterPerBatch));
    mDownSamplingEdit->setText(mLoc.toString(settings.mThinningInterval));
    mSeedsEdit->setText(QListUnsignedToQString(settings.mSeeds, ";"));

    mLevelEdit->setText(mLoc.toString(settings.mMixingLevel));
}

MCMCSettings MCMCSettingsDialog::getSettings()
{
    const QLocale mLoc = QLocale();
    MCMCSettings settings;
    const int UN = 1;
    settings.mNumChains = qMax(UN, mNumProcEdit->text().toInt());

    settings.mIterPerBurn = qMax(UN, mNumBurnEdit->text().toInt());

    settings.mMaxBatches = qMax(UN, mMaxBatchesEdit->text().toInt());
    settings.mIterPerBatch = qMax(UN,  mIterPerBatchEdit->text().toInt());

    settings.mIterPerAquisition = qMax(10, mNumIterEdit->text().toInt());
    settings.mThinningInterval = std::clamp(mDownSamplingEdit->text().toInt(), UN, int (floor(settings.mIterPerAquisition/10)) );

    settings.mMixingLevel = std::clamp(mLoc.toDouble(mLevelEdit->text()), 0.0001, 0.9999);

    settings.mSeeds = QStringToQListUnsigned(mSeedsEdit->text(), ";");

    return settings;
}


void MCMCSettingsDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), palette().window().color());// QColor(220, 220, 220));

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
    p.drawText(int (mAquireRect.x() + (mAcquireBoxWidth - fontMetrics().horizontalAdvance(tr("Thinning")))/2), int (mAquireRect.y() + 3 * mLineH + 4 * mMarginH), fontMetrics().horizontalAdvance(tr("Thinning")), mButH, Qt::AlignCenter, tr("Thinning") );


    qDebug()<<mResetBut->QWidget::toolTip()<<mResetBut->toolTipDuration()<<QToolTip::isVisible();

}

void MCMCSettingsDialog::resizeEvent(QResizeEvent* e)
{
    (void)e;
    updateLayout();
}

void MCMCSettingsDialog::updateLayout()
{

    mNumProcLabel->move(width()/2 - mMarginW - fontMetrics().horizontalAdvance(mNumProcLabel->text()), (mTop - mNumProcLabel->height()) /2 );
    mNumProcEdit->move(width()/2 + mMarginW, (mTop - mNumProcLabel->height()) /2);

    // Setting Edit boxes
    // 1 -BURN
    mTitleBurnLabel->move(int (mBurnRect.x() + (mBurnBoxWidth - fontMetrics().horizontalAdvance(mTitleBurnLabel->text()))/2), int (mBurnRect.y()) + 1 * mMarginH);
    mIterBurnLabel->move(int (mBurnRect.x() + (mBurnBoxWidth - fontMetrics().horizontalAdvance(mIterBurnLabel->text()))/2), mTitleBurnLabel->y() + mTitleBurnLabel->height() + 1 * mMarginH);
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
    if (mHelp->text().isEmpty())
        mHelp->setGeometry(0, 0, 0, 0);
    else
        mHelp->setGeometry(mMarginW,
                           mTop + mColoredBoxHeigth + mMarginH,
                           width() - 2 * mMarginW,
                           mHelp->heightForWidth(width() - 2 * mMarginW ) );


   // Bottom Info

    mSeedsLabel->move(2 * mMarginW, height() - 5 * mMarginH - mButH - mLineH);
    mSeedsEdit->move(mSeedsLabel->x() + mSeedsLabel->width() + mMarginW, mSeedsLabel->y());
    const int margingRight = (width()/2 -mLevelLabel->width() - mLevelEdit->width() - mMarginW)/2;
    mLevelLabel->move(width()/2 + margingRight, mSeedsLabel->y());
    mLevelEdit->move(mLevelLabel->x() + mLevelLabel->width() + mMarginW,  mLevelLabel->y());

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
    bool ok (true);
    const QLocale mLoc = QLocale();
    QString errorMessage;

    MCMCSettings settings;

    settings.mNumChains = mNumProcEdit->text().toInt(&ok);
    if (ok == false || settings.mNumChains < 1 ) {
        errorMessage = tr("The number of chain must be bigger than 0");
        isValided = false;
    }

    settings.mIterPerBurn = mNumBurnEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mIterPerBurn < 1) ) {
        errorMessage = tr("The number of iteration in the burn-in must be bigger than 0");
        isValided = false;
    }

    settings.mMaxBatches = mMaxBatchesEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mMaxBatches < 1) ) {
        errorMessage = tr("The number of the maximun batches in the adaptation must be bigger than 0");
        isValided = false;
    }

    settings.mIterPerBatch =  mIterPerBatchEdit->text().toInt(&ok);
    if (isValided == true && settings.mIterPerBatch < 1) {
        errorMessage = tr("The number of the iteration in one batch of the adaptation must be bigger than 0");
        isValided = false;
    }

    settings.mIterPerAquisition = mNumIterEdit->text().toInt(&ok);

    if (isValided == true && (ok == false || settings.mIterPerAquisition < 50)) {
        errorMessage = tr("The number of the iteration in one run must be bigger than 50");

#ifndef DEBUG
        isValided = false;
#else
        errorMessage = tr("But this is DEBUG ...");
#endif
    }

    settings.mThinningInterval = mDownSamplingEdit->text().toInt(&ok);
    if (isValided == true && (ok == false || settings.mThinningInterval < 1 ) ) {
        errorMessage = tr("The thinning interval in one run must be bigger than 1");
        isValided = false;
     }

    if (isValided == true && (ok == false || (settings.mIterPerAquisition/settings.mThinningInterval) < 40) ) {
        if ((settings.mIterPerAquisition/40) < 2)
            errorMessage = tr("With %1 the thinning interval in one run must be 1").arg(mLoc.toString(settings.mIterPerAquisition));

        else
            errorMessage = tr("The thinning interval in one run must be smaller than %1").arg(mLoc.toString(static_cast<unsigned int>(floor(settings.mIterPerAquisition/40))));
#ifndef DEBUG
        isValided = false;
#else
        errorMessage = tr("But this is DEBUG ...");
#endif
     }

    settings.mMixingLevel = mLoc.toDouble(mLevelEdit->text(), &ok);
    if (isValided == true && (ok == false || settings.mMixingLevel < 0.0001 || settings.mMixingLevel > 0.9999) ) {
            errorMessage = tr("The number of the iteration in one run must be bigger than %1  and smaller than %2").arg(mLoc.toString(0.0001), mLoc.toString(0.9999));
            isValided = false;
     }

    QList<int> seedList;
    if (isValided) {
        seedList = QStringToQListInt(mSeedsEdit->text(), ";");
        for (auto& seed : seedList)
            if ( seed <= 0) {
                errorMessage = tr("Each seed must be an integer, bigger than 0");
                isValided = false;
            }
    }

    if (isValided) {
        settings.mSeeds = QStringToQListUnsigned(mSeedsEdit->text(), ";");
        if (initalSetting == settings)
            emit nothing();
        else
            emit inputValided();

   } else
        QMessageBox::warning(this, tr("Invalid input"),
                                   errorMessage,
                                   QMessageBox::Ok ,
                                   QMessageBox::Ok);

}

void MCMCSettingsDialog::reset()
{
    mNumProcEdit->setText(QLocale().toString(MCMC_NUM_CHAINS_DEFAULT));
    mNumIterEdit->setText(QLocale().toString(MCMC_NUM_RUN_DEFAULT));
    mNumBurnEdit->setText(QLocale().toString(MCMC_NUM_BURN_DEFAULT));
    mMaxBatchesEdit->setText(QLocale().toString(MCMC_MAX_ADAPT_BATCHES_DEFAULT));
    mIterPerBatchEdit->setText(QLocale().toString(MCMC_ITER_PER_BATCH_DEFAULT));
    mDownSamplingEdit->setText(QLocale().toString(MCMC_THINNING_INTERVAL_DEFAULT));
    mSeedsEdit->setText("");

    mLevelEdit->setText(QLocale().toString(MCMC_MIXING_DEFAULT));
}

void MCMCSettingsDialog::setQuickTest()
{
    mNumProcEdit->setText(QLocale().toString(1));

    mNumBurnEdit->setText(QLocale().toString(10));
    mMaxBatchesEdit->setText(QLocale().toString(10));
    mIterPerBatchEdit->setText(QLocale().toString(20));
    mNumIterEdit->setText(QLocale().toString(100));
    mDownSamplingEdit->setText(QLocale().toString(1));

    mLevelEdit->setText(QLocale().toString(MCMC_MIXING_DEFAULT));
}

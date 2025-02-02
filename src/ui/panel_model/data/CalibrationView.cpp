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

#include "CalibrationView.h"

#include "Date.h"
#include "PluginAbstract.h"
#include "GraphViewRefAbstract.h"
#include "MainWindow.h"
#include "GraphView.h"
#include "LineEdit.h"
#include "Button.h"
#include "StdUtilities.h"
#include "Painting.h"
#include "Label.h"
#include "QtUtilities.h"
#include "DoubleValidator.h"
#include "AppSettings.h"

#include "CalibrationDrawing.h"

#include <QtWidgets>
#include <QClipboard>

CalibrationView::CalibrationView(QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mRefGraphView(nullptr),
    mTminDisplay(-HUGE_VAL),
    mTmaxDisplay(HUGE_VAL),
    mMajorScale (100),
    mMinorScale (4)
{
    setParent(parent);
    QPalette palette( parent->palette());

    QPalette palette_BW;
    palette_BW.setColor(QPalette::Base, Qt::white);
    palette_BW.setColor(QPalette::Text, Qt::black);
    palette_BW.setColor(QPalette::Window, Qt::white);
    palette_BW.setColor(QPalette::WindowText, Qt::black);

    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);

    mDrawing = new CalibrationDrawing(this);
    mDrawing->setMouseTracking(true);
    setMouseTracking(true);

    mImageSaveBut = new Button(tr("Save"), this);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));
    mImageSaveBut->setIconOnly(true);

    mImageClipBut = new Button(tr("Copy"), this);
    mImageClipBut->setIcon(QIcon(":clipboard_graph.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));
    mImageClipBut->setIconOnly(true);

    mResultsClipBut = new Button(tr("Copy"), this);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));
    mResultsClipBut->setIconOnly(true);

    mHPDLab = new Label(tr("HPD (%)"), this);
    mHPDLab->setAlignment(Qt::AlignHCenter);
    mHPDLab->setAdjustText();
    mHPDLab->setLight();
    mHPDLab->setBackground(Painting::borderDark);

    mHPDEdit = new LineEdit(this);
    mHPDEdit->setAdjustText();
    mHPDEdit->setText("95");

    DoubleValidator* percentValidator = new DoubleValidator(this);
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    percentValidator->setDecimals(3);
    mHPDEdit->setValidator(percentValidator);

    mStartLab = new Label(tr("Start"), this);
    mStartLab->setAdjustText();
    mStartLab->setAlignment(Qt::AlignHCenter);
    mStartLab->setLight();
    mStartLab->setBackground(Painting::borderDark);

    mStartEdit = new LineEdit(this);
    mStartEdit->setAdjustText();
    mStartEdit->setText("-1000");

    mEndLab = new Label(tr("End"), this);
    mEndLab->setAdjustText();
    mEndLab->setAlignment(Qt::AlignHCenter);
    mEndLab->setLight();
    mEndLab->setBackground(Painting::borderDark);

    mEndEdit = new LineEdit(this);
    mEndEdit->setAdjustText();
    mEndEdit->setText("1000");

    mDisplayStudyBut = new QPushButton(tr("Study Period Display"), this);
    mDisplayStudyBut->setPalette(palette_BW);
    //mDisplayStudyBut->setStyleSheet("QPushButton { border : 2px; border-radius: 4px; color: black; background-color: white;};//border-radius: 3px; }");
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));
    mDisplayStudyBut->setMinimumWidth(fontMetrics().horizontalAdvance(mDisplayStudyBut->text()) + 10);

    mMajorScaleLab = new Label(tr("Maj. Int"), this);
    mMajorScaleLab->setAdjustText();
    mMajorScaleLab->setAlignment(Qt::AlignHCenter);
    mMajorScaleLab->setLight();
    mMajorScaleLab->setBackground(Painting::borderDark);

    mMajorScaleEdit = new LineEdit(this);
    mMajorScaleEdit->setAdjustText();
    mMajorScaleEdit->setToolTip(tr("Enter a interval for the main division of the axes under the curves, upper than 1"));
    mMajorScaleEdit->setText(locale().toString(mMajorScale));

    mMinorScaleLab = new Label(tr("Min. Cnt"), this);
    mMinorScaleLab->setAdjustText();
    mMinorScaleLab->setAlignment(Qt::AlignHCenter);
    mMinorScaleLab->setLight();
    mMinorScaleLab->setBackground(Painting::borderDark);

    mMinorScaleEdit = new LineEdit(this);
    mMinorScaleEdit->setAdjustText();
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves, upper than 1"));
    mMinorScaleEdit->setText(locale().toString(mMinorScale));

    mHPDLab->raise();
    mHPDEdit->raise();

    //-------- DrawingView

    mCalibGraph = new GraphView(mDrawing);

    //mCalibGraph->setRendering(GraphView::eHD);

    mCalibGraph->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
    mCalibGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

    mCalibGraph->setXAxisMode(GraphView::eAllTicks);
    mCalibGraph->setYAxisMode(GraphView::eHidden);
    mCalibGraph->setMouseTracking(true);

    mResultsText = new QTextEdit(this);
    mResultsText->setFrameStyle(QFrame::NoFrame);

    mResultsText->setPalette(palette);
    mResultsText->setText(tr("No Result to display"));
    mResultsText->setReadOnly(true);

    // Connection
    // setText doesn't emit signal textEdited, when the text is changed programmatically

    connect(mStartEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScroll);
    connect(mEndEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScroll);

    connect(mDisplayStudyBut, &QPushButton::clicked, this, &CalibrationView::applyStudyPeriod);

    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScaleX);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScaleX);

    connect(mHPDEdit, &QLineEdit::textEdited, this, &CalibrationView::updateGraphs);
    connect(mImageSaveBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &CalibrationView::exportImage);
    connect(mResultsClipBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &CalibrationView::copyText);
    connect(mImageClipBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &CalibrationView::copyImage);

}

CalibrationView::~CalibrationView()
{
    mRefGraphView = nullptr;
}

void CalibrationView::applyAppSettings()
{
    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);

    if ( isVisible() && width()> 0 ) {
        updateGraphs();
        repaint();
    }
}

void CalibrationView::applyStudyPeriod()
{
    mStartEdit->setText(QString::number(mSettings.getTminFormated()));
    mEndEdit->setText(QString::number(mSettings.getTmaxFormated()));
    updateScroll();
}

void CalibrationView::setDate(const QJsonObject& date)
{
    if (date.isEmpty())
        return;

    Date d (date);
    setDate(d);
}

void CalibrationView::setDate(const Date& d)
{
    const QJsonObject &state = MainWindow::getInstance()->getState();
    const QJsonObject &settings = state.value(STATE_SETTINGS).toObject();
    mSettings = StudyPeriodSettings::fromJson(settings);

    if (mRefGraphView) {
         if (mDate.mPlugin)
                    mDate.mPlugin->deleteGraphViewRef(mRefGraphView);
        mRefGraphView = nullptr;
    }

    try {
        mDate = d;

        mDrawing->setTitle(mDate.getQStringName() + " (" + mDate.mPlugin->getName() + ")");
        qDebug() << "[CalibrationView::setDate] mUUID " << mDate.getQStringName() << mDate.mUUID;
        const double t1 = mSettings.getTminFormated();
        const double t2 = mSettings.getTmaxFormated();
        if (mDate.mIsValid && !mDate.mCalibration->mMap.empty()) {
            const double t3 = mDate.getFormatedTminCalib();
            const double t4 = mDate.getFormatedTmaxCalib();

            mTminDisplay = qMin(t1, t3);
            mTmaxDisplay = qMax(t2, t4);
        } else {
            mTminDisplay = t1;
            mTmaxDisplay = t2;
        }

        // setText doesn't emit signal textEdited, when the text is changed programmatically
        mStartEdit->setText(stringForGraph(mTminDisplay));
        mEndEdit->setText(stringForGraph(mTmaxDisplay));

        if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay))
            throw(tr("CalibrationView::setDate ") + mDate.mPlugin->getName() + mDate.mCalibration->getQStringName() + locale().toString(mDate.mCalibration->mTmin) + locale().toString(mDate.mCalibration->mTmax));

        updateScroll();

        mDrawing->setRefGraph( mRefGraphView);
        mDrawing->updateLayout();
        update();
    }
    catch(QString error) {
        QMessageBox message(QMessageBox::Critical,
                            qApp->applicationName() + " " + qApp->applicationVersion(),
                            tr("Error : %1").arg(error),
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();
    }

}

void CalibrationView::updateGraphs()
{
    mCalibGraph->removeAllCurves();
    mCalibGraph->remove_all_zones();

    if (!mDate.isNull()) {
        mCalibGraph->setRangeX (mTminDisplay, mTmaxDisplay);
        mCalibGraph->setCurrentX (mTminDisplay, mTmaxDisplay);
        mCalibGraph->changeXScaleDivision (mMajorScale, mMinorScale);

        // ------------------------------------------------------------
        //  Show zones if calibrated data are outside study period
        // ------------------------------------------------------------
        if (mTminDisplay < mSettings.getTminFormated()) {
            GraphZone zone;
            zone.mXStart = mTminDisplay;
            zone.mXEnd = mSettings.getTminFormated();
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(75);
            zone.mText = tr("Outside study period");
            mCalibGraph->add_zone(zone);
        }
        if (mTmaxDisplay > mSettings.getTmaxFormated()) {
            GraphZone zone;
            zone.mXStart = mSettings.getTmaxFormated();
            zone.mXEnd = mTmaxDisplay;
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(75);
            zone.mText = tr("Outside study period");
            mCalibGraph->add_zone(zone);
        }

        /* ------------------------------------------------------------
          Calibration curve
          ------------------------------------------------------------ */
        QColor penColor = Painting::mainColorDark;
        QColor brushColor = Painting::mainColorLight;
        brushColor.setAlpha(100);

        // Fill under distrib. of calibrated date only if Unif-typo :
        //const bool isUnif (mDate.mPlugin->getName() == "Unif");

        // Fill HPD only if not Unif :
        mResultsText->clear();

        if (mDate.mIsValid) {
            const std::map<double, double> &calibToShow = mDate.getFormatedCalibToShow();
            GraphCurve calibCurve = densityCurve(calibToShow, "Calibration", penColor);
            calibCurve.mVisible = true;
            QFontMetrics fm (mCalibGraph->font());

            mCalibGraph->add_curve(calibCurve);
            mCalibGraph->setMarginBottom(fm.ascent() * 2.2);

            GraphCurve calibWiggleCurve;

            if (mDate.mDeltaType != Date::eDeltaNone) {
                const std::map<double, double> &wiggleCalibMap =  mDate.getFormatedWiggleCalibToShow();
                const std::map<double, double> &calibWiggle = normalize_map(wiggleCalibMap, map_max(calibCurve.mData).value());
                calibWiggleCurve = densityCurve(calibWiggle, "Wiggle", Qt::blue, Qt::DashLine, Qt::NoBrush);
                calibWiggleCurve.mVisible = true;
                mCalibGraph->add_curve(calibWiggleCurve);
            }

            QString input = mHPDEdit->text();
            mHPDEdit->validator()->fixup(input);
            mHPDEdit->setText(input);

            // hpd results
            std::map<type_data, type_data> periodCalib = getMapDataInRange(mDate.getFormatedCalibMap(), mSettings.getTminFormated(), mSettings.getTmaxFormated());
            periodCalib = equal_areas(periodCalib, 1.);
            std::map<double, double> hpd;
            QList<QPair<double, QPair<double, double> > > formated_intervals;
            const double thresh = std::clamp(locale().toDouble(input), 0., 100.);

            if (thresh>(100. - 2*mDate.threshold_limit)) {
                // In the case of uniform, or calibrations with a length of less than 10, there is no reduction in support for the calibrate (see Date::calibrate).
                hpd = getMapDataInRange(mDate.getFormatedCalibMap(), mSettings.getTminFormated(), mSettings.getTmaxFormated());

                if (periodCalib.size()<10)
                    formated_intervals.append(qMakePair(1., qMakePair(periodCalib.cbegin()->first, periodCalib.crbegin()->first ) ));
                else
                    formated_intervals.append(qMakePair(1.- 2*mDate.threshold_limit, qMakePair(periodCalib.cbegin()->first, periodCalib.crbegin()->first ) ));

            } else {
                // do QMap<type_data, type_data> mData; to calcul HPD on study Period
                hpd = std::map<double, double>(create_HPD_by_dichotomy(periodCalib, formated_intervals, thresh));

            }


            if (!hpd.empty()) {
                GraphCurve hpdCurve;
                hpdCurve.mName = "Calibration HPD";
                hpdCurve.mPen = brushColor;
                hpdCurve.mBrush = brushColor;
                hpdCurve.mIsRectFromZero = true;

                // update max inside the display period
                type_data yMax = 0;
                QMap<type_data, type_data> displayCalib = getMapDataInRange(calibCurve.mData, mTminDisplay, mTmaxDisplay);
                if (!displayCalib.isEmpty()) {

                    yMax = map_max(displayCalib).value();

                    if (mDate.mDeltaType != Date::eDeltaNone) {
                        yMax = std::max( yMax, map_max(calibWiggleCurve.mData, mTminDisplay, mTmaxDisplay).value());
                    }

                    mCalibGraph->setRangeY(0., yMax);
                }

                const std::map<type_data, type_data> &displayHpd = getMapDataInRange(hpd, mTminDisplay, mTmaxDisplay);

                hpdCurve.mData = QMap(normalize_map(displayHpd, yMax));
                hpdCurve.mVisible = true;
                mCalibGraph->add_curve(hpdCurve);


                mCalibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
                mCalibGraph->setFormatFunctX(nullptr);
                mCalibGraph->setFormatFunctY(nullptr);
             }
            // ------------------------------------------------------------
            //  Display numerical results
            // ------------------------------------------------------------
            QString resultsStr;

            if (!periodCalib.empty()) {
                DensityAnalysis results;
                results.funcAnalysis = analyseFunction(periodCalib);
                resultsStr += FunctionStatToString(results.funcAnalysis);

                const double real_thresh = std::accumulate(formated_intervals.begin(), formated_intervals.end(), 0., [](double sum, QPair<double, QPair<double, double> > p) {return sum + p.first;});

                // formated_intervals is already in the right date format
                resultsStr += "<br> HPD (" + stringForLocal(real_thresh * 100.) + "%) : " + get_HPD_text(formated_intervals, DateUtils::getAppSettingsFormatStr(), nullptr, false) ;
                //resultsStr += "<br> Calibrated date step : " + stringForLocal(std::abs(hpd.lastKey() - hpd.firstKey())/hpd.size());
                mResultsText->setHtml(resultsStr);
            }

        } else {
            GraphZone zone;
            zone.mXStart = mSettings.getTminFormated();
            zone.mXEnd = mSettings.getTmaxFormated();
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Individual calibration not digitally computable ...");
            mCalibGraph->add_zone(zone);
        }

        mDrawing->setCalibGraph(mCalibGraph);
        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------

        // Get the ref graph for this plugin and this date
        if (!mRefGraphView)
                mRefGraphView = mDate.mPlugin->getGraphViewRef();
            

        if (mRefGraphView) {
            mRefGraphView->setParent(mDrawing);
            mRefGraphView->setVisible(true);

            mRefGraphView->setFormatFunctX(DateUtils::convertFromAppSettingsFormat); // must be before setDate, because setDate use it
            StudyPeriodSettings tmpSettings;
            const double maxDisplayInRaw = DateUtils::convertFromAppSettingsFormat(mTmaxDisplay);
            const double minDisplayInRaw = DateUtils::convertFromAppSettingsFormat(mTminDisplay);
            tmpSettings.mTmax = qMax(minDisplayInRaw, maxDisplayInRaw);
            tmpSettings.mTmin = qMin(minDisplayInRaw, maxDisplayInRaw);
            tmpSettings.mStep = 1.;
            mRefGraphView->setDate(mDate, tmpSettings);

            mRefGraphView->zoomX(mTminDisplay, mTmaxDisplay);
            mRefGraphView->changeXScaleDivision(mMajorScale, mMinorScale);

        }
        mDrawing->setRefGraph(mRefGraphView);

        // ------------------------------------------------------------
        //  Labels
        // ------------------------------------------------------------
        if (mRefGraphView) {
            if (mDate.mOrigin == Date::eSingleDate) {
                mDrawing->setRefTitle(tr("Calibration process"));
                mDrawing->setRefComment(mDate.mPlugin->getDateDesc(&mDate));

            } else {
                mDrawing->setRefTitle(tr("Overlay densities"));
                QList<QString> subDatesName;
                for (auto&& d : mDate.mSubDates) {
                    if (d.toObject().value(STATE_DATE_DELTA_TYPE).toInt() == Date::eDeltaNone)
                        subDatesName.append(d.toObject().value(STATE_NAME).toString());
                    else
                        subDatesName.append(d.toObject().value(STATE_NAME).toString() + " " + Date::getWiggleDesc(d.toObject()));
                }

                mDrawing->setRefComment(subDatesName.join(" ; "));
            }

            mDrawing->setCalibTitle(tr("Distribution of calibrated date"));
            mDrawing->setCalibComment("HPD = " + mHPDEdit->text() + " %");

        } else {
            mDrawing->setRefTitle(tr("No calibration process to display"));
            mDrawing->setRefComment(mDate.mPlugin->getDateDesc(&mDate));
            mDrawing->setCalibTitle(tr("Uniforme date"));
            mDrawing->setCalibComment("HPD = " + mHPDEdit->text() + " %");
        }
        mDrawing->updateLayout();

    }
    updateLayout();
}

void CalibrationView::updateZoom()
{
    updateScroll();
}

void CalibrationView::updateScaleX()
{
    QString str = mMajorScaleEdit->text();
    bool isNumber = true;
    double aNumber = locale().toDouble(str, &isNumber);

    qreal textSize = fontMetrics().horizontalAdvance(str)  + fontMetrics().horizontalAdvance("0");
    if (textSize > mMajorScaleEdit->width()) {
        QFont adaptedFont (font());
        const qreal fontRate = textSize / mMajorScaleEdit->width();
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mMajorScaleEdit->setFont(adaptedFont);

    } else
        mMajorScaleEdit->setFont(font());


    if (!isNumber || aNumber < 1)
        return;

    mMajorScale = aNumber;

    //---------------------------------

    str = mMinorScaleEdit->text();

    textSize = fontMetrics().horizontalAdvance(str)  + fontMetrics().horizontalAdvance("0");
    if (textSize > mMinorScaleEdit->width()) {
        QFont adaptedFont (font());
        const qreal fontRate = textSize / mMinorScaleEdit->width();
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mMinorScaleEdit->setFont(adaptedFont);

    } else
        mMinorScaleEdit->setFont(font());

    aNumber = locale().toDouble(str, &isNumber);

    if (isNumber && aNumber >= 1) {
        mMinorScale =  int (aNumber);
        mCalibGraph->changeXScaleDivision(mMajorScale, mMinorScale);
        if (mRefGraphView)
            mRefGraphView->changeXScaleDivision(mMajorScale, mMinorScale);
    }

}

void CalibrationView::updateScroll()
{
    bool ok;

    double val = locale().toDouble(mStartEdit->text(), &ok);
    if (ok)
        mTminDisplay = val;
    else
        mTminDisplay = mSettings.getTminFormated();

    val = locale().toDouble(mEndEdit->text(), &ok);
    if (ok)
        mTmaxDisplay = val;
    else
        mTmaxDisplay = mSettings.getTmaxFormated();

    qDebug()<<"[CalibrationView::updateScroll] "<< mTminDisplay << mTmaxDisplay;
        // usefull when we set mStartEdit and mEndEdit at the begin of the display,
        // after a call to setDate
        if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay) || mTminDisplay>mTmaxDisplay )
            return;


    qreal textSize = fontMetrics().horizontalAdvance(mStartEdit->text())  + fontMetrics().horizontalAdvance("0");
    if (textSize > mStartEdit->width() ) {
        QFont adaptedFont (font());
        const qreal fontRate = textSize / mStartEdit->width() ;
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mStartEdit->setFont(adaptedFont);
    }
    else
        mStartEdit->setFont(font());

    textSize = fontMetrics().horizontalAdvance(mEndEdit->text()) + fontMetrics().horizontalAdvance("0");
    if (textSize > mEndEdit->width() ) {
        QFont adaptedFont (font());
        const qreal fontRate = textSize / mEndEdit->width();
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mEndEdit->setFont(adaptedFont);
    }
    else
        mEndEdit->setFont(font());

    updateGraphs();

}

void CalibrationView::exportImage()
{
    mDrawing->hideMarker();

    QFileInfo fileInfo = saveWidgetAsImage(mDrawing, mDrawing->rect(), tr("Save calibration image as..."),
                                           MainWindow::getInstance()->getCurrentPath());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());

    mDrawing->showMarker();

}

void CalibrationView::copyImage()
{
    mDrawing->hideMarker();
    repaint();
    QApplication::clipboard()->setPixmap(mDrawing->grab());
    mDrawing->showMarker();
}

void CalibrationView::copyText()
{
    QString text = mDate.getQStringName() + " (" + mDate.mPlugin->getName() + ")" +"<br>" + mDate.getDesc() + "<br>" + mResultsText->toPlainText();
    QApplication::clipboard()->setText(text.replace("<br>", "\r"));
}

void CalibrationView::setVisible(bool visible)
{
    mImageSaveBut->setVisible(visible);
    mImageClipBut->setVisible(visible);
    mResultsClipBut->setVisible(visible);

    mStartLab->setVisible(visible);
    mStartEdit->setVisible(visible);
    mEndLab->setVisible(visible);
    mEndEdit->setVisible(visible);

    mHPDLab->setVisible(visible);
    mHPDEdit->setVisible(visible);

    mDrawing->setVisible(visible);
    mResultsText->setVisible(visible);
    QWidget::setVisible(visible);
}

void CalibrationView::resizeEvent(QResizeEvent*)
{
    if (!mDate.mPlugin || width()<0 || height()<0) {
        setVisible(false);
        return;

    } else
        setVisible(true);

    update();

}

void CalibrationView::paintEvent(QPaintEvent* )
{
    const int graphWidth = width() - mButtonWidth;

    QPainter p(this);
    // 1 - Drawing a background under button to hide buttonn of Even Scene
    p.fillRect(0, 0, mButtonWidth, height(), Painting::borderDark);

    //  2 - Behind toolBar
    p.fillRect(mButtonWidth, mDrawing->y() + mDrawing->height(), graphWidth, mResultsText->y() - mDrawing->height() , Painting::borderDark);

    // 3 - Drawing a background under curve
    p.fillRect(mButtonWidth, 0, graphWidth, mDrawing->height(),  Qt::white);

    // 4 - Behind mResultText, mResultsText is less wide than the remaining space to put a visible space to the left of the text
    p.fillRect(mButtonWidth, mResultsText->y(), graphWidth, height() - mResultsText->y() , mResultsText->palette().base().color());
    updateLayout();

}

void CalibrationView::updateLayout()
{
    if (!mCalibGraph->has_curves()) {
        mDrawing->setGeometry(mButtonWidth, 0, 0, 0);
        return;
    }

    QFontMetrics fm (font());

    // Same variable in MultiCalibrationView::updateLayout()
    const int textHeight (int (1.2 * (fm.descent() + fm.ascent()) ));
    const qreal toolBarHeigth = 2*textHeight + 12;
    //Position of Widget
    int y = 0;

    mImageSaveBut->setGeometry(0, y, mButtonWidth, mButtonHeigth);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(0, y, mButtonWidth, mButtonHeigth);

    const int graphLeft = mImageSaveBut->x() + mImageSaveBut->width();
    const int graphWidth = width() - graphLeft;

    const int resTextH = 5 * fm.height();

    mDrawing->setGeometry(graphLeft, 0, graphWidth, height() - resTextH - toolBarHeigth);

    // Bottom toolBar
    const int yPosBottomBar0 = mDrawing->y() + mDrawing->height();
    const int yPosBottomBar1 = yPosBottomBar0 + textHeight + 2;

    const int buttonWidth =   mDisplayStudyBut->width();

    const int labelWidth = std::min( fontMetrics().horizontalAdvance("-1000000") , (graphWidth - buttonWidth) /5);
    const int editWidth = labelWidth;
    const int marginBottomBar = (graphWidth- 5.*labelWidth - buttonWidth)/7.;


    qreal xShift = marginBottomBar + graphLeft;
    mStartLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mStartEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    xShift += mStartLab->width() + marginBottomBar;
    mEndLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mEndEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    xShift += mEndLab->width() + marginBottomBar;
    mDisplayStudyBut->setGeometry(xShift, yPosBottomBar1 - 5, mDisplayStudyBut->width(), textHeight + 10);


    xShift += mDisplayStudyBut->width() + marginBottomBar;
    mMajorScaleLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mMajorScaleEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    xShift += mMajorScaleLab->width() + marginBottomBar;
    mMinorScaleLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mMinorScaleEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    xShift += mMinorScaleLab->width() + marginBottomBar;
    mHPDLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mHPDEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    // ResultBox
    mResultsClipBut->setGeometry(0, mDrawing->y() + mDrawing->height() + toolBarHeigth, mButtonWidth, mButtonHeigth);

    mResultsText->setGeometry(graphLeft + 20, mDrawing->y() + mDrawing->height() + toolBarHeigth, graphWidth - 40 , resTextH);
    mResultsText->setAutoFillBackground (true);

}

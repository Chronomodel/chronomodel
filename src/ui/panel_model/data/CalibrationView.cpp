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

#include "CalibrationView.h"

#include <QtWidgets>
#include <QClipboard>

#include "Ruler.h"
#include "Date.h"
#include "Event.h"
#include "Marker.h"
#include "../PluginAbstract.h"
#include "../GraphViewRefAbstract.h"
#include "MainWindow.h"
#include "Project.h"
#include "GraphView.h"
#include "CheckBox.h"
#include "LineEdit.h"
#include "Button.h"
#include "StdUtilities.h"
#include "Painting.h"
#include "Label.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "DoubleValidator.h"

#include "CalibrationDrawing.h"

CalibrationView::CalibrationView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
    mRefGraphView(nullptr),
    mTminDisplay(-HUGE_VAL),
    mTmaxDisplay(HUGE_VAL),
    mMajorScale (100),
    mMinorScale (4)
{

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

    frameSeparator = new QFrame(this);
    frameSeparator->setFrameStyle(QFrame::Panel);
    frameSeparator->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    mHPDLab = new Label(tr("HPD (%)"), this);
    mHPDLab->setAlignment(Qt::AlignHCenter);
    mHPDLab->setAdjustText();
    mHPDLab->setLight();
    mHPDLab->setBackground(Painting::borderDark);

    mHPDEdit = new LineEdit(this);
    mHPDEdit->setAdjustText();
    mHPDEdit->setText("95");

    DoubleValidator* percentValidator = new DoubleValidator();
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
    mCalibGraph->setYAxisMode(GraphView::eHidden);
    mCalibGraph->setXAxisMode(GraphView::eAllTicks);
    mCalibGraph->setMouseTracking(true);

    mResultsText = new QTextEdit(this);
    mResultsText->setFrameStyle(QFrame::HLine);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);

    mResultsText->setPalette(palette);
    mResultsText->setText(tr("No Result to display"));
    mResultsText->setReadOnly(true);


    // Connection
    // setText doesn't emit signal textEdited, when the text is changed programmatically

    connect(mStartEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScroll);
    connect(mEndEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScroll);
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScaleX);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScaleX);

    connect(mHPDEdit, &QLineEdit::textEdited, this, &CalibrationView::updateGraphs);
    connect(mImageSaveBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &CalibrationView::exportImage);
    connect(mResultsClipBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &CalibrationView::copyText);
    connect(mImageClipBut, static_cast<void (Button::*)(bool)> (&Button::clicked), this, &CalibrationView::copyImage);

    setVisible(false);
}

CalibrationView::~CalibrationView()
{
    mRefGraphView = nullptr;
}

void CalibrationView::applyAppSettings()
{
    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);

    repaint();
}

void CalibrationView::setDate(const QJsonObject& date)
{
    Q_ASSERT(&date);
    if (date.isEmpty())
        return;

    Date d (date);
    setDate(d);
}

void CalibrationView::setDate(const Date& d)
{
    //Q_ASSERT(&d);

    Project* project = MainWindow::getInstance()->getProject();
    const QJsonObject state = project->state();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();
    mSettings = ProjectSettings::fromJson(settings);

    if (mRefGraphView) {
         if (mDate.mPlugin)
                    mDate.mPlugin->deleteGraphViewRef(mRefGraphView);
        mRefGraphView = nullptr;
    }

    try {
        mDate = d;

        mDrawing->setTitle(mDate.mName + " (" + mDate.mPlugin->getName() + ")");
        qDebug() << "CalibrationView::setDate mUUID "<<mDate.mName << mDate.mUUID;
        const double t1 = mSettings.getTminFormated();
        const double t2 = mSettings.getTmaxFormated();
        if (mDate.mIsValid) {
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
            throw(tr("CalibrationView::setDate ") + mDate.mPlugin->getName() + mDate.mCalibration->mName + locale().toString(mDate.mCalibration->mTmin) + locale().toString(mDate.mCalibration->mTmax));

        updateScroll();

    }
    catch(QString error) {
        QMessageBox message(QMessageBox::Critical,
                            qApp->applicationName() + " " + qApp->applicationVersion(),
                            tr("Error : %1").arg(error),
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();
    }
    mDrawing->setRefGraph( mRefGraphView);
    mDrawing->updateLayout();
    update();
}

void CalibrationView::updateGraphs()
{
    mCalibGraph->removeAllCurves();
    mCalibGraph->removeAllZones();

     if (!mDate.isNull() ) {
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
            mCalibGraph->addZone(zone);
        }
        if (mTmaxDisplay > mSettings.getTmaxFormated()) {
            GraphZone zone;
            zone.mXStart = mSettings.getTmaxFormated();
            zone.mXEnd = mTmaxDisplay;
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(75);
            zone.mText = tr("Outside study period");
            mCalibGraph->addZone(zone);
        }

        /* ------------------------------------------------------------
          Calibration curve
          ------------------------------------------------------------ */
        QColor penColor = Painting::mainColorDark;
        QColor brushColor = Painting::mainColorLight;
        brushColor.setAlpha(100);

        // Fill under distrib. of calibrated date only if Unif-typo :
        const bool isUnif (mDate.mPlugin->getName() == "Unif");

        // Fill HPD only if not Unif :
        mResultsText->clear();

        QMap<double, double> calibMap;
        if (mDate.mIsValid)
            calibMap = mDate.getFormatedCalibMap();
         
        QMap<double, double> wiggleCalibMap;
        if (mDate.mDeltaType != Date::eDeltaNone)
             wiggleCalibMap =  mDate.getFormatedWiggleCalibMap();


        if (!calibMap.isEmpty()) {

            GraphCurve calibCurve;
            calibCurve.mName = "Calibration";
            calibCurve.mPen.setColor(penColor);
            calibCurve.mIsHisto = false;
            calibCurve.mData = calibMap;
            calibCurve.mIsRectFromZero = isUnif;
            calibCurve.mBrush = isUnif ? QBrush(brushColor) : QBrush(Qt::NoBrush);
            QFontMetrics fm (mCalibGraph->font());
            mCalibGraph->addCurve(calibCurve);
            mCalibGraph->setMarginBottom(fm.ascent() * 2.2);

            GraphCurve calibWiggleCurve;
            if (!wiggleCalibMap.isEmpty()) { 
                calibWiggleCurve.mName = "Wiggle";
                calibWiggleCurve.mPen.setColor(Qt::red);
                calibWiggleCurve.mIsHisto = false;
                calibWiggleCurve.mData = normalize_map(wiggleCalibMap, map_max_value(calibMap));;
                calibWiggleCurve.mIsRectFromZero = false;
                calibWiggleCurve.mBrush = QBrush(Qt::NoBrush);
                mCalibGraph->addCurve(calibWiggleCurve);
            }
            
            
         //   mCalibGraph->setTipXLab("t"); // don't work

            QString input = mHPDEdit->text();
            mHPDEdit->validator()->fixup(input);
            mHPDEdit->setText(input);

            // hpd results
            const double thresh = qBound(0., locale().toDouble(input), 100.);
            // do QMap<type_data, type_data> mData; to calcul HPD on study Period
            QMap<type_data, type_data> subData = calibCurve.mData;
            subData = getMapDataInRange(subData, mSettings.getTminFormated(), mSettings.getTmaxFormated());


            QMap<type_data, type_data> hpd (create_HPD(subData, thresh));


            GraphCurve hpdCurve;
            hpdCurve.mName = "Calibration HPD";
            hpdCurve.mPen = brushColor;
            hpdCurve.mBrush = brushColor;
            hpdCurve.mIsHisto = false;
            hpdCurve.mIsRectFromZero = true;
            hpdCurve.mData = hpd;
            mCalibGraph->addCurve(hpdCurve);

            // update max inside the display period
            QMap<type_data, type_data> subDisplayCalib = calibCurve.mData;
            subDisplayCalib = getMapDataInRange(subDisplayCalib, mTminDisplay, mTmaxDisplay);
            type_data yMax = map_max_value(subDisplayCalib);

            if (!wiggleCalibMap.isEmpty()) {
                QMap<type_data, type_data> subDisplayWiggle = getMapDataInRange(calibWiggleCurve.mData, mTminDisplay, mTmaxDisplay);
                yMax = std::max( yMax, map_max_value(subDisplayWiggle));
            }


            mCalibGraph->setRangeY(0., yMax);

            mCalibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mCalibGraph->setFormatFunctX(nullptr);
            mCalibGraph->setFormatFunctY(nullptr);

            // ------------------------------------------------------------
            //  Display numerical results
            // ------------------------------------------------------------
            QString resultsStr;

            DensityAnalysis results;
            results.funcAnalysis = analyseFunction(subData);


            if (!subData.isEmpty()) {

                resultsStr += FunctionStatToString(results.funcAnalysis);

                /* with the calibration we don't need the statistic on the trace*/
                /*
                 * QVector<double> subRepart = calculRepartition(subData);
                 * results.quartiles = quartilesForRepartition(subRepart, subData.firstKey(), mSettings.mStep);
                 *  results.xmin = subData.firstKey(); // useless
                 *  results.xmax = subData.lastKey();
                 *  resultsStr += densityAnalysisToString(results,"<br>", false);
               */

             }

            const double realThresh = map_area(hpd) / map_area(subData);

            resultsStr +=  "<br> HPD (" + stringForLocal(100. * realThresh) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat, false) ;

            mResultsText->setText(resultsStr);

        } else {
            GraphZone zone;
            zone.mXStart = mSettings.getTminFormated();
            zone.mXEnd = mSettings.getTmaxFormated();
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Individual calibration not digitally computable ...");
            mCalibGraph->addZone(zone);
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
            ProjectSettings tmpSettings;
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
                        subDatesName.append(d.toObject().value(STATE_NAME).toString() + " " +Date::getWiggleDesc(d.toObject()));
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

    QFont adaptedFont (font());

    qreal textSize = fontMetrics().boundingRect(str).width()  + fontMetrics().boundingRect("0").width();
    if (textSize > mMajorScaleEdit->width()) {
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

    adaptedFont = font();

    textSize = fontMetrics().boundingRect(str).width()  + fontMetrics().boundingRect("0").width();
    if (textSize > mMinorScaleEdit->width()) {
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

    qDebug()<<"CalibrationView::updateScroll()"<<mTminDisplay<<mTmaxDisplay;
        // usefull when we set mStartEdit and mEndEdit at the begin of the display,
        // after a call to setDate
        if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay) || mTminDisplay>mTmaxDisplay )
            return;

    QFont adaptedFont (font());
    qreal textSize = fontMetrics().boundingRect(mStartEdit->text()).width()  + fontMetrics().boundingRect("0").width();
    if (textSize > mStartEdit->width() ) {
        const qreal fontRate = textSize / mStartEdit->width() ;
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mStartEdit->setFont(adaptedFont);
    }
    else
        mStartEdit->setFont(font());

    adaptedFont = font();
    textSize = fontMetrics().boundingRect(mEndEdit->text()).width() + fontMetrics().boundingRect("0").width();
    if (textSize > mEndEdit->width() ) {
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
    QString text = mDate.mName + " (" + mDate.mPlugin->getName() + ")" +"<br>" + mDate.getDesc() + "<br>" + mResultsText->toPlainText();
    QApplication::clipboard()->setText(text.replace("<br>", "\r"));

}

void CalibrationView::setVisible(bool visible)
{
    mImageSaveBut->setVisible(visible);
    mImageClipBut->setVisible(visible);
    mResultsClipBut->setVisible(visible);
    frameSeparator->setVisible(visible);

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

void CalibrationView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    if (!mDate.mPlugin || width()<0 || height()<0) {
        setVisible(false);
        return;

    } else
        setVisible(true);

    update();

}
void CalibrationView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    const int graphLeft = mButtonWidth;
    const int graphWidth = width() - graphLeft;

    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing); // not necessary
    // drawing a background under button
    p.fillRect(QRect(0, 0, graphLeft, height()), Painting::borderDark);

    // drawing a background under curve
    p.fillRect(QRect(graphLeft, 0, graphWidth, height()), Qt::white);

    updateLayout();

}

void CalibrationView::updateLayout()
{
    if (!mCalibGraph->hasCurve()) {
        mDrawing->setGeometry(mButtonWidth, 0, 0, 0);
        return;
    }

    QFontMetrics fm (font());

    // same variable in MultiCalibrationView::updateLayout()
    const int textHeight (int (1.2 * (fm.descent() + fm.ascent()) ));
    const int verticalSpacer (int (0.3 * AppSettings::heigthUnit()));
    const int margin = int (0.1 * mButtonWidth);

    //Position of Widget
    int y = 0;

    mImageSaveBut->setGeometry(0, y, mButtonWidth, mButtonHeigth);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(0, y, mButtonWidth, mButtonHeigth);
    y += mImageClipBut->height();
    mResultsClipBut->setGeometry(0, y, mButtonWidth, mButtonHeigth);
    y += mResultsClipBut->height();

    const int separatorHeight (height()- y - 10*textHeight - 10* verticalSpacer);
    frameSeparator->setGeometry(0, y, mButtonWidth, separatorHeight);
    y += frameSeparator->height() + verticalSpacer;

    mStartLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mStartLab->height();
    mStartEdit->setGeometry(margin, y, mButtonWidth - 2*margin, textHeight);
    y += mStartEdit->height() + verticalSpacer;
    mEndLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mEndLab->height();
    mEndEdit->setGeometry(margin, y, mButtonWidth - 2*margin, textHeight);
    y += mEndEdit->height() + verticalSpacer;

    mMajorScaleLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mMajorScaleLab->height();
    mMajorScaleEdit->setGeometry(margin, y, mButtonWidth- 2*margin, textHeight);
    y += mMajorScaleEdit->height() + verticalSpacer;

    mMinorScaleLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mMinorScaleLab->height();
    mMinorScaleEdit->setGeometry(margin, y, mButtonWidth - 2*margin, textHeight);
    y += mMinorScaleEdit->height() + 3*verticalSpacer;

    mHPDLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mHPDLab->height();
    mHPDEdit->setGeometry(margin, y, mButtonWidth - 2*margin, textHeight);

    const int graphLeft = mImageSaveBut->x() + mImageSaveBut->width();
    const int graphWidth = width() - graphLeft;

    const int resTextH = 5 * fm.height();
    mDrawing->setGeometry(graphLeft, 0, graphWidth, height() - resTextH);
    mResultsText->setGeometry(graphLeft + 20, mDrawing->y() + mDrawing->height(), graphWidth - 40 , resTextH);
    mResultsText->setAutoFillBackground (true);

}

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

#include "MultiCalibrationView.h"

#include "DoubleValidator.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "EventKnown.h"
#include "GraphViewResults.h"

#include <QPainter>
#include "Project.h"
#include <QJsonArray>

MultiCalibrationView::MultiCalibrationView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMajorScale (100),
mMinorScale (4),
mTminDisplay(-HUGE_VAL),
mTmaxDisplay(HUGE_VAL),
mThreshold(95),
mGraphHeight(GraphViewResults::mHeightForVisibleAxis),
mCurveColor(Painting::mainColorDark)
{
    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    setMouseTracking(true);
    mDrawing = new MultiCalibrationDrawing(this);
    setMouseTracking(true);

    mTextArea = new QTextEdit(this);
    mTextArea->setFrameStyle(QFrame::HLine);
    QPalette palette = mTextArea->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    mTextArea->setPalette(palette);
    //QFont font = mTextArea->font();
    //mTextArea->setFont(font);
    mTextArea->setText(tr("Nothing to display"));
    mTextArea->setVisible(false);
    mTextArea->setReadOnly(true);

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

    mStatClipBut = new Button(tr("Stat"), this);
    mStatClipBut->setIcon(QIcon(":stats_w.png"));
    mStatClipBut->setFlatVertical();
    mStatClipBut->setToolTip(tr("Show Stats for calibrated dates"));
    mStatClipBut->setIconOnly(true);
    mStatClipBut->setChecked(false);
    mStatClipBut->setCheckable(true);

    mGraphHeightLab = new Label(tr("Y Zoom"), this);
    mGraphHeightLab->setAdjustText();
    mGraphHeightLab->setAlignment(Qt::AlignHCenter);
    mGraphHeightLab->setLight();
    mGraphHeightLab->setBackground(Painting::borderDark);

    mGraphHeightEdit = new LineEdit(this);
    mGraphHeightEdit->setText(locale().toString(100));

    QIntValidator* heightValidator = new QIntValidator();
    heightValidator->setRange(50, 500);
    mGraphHeightEdit->setValidator(heightValidator);

    mColorClipBut = new Button(tr("Color"), this);
    mColorClipBut->setIcon(QIcon(":color.png"));
    mColorClipBut->setFlatVertical();
    mColorClipBut->setToolTip(tr("Set Personal Densities Color"));
    mColorClipBut->setIconOnly(true);

    frameSeparator = new QFrame(this);
    frameSeparator->setFrameStyle(QFrame::Panel);
    frameSeparator->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

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
    mMajorScaleEdit->setToolTip(tr("Enter a interval for the main division of the axes under the curves"));
    mMajorScaleEdit->setText(locale().toString(mMajorScale));

    mMinorScaleLab = new Label(tr("Min. Cnt"), this);
    mMinorScaleLab->setAdjustText();
    mMinorScaleLab->setAlignment(Qt::AlignHCenter);
    mMinorScaleLab->setLight();
    mMinorScaleLab->setBackground(Painting::borderDark);

    mMinorScaleEdit = new LineEdit(this);
    mMinorScaleEdit->setAdjustText();
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves"));
    mMinorScaleEdit->setText(locale().toString(mMinorScale));

    mHPDLab = new Label(tr("HPD (%)"), this);
    mHPDLab->setAdjustText();
    mHPDLab->setAlignment(Qt::AlignHCenter);
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

    mHPDLab->raise();
    mHPDEdit->raise();

    //-------- DrawingView



    // Connection
    // setText doesn't emit signal textEdited, when the text is changed programmatically

    connect(mStartEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &MultiCalibrationView::updateScroll);
    connect(mEndEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &MultiCalibrationView::updateScroll);
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &MultiCalibrationView::updateScaleX);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &MultiCalibrationView::updateScaleX);


    connect(mHPDEdit, &QLineEdit::textEdited, this, &MultiCalibrationView::updateHPDGraphs);
    connect(mImageSaveBut, &Button::clicked, this, &MultiCalibrationView::exportFullImage);
    connect(mImageClipBut, &Button::clicked, this, &MultiCalibrationView::copyImage);
    connect(mStatClipBut, &Button::clicked, this, &MultiCalibrationView::showStat);
    connect(mGraphHeightEdit, &QLineEdit::textEdited, this, &MultiCalibrationView::updateGraphsSize);
    connect(mColorClipBut, &Button::clicked, this, &MultiCalibrationView::changeCurveColor);

   // setVisible(false);
}

MultiCalibrationView::~MultiCalibrationView()
{

}

void MultiCalibrationView::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void MultiCalibrationView::paintEvent(QPaintEvent* e)
{
    (void) e;
    const int graphWidth (width() - mButtonWidth);

    QPainter p(this);
    // drawing a background under button
    p.fillRect(QRect(graphWidth, 0, mButtonWidth, height()), Painting::borderDark);

    // drawing a background under curve
   // p.fillRect(QRect(0, 0, graphWidth, height()), Qt::green);


}

void MultiCalibrationView::setVisible(bool visible)
{
    mImageSaveBut->setVisible(visible);
    mImageClipBut->setVisible(visible);
    mStatClipBut->setVisible(visible);
    frameSeparator->setVisible(visible);

    mStartLab->setVisible(visible);
    mStartEdit->setVisible(visible);
    mEndLab->setVisible(visible);
    mEndEdit->setVisible(visible);

    mHPDLab->setVisible(visible);
    mHPDEdit->setVisible(visible);

    QWidget::setVisible(visible);
}

void MultiCalibrationView::applyAppSettings()
{

    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);

    updateLayout();
}

void MultiCalibrationView::updateLayout()
{
    const int x0  (width() - mButtonWidth);
    const int margin ( int(0.1 * mButtonWidth));
    const int xm (x0 + margin);

    const int textHeight (int (1.2 * (fontMetrics().descent() + fontMetrics().ascent()) ));
    const int verticalSpacer (int (0.3 * AppSettings::heigthUnit()));
    mMarginRight = int (1.5 * floor(fontMetrics().boundingRect(mEndEdit->text()).width()/2) + 5);
    mMarginLeft = int (1.5 * floor(fontMetrics().boundingRect(mStartEdit->text()).width()/2) + 5);

    //Position of Widget
    int y (0);

    mImageSaveBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mImageClipBut->height();
    mStatClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mStatClipBut->height() + 5;

    mGraphHeightLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mGraphHeightLab->height();
    mGraphHeightEdit->setGeometry(xm, y, mButtonWidth - 2 * margin, textHeight);
    y += mGraphHeightEdit->height() + verticalSpacer;

    mColorClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mColorClipBut->height();

    const int separatorHeight (height() - y - 10* textHeight - 10*verticalSpacer);
    frameSeparator->setGeometry(x0, y, mButtonWidth, separatorHeight);
    y += frameSeparator->height() + verticalSpacer;

    mStartLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mStartLab->height();
    mStartEdit->setGeometry(xm, y, mButtonWidth  - 2 * margin, textHeight);
    y += mStartEdit->height() + verticalSpacer;
    mEndLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mEndLab->height();
    mEndEdit->setGeometry(xm, y, mButtonWidth - 2 * margin, textHeight);
    y += mEndEdit->height() + verticalSpacer;

    mMajorScaleLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mMajorScaleLab->height();
    mMajorScaleEdit->setGeometry(xm, y, mButtonWidth - 2 * margin, textHeight);
    y += mMajorScaleEdit->height() + verticalSpacer;
    mMajorScaleEdit->setText(locale().toString(mMajorScale));

    mMinorScaleLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mMinorScaleLab->height();
    mMinorScaleEdit->setGeometry(xm, y, mButtonWidth  - 2 * margin, textHeight);
    y += mMinorScaleEdit->height() + 3*verticalSpacer;
    mMinorScaleEdit->setText(locale().toString(mMinorScale));


    mHPDLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mHPDLab->height();
    mHPDEdit->setGeometry(xm, y, mButtonWidth - 2 * margin, textHeight);

    const int graphWidth = width() - mButtonWidth;

    if (mStatClipBut->isChecked())
        mTextArea->setGeometry(0, 0, graphWidth, height());

    else {
        mDrawing->setGeometry(0, 0, graphWidth, height());
        mDrawing->setGraphHeight(mGraphHeight);

        mDrawing->update();
    }

}

void MultiCalibrationView::updateGraphList()
{
    QJsonObject state = mProject->state();
    mSettings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());

    mTminDisplay = mSettings.getTminFormated() ;
    mTmaxDisplay = mSettings.getTmaxFormated();

    // setText doesn't emit signal textEdited, when the text is changed programmatically
    mStartEdit->setText(locale().toString(mTminDisplay));
    mEndEdit->setText(locale().toString(mTmaxDisplay));
    mHPDEdit->setText(locale().toString(mThreshold));
  //The same Name and same Value as in MultiCalibrationView::exportFullImage()
    mMarginRight = int (1.5 * floor(fontMetrics().boundingRect(mEndEdit->text()).width()/2) + 5);
    mMarginLeft = int (1.5 * floor(fontMetrics().boundingRect(mStartEdit->text()).width()/2) + 5);

    QColor penColor = mCurveColor;
    QColor brushColor = mCurveColor;
    brushColor.setAlpha(170);

    QList<GraphView*> graphList;
    QList<QColor> colorList;
    const QJsonArray events = state.value(STATE_EVENTS).toArray();




    QVector<QJsonObject> selectedEvents;

    for (auto&& ev : events) {
       QJsonObject jsonEv = ev.toObject();
       if (jsonEv.value(STATE_IS_SELECTED).toBool())
            selectedEvents.append(jsonEv);
    }


    // Sort Event by Position
    std::sort(selectedEvents.begin(), selectedEvents.end(), [] (QJsonObject ev1, QJsonObject ev2) {return (ev1.value(STATE_ITEM_Y).toDouble() < ev2.value(STATE_ITEM_Y).toDouble());});

    QString preEventName ="";

    for (auto&& ev : selectedEvents) {

        if (ev.value(STATE_EVENT_TYPE).toInt() == Event::eKnown) {

            EventKnown *bound = new EventKnown(ev);

            GraphCurve calibCurve;
            calibCurve.mName = "Bound";
            calibCurve.mPen.setColor(penColor);
            calibCurve.mPen.setWidth(20);
            calibCurve.mBrush = brushColor;
            calibCurve.mPen = QPen(Painting::mainColorLight, 2.);
            calibCurve.mIsHorizontalSections = true;

            double tFixedFormated = DateUtils::convertToAppSettingsFormat( bound->fixedValue());
            calibCurve.mSections.append(qMakePair(tFixedFormated, tFixedFormated));

            GraphView* calibGraph = new GraphView(this);
            QString boundName (ev.value(STATE_NAME).toString());

            calibGraph->addInfo(tr("Bound : %1").arg(boundName));
            calibGraph->showInfos(true);

            calibGraph->setRangeY(0., 1.);

            calibGraph->addCurve(calibCurve);


            calibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            calibGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            calibGraph->setFormatFunctY(nullptr);

            calibGraph->setMarginRight(mMarginRight);
            calibGraph->setMarginLeft(mMarginLeft);

            calibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
            calibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
            calibGraph->changeXScaleDivision(mMajorScale, mMinorScale);
            calibGraph->setOverArrow(GraphView::eNone);

           // calibGraph->setRendering(GraphView::eHD);
            graphList.append(calibGraph);

            QColor color = QColor(ev.value(STATE_COLOR_RED).toInt(),
                                  ev.value(STATE_COLOR_GREEN).toInt(),
                                  ev.value(STATE_COLOR_BLUE).toInt());
            colorList.append(color);
            delete bound;
            bound = nullptr;

        } else {
            const QJsonArray dates = ev.value(STATE_EVENT_DATES).toArray();

            for (auto&& date : dates) {

                Date d (date.toObject());
               // d.autoSetTiSampler(true);

                GraphCurve calibCurve;
                GraphView* calibGraph = new GraphView(this);

                 if (d.mIsValid && !d.mCalibration->mCurve.isEmpty()) {
                    calibCurve.mName = "Calibration";
                    calibCurve.mPen.setColor(penColor);
                    calibCurve.mPen.setWidth(1);
                    calibCurve.mIsHisto = false;

                    calibCurve.mData = d.getFormatedCalibMap();

                    const bool isUnif (d.mPlugin->getName() == "Unif");
                    calibCurve.mIsRectFromZero = isUnif;
                    calibCurve.mBrush = QBrush(Qt::NoBrush);

                    calibGraph->addCurve(calibCurve);

                    // Drawing the wiggle
                    if (d.mDeltaType !=  Date::eDeltaNone) {
                        GraphCurve curveWiggle;
                        QMap<double, double> calibWiggle = normalize_map(d.getFormatedWiggleCalibMap(), map_max_value(calibCurve.mData));
                        curveWiggle.mData = calibWiggle;

                        curveWiggle.mName = "Wiggle";
                        curveWiggle.mPen.setColor(Qt::red);
                        curveWiggle.mPen.setWidth(1);
                        curveWiggle.mBrush = QBrush(Qt::NoBrush);
                        curveWiggle.mIsHisto = false;
                        curveWiggle.mIsRectFromZero = true;
                        calibGraph->addCurve(curveWiggle);
                    }

                }

                // Insert the Event Name only if different to the previous Event's name
                QString eventName (ev.value(STATE_NAME).toString());
                if (eventName != preEventName)
                    calibGraph->addInfo(tr("Event") + " : "+ eventName);

                else
                    calibGraph->addInfo("");

                preEventName = eventName;

                calibGraph->addInfo(d.mName);
                calibGraph->showInfos(true);

                if (d.mIsValid && !d.mCalibration->mCurve.isEmpty()) {
                        // hpd is calculate only on the study Period
                        QMap<type_data, type_data> subData = calibCurve.mData;
                        subData = getMapDataInRange(subData, mSettings.getTminFormated(), mSettings.getTmaxFormated());

                        QMap<type_data, type_data> hpd = create_HPD(subData, mThreshold);

                        GraphCurve hpdCurve;
                        hpdCurve.mName = "Calibration HPD";
                        hpdCurve.mPen = brushColor;
                        hpdCurve.mBrush = brushColor;
                        hpdCurve.mIsHisto = false;
                        hpdCurve.mIsRectFromZero = true;
                        hpdCurve.mData = hpd;
                        calibGraph->addCurve(hpdCurve);

                        // update max inside the display period
                        QMap<type_data, type_data> subDisplay = calibCurve.mData;
                        subDisplay = getMapDataInRange(subDisplay, mTminDisplay, mTmaxDisplay);

                        const type_data yMax = map_max_value(subDisplay);

                        calibGraph->setRangeY(0., 1. * yMax);

                        calibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
                        calibGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
                        calibGraph->setFormatFunctY(nullptr);

                        calibGraph->setMarginRight(mMarginRight);
                        calibGraph->setMarginLeft(mMarginLeft);

                        calibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
                        calibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
                        calibGraph->changeXScaleDivision(mMajorScale, mMinorScale);

                        //calibGraph->setRendering(GraphView::eHD);

        }

                graphList.append(calibGraph);
                QColor color = QColor(ev.value(STATE_COLOR_RED).toInt(),
                                      ev.value(STATE_COLOR_GREEN).toInt(),
                                      ev.value(STATE_COLOR_BLUE).toInt());
                colorList.append(color);

            }

        }
   }
   mDrawing->showMarker();
   updateGraphsSize(mGraphHeightEdit->text());
   mDrawing->setEventsColorList(colorList);
   mDrawing->setGraphList(graphList);

   if (mStatClipBut->isChecked())
       showStat();

   updateScroll();
   update();
}

void MultiCalibrationView::updateMultiCalib()
{
    qDebug()<<"MultiCalibrationView::updateMultiCalib()";
    updateGraphList();
}

void MultiCalibrationView::updateHPDGraphs(const QString &thres)
{
    bool ok;
    double val = locale().toDouble(thres, &ok);
    if (ok)
        mThreshold = val;
    else
        return;

    QList<GraphView*> *graphList = mDrawing->getGraphList();

    for (GraphView* gr : *graphList) {
        GraphCurve* calibCurve = gr->getCurve("Calibration");
        // there is curve named "Calibration" in a Bound
        if (calibCurve) {
            // hpd is calculate only on the study Period
            QMap<type_data, type_data> subData = calibCurve->mData;
            subData = getMapDataInRange(subData, mSettings.getTminFormated(), mSettings.getTmaxFormated());

            QMap<type_data, type_data> hpd = create_HPD(subData, mThreshold);

            GraphCurve* hpdCurve = gr->getCurve("Calibration HPD");
            hpdCurve->mData = hpd;
            gr->forceRefresh();
        }
    }

    if (mStatClipBut ->isChecked())
        showStat();

}

void MultiCalibrationView::updateGraphsSize(const QString &sizeStr)
{
    bool ok;
    double val = locale().toDouble(sizeStr, &ok);
    if (ok) {
        const double origin (GraphViewResults::mHeightForVisibleAxis);//Same value in ResultsView::applyAppSettings()
        const double prop (val / 100.);
        mGraphHeight = int ( prop * origin );

    } else
        return;

    mDrawing->setGraphHeight(mGraphHeight);

}

void MultiCalibrationView::updateScroll()
{
    bool ok;
    double val = locale().toDouble(mStartEdit->text(),&ok);
    if (ok)
        mTminDisplay = val;
    else
        mTminDisplay = mSettings.getTminFormated();


    val = locale().toDouble(mEndEdit->text(),&ok);
    if (ok)
        mTmaxDisplay = val;
    else
        mTmaxDisplay = mSettings.getTmaxFormated();

    QFont adaptedFont (font());

    qreal textSize = fontMetrics().boundingRect(mStartEdit->text()).width()  + fontMetrics().boundingRect("0").width();
    if (textSize > mStartEdit->width()) {
        const qreal fontRate = textSize / mStartEdit->width();
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mStartEdit->setFont(adaptedFont);

    } else
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

    // Usefull when we set mStartEdit and mEndEdit at the begin of the display,
    // after a call to setDate
    if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay))
        return;
    else if (mTminDisplay<mTmaxDisplay)
            updateGraphsZoom();
    else
        return;

}

void MultiCalibrationView::updateScaleX()
{
    QString str = mMajorScaleEdit->text();
    bool isNumber(true);
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


    if (!isNumber || aNumber<1)
        return;
    mMajorScale = aNumber;

//----------------------------------------

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

    if (isNumber && aNumber>=1) {
        mMinorScale =  int (aNumber);
        QList<GraphView*> *graphList = mDrawing->getGraphList();

        for (GraphView* gr : *graphList) {
            if (!gr->hasCurve())
                continue;
            gr->changeXScaleDivision(mMajorScale, mMinorScale);
        }

    } else
        return;

}

void MultiCalibrationView::updateGraphsZoom()
{

    mMarginRight = int (1.5 * floor(fontMetrics().boundingRect(mEndEdit->text()).width()/2) + 5);
    mMarginLeft = int (1.5 * floor(fontMetrics().boundingRect(mStartEdit->text()).width()/2) + 5);

    QList<GraphView*> *graphList = mDrawing->getGraphList();

    for (GraphView* gr : *graphList) {

        if (gr->hasCurve()) {

            gr->setRangeX(mTminDisplay, mTmaxDisplay);
            gr->setCurrentX(mTminDisplay, mTmaxDisplay);

            // update max inside the display period (mTminDisplay, mTmaxDisplay)
            // Bound doesn't have curve named "Calibration", this curve name is "Bound"
            GraphCurve* calibCurve = gr->getCurve("Calibration");
            if (!calibCurve)
                calibCurve = gr->getCurve("Bound");

            QMap<type_data, type_data> subDisplay = calibCurve->mData;
            subDisplay = getMapDataInRange(subDisplay, mTminDisplay, mTmaxDisplay);

            type_data yMax = map_max_value(subDisplay);
            if (yMax == 0.)
                yMax = 1;

            gr->setRangeY(0., 1. * yMax);

            gr->setMarginRight(mMarginRight);
            gr->setMarginLeft(mMarginLeft);

            // ------------------------------------------------------------
            //  Show zones if calibrated data are outside study period
            // ------------------------------------------------------------
           gr->removeAllZones();
           if (mTminDisplay < mSettings.getTminFormated()) {
                GraphZone zone;
                zone.mXStart = mTminDisplay;
                zone.mXEnd = mSettings.getTminFormated();
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mText = tr("Outside study period");
                gr->addZone(zone);
            }
           if (mTmaxDisplay > mSettings.getTmaxFormated()) {
                GraphZone zone;
                zone.mXStart = mSettings.getTmaxFormated();
                zone.mXEnd = mTmaxDisplay;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mText = tr("Outside study period");
                gr->addZone(zone);
            }

            gr->forceRefresh();
        }
    }

}

void MultiCalibrationView::exportImage()
{
    mDrawing->hideMarker();
    mDrawing->update();
    QWidget* widgetExport = mDrawing->getGraphWidget();
    QFileInfo fileInfo = saveWidgetAsImage(widgetExport, widgetExport->rect(), tr("Save calibration image as..."),
                                           MainWindow::getInstance()->getCurrentPath());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());

    mDrawing->showMarker();
}

void MultiCalibrationView::exportFullImage()
{
    bool printAxis = (mGraphHeight < GraphViewResults::mHeightForVisibleAxis);
    QFontMetricsF fmAxe (mDrawing->font());

    QWidget* widgetExport = mDrawing->getGraphWidget();

    int minDeltaPix (3); //same value as GraphView::mStepMinWidth
    widgetExport->setFont(mDrawing->font());


    // --------------------------------------------------------------------

    AxisWidget* axisWidget = nullptr;
    QLabel* axisLegend = nullptr;
   // int axeHeight (int (font().pointSize() * 2.2));
    int axeHeight (int ( fmAxe.ascent() * 2.2));
    int legendHeight ( int (1.5 * (fmAxe.descent() + fmAxe.ascent())));

    if (printAxis) {
        widgetExport->resize(widgetExport->width(), widgetExport->height() + axeHeight + legendHeight);

        DateConversion f = nullptr; //DateUtils::convertToAppSettingsFormat;

        const int graphShift (5); // the same name and the same value as MultiCalibrationDrawing::updateLayout()
        axisWidget = new AxisWidget(f, widgetExport);
        axisWidget->setFont(mDrawing->font());
        axisWidget->mMarginLeft = mMarginLeft;
        axisWidget->mMarginRight = mMarginRight ;

        //qDebug()<<"multiCal Export"<<mMajorScale << mMinorScale;
        axisWidget->setScaleDivision(mMajorScale, mMinorScale);
        axisWidget->updateValues(int (widgetExport->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight -ColoredBar::mWidth - graphShift), minDeltaPix, mTminDisplay, mTmaxDisplay);


        //axisWidget->mShowText = true;
        axisWidget->setAutoFillBackground(true);
        axisWidget->mShowSubs = true;
        axisWidget->mShowSubSubs = true;
        axisWidget->mShowArrow = true;
        axisWidget->mShowText = true;
        axisWidget->setGeometry(ColoredBar::mWidth + graphShift, widgetExport->height() - axeHeight, widgetExport->width() - ColoredBar::mWidth - graphShift, axeHeight);

        axisWidget->raise();
        axisWidget->setVisible(true);

        QString legend = DateUtils::getAppSettingsFormatStr();

        axisLegend = new QLabel(legend, widgetExport);

        axisLegend->setGeometry(0, widgetExport->height() - axeHeight - legendHeight, widgetExport->width() - 10, legendHeight);

        axisLegend->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        axisLegend->raise();
        axisLegend->setVisible(true);
    }

    QFileInfo fileInfo = saveWidgetAsImage(widgetExport,
                                           QRect(0, 0, widgetExport->width() , widgetExport->height()),
                                           tr("Save graph image as..."),
                                           MainWindow::getInstance()->getCurrentPath());

    // Delete additional widgets if necessary :
    if (printAxis) {
        if (axisWidget) {
            axisWidget->setParent(nullptr);
            delete axisWidget;
        }
        if (axisLegend) {
            axisLegend->setParent(nullptr);
            delete axisLegend;
        }
        widgetExport->resize(widgetExport->width() ,widgetExport->height() - axeHeight - legendHeight);
    } else
        widgetExport->resize(widgetExport->width() ,widgetExport->height() - legendHeight);


    //delete (widgetExport);
    // Revert to default display :

    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    updateLayout();
}


void MultiCalibrationView::copyImage()
{
    if (mStatClipBut->isChecked()) {
        QApplication::clipboard()->setText(mTextArea->toPlainText());

    } else {
        mDrawing->hideMarker();
        mDrawing->repaint();
        QApplication::clipboard()->setPixmap(mDrawing->grab());
        mDrawing->showMarker();
    }
}

void MultiCalibrationView::changeCurveColor()
{
    QColor color = QColorDialog::getColor(mCurveColor, qApp->activeWindow(), tr("Select Colour"));
    if (color.isValid()) {
        mCurveColor = color;
        QList<GraphView*> *graphList = mDrawing->getGraphList();

        for (GraphView* gr : *graphList) {
            if (gr->hasCurve()) {
                GraphCurve* calibCurve = gr->getCurve("Calibration");
                if (!calibCurve)
                    calibCurve = gr->getCurve("Bound");

                calibCurve->mPen.setColor(mCurveColor);


                GraphCurve* hpdCurve = gr->getCurve("Calibration HPD");
                if (hpdCurve) {
                    hpdCurve->mPen.setColor(mCurveColor);
                    const QColor brushColor (mCurveColor.red(),mCurveColor.green(), mCurveColor.blue(), 100);
                    hpdCurve->mBrush = QBrush(brushColor);
                }


                gr->forceRefresh();
            }
        }
    }
}

void MultiCalibrationView::copyText()
{
    QApplication::clipboard()->setText(mResultText.replace("<br>", "\r"));
}

void MultiCalibrationView::showStat()
{
   if (mStatClipBut ->isChecked()) {
       mDrawing->setVisible(false);
       mTextArea->setVisible(true);
       mTextArea->setFont(font());

       // update Results from selected Event in JSON
       QJsonObject state = mProject->state();
       const QJsonArray events = state.value(STATE_EVENTS).toArray();
       QVector<QJsonObject> selectedEvents;

       for (auto&& ev : events) {
          QJsonObject jsonEv = ev.toObject();
           if (jsonEv.value(STATE_IS_SELECTED).toBool())
               selectedEvents.append(jsonEv);
       }

       // Sort Event by Position
       std::sort(selectedEvents.begin(), selectedEvents.end(), [] (QJsonObject ev1, QJsonObject ev2) {return (ev1.value(STATE_ITEM_Y).toDouble() < ev2.value(STATE_ITEM_Y).toDouble());});

       mResultText = "";
       for (auto& ev : selectedEvents) {
           // Insert the Event's Name only if different to the previous Event's name
           const QString eventName (ev.value(STATE_NAME).toString());
           const QColor color = QColor(ev.value(STATE_COLOR_RED).toInt(),
                                 ev.value(STATE_COLOR_GREEN).toInt(),
                                 ev.value(STATE_COLOR_BLUE).toInt());
           const QColor nameColor = getContrastedColor(color);
           QString resultsStr = textBackgroundColor("<big>" +textColor(eventName, nameColor) + "</big> ", color);



           if ( Event::Type (ev.value(STATE_EVENT_TYPE).toInt()) == Event::eKnown) {
               const double bound = ev.value(STATE_EVENT_KNOWN_FIXED).toDouble();
               resultsStr += " <br><strong>"+ tr("Bound : %1").arg(locale().toString(bound)) +" BC/AD </strong><br>";

           } else {
               const QJsonArray dates = ev.value(STATE_EVENT_DATES).toArray();


                for (auto&& date : dates) {
                   const QJsonObject jdate = date.toObject();

                   Date d(jdate);

                   resultsStr += " <br> <strong>"+ d.mName + "</strong> (" + d.mPlugin->getName() + ")" +"<br> <i>" + d.getDesc() + "</i><br> ";

                 if (d.mIsValid && !d.mCalibration->mCurve.isEmpty()) {


                       const bool isUnif (d.mPlugin->getName() == "Unif");


                       if (!isUnif) {
                           d.autoSetTiSampler(true); // needed if calibration is not done

                           QMap<double, double> calibMap = d.getFormatedCalibMap();
                           // hpd is calculate only on the study Period

                           QMap<double, double>  subData = calibMap;
                           subData = getMapDataInRange(subData, mSettings.getTminFormated(), mSettings.getTmaxFormated());

                           DensityAnalysis results;
                           results.analysis = analyseFunction(subData);

                           if (!subData.isEmpty()) {
                               QVector<double> subRepart = calculRepartition(subData);

                               results.quartiles = quartilesForRepartition(subRepart, subData.firstKey(), mSettings.mStep);
                               resultsStr += densityAnalysisToString(results);

                                // hpd results

                               QMap<type_data, type_data> hpd = create_HPD(subData, mThreshold);

                               const double realThresh = map_area(hpd) / map_area(subData);

                               resultsStr += + "<br> HPD (" + locale().toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormatStr(), DateUtils::convertToAppSettingsFormat) + "<br>";

                           } else
                               resultsStr += "<br>" + textBold(textRed(QObject::tr("Solutions exist outside study period") ))  + "<br>";

                      }

                 } else
                     resultsStr += + "<br> HPD  : " + tr("Not  computable")+ "<br>";

               }
            }
            mResultText += resultsStr;
      }

    mTextArea->setHtml(mResultText);


    } else {
       mDrawing->setVisible(true);
       mTextArea->setVisible(false);
    }

   updateLayout();
}

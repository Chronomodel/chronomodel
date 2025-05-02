/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

#include "CalibrationCurve.h"
#include "DoubleValidator.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "GraphViewResults.h"
#include "CurveSettings.h"
#include "PluginAbstract.h"
#include "AppSettings.h"

#include "SilvermanDialog.h"

#include <QPainter>
#include <QJsonArray>
#include <QLocale>


MultiCalibrationView::MultiCalibrationView(QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mDrawing (nullptr),
    mMajorScale (100),
    mMinorScale (4),
    mTminDisplay (-HUGE_VAL),
    mTmaxDisplay (HUGE_VAL),
    mThreshold (95),
    mGraphHeight (120),
    mUsePluginColor (false),
    mUseEventColor (false),
    mUseCustomColor (true),
    mCurveCustomColor(Painting::mainColorDark)
{
    QPalette palette_BW;
    palette_BW.setColor(QPalette::Base, Qt::white);
    palette_BW.setColor(QPalette::Text, Qt::black);

    mButtonWidth = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    setMouseTracking(true);
    mDrawing = new MultiCalibrationDrawing(this);

    mStatArea = new QTextEdit(this);
    mStatArea->setFrameStyle(QFrame::NoFrame);
    mStatArea->setPalette(palette_BW);

    mStatArea->setText(tr("Nothing to display"));
    mStatArea->setVisible(false);
    mStatArea->setReadOnly(true);

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

    mScatterClipBut= new Button(tr("Scatter"), this);
    mScatterClipBut->setIcon(QIcon(":curve_data_graph_w.png"));
    mScatterClipBut->setFlatVertical();
    mScatterClipBut->setToolTip(tr("Show Scatter plot for selected dates"));
    mScatterClipBut->setIconOnly(true);
    mScatterClipBut->setChecked(false);
    mScatterClipBut->setCheckable(true);

    mFitClipBut = new Button(tr("Fit"), this);
    mFitClipBut->setIcon(QIcon(":curve_silver_graph_w.png"));
    mFitClipBut->setFlatVertical();
    mFitClipBut->setToolTip(tr("Show Fit plot for selected dates"));
    mFitClipBut->setIconOnly(true);
    mFitClipBut->setChecked(false);
    mFitClipBut->setCheckable(true);

    mExportResults = new Button(tr("CSV"), this);
    mExportResults->setFlatVertical();
    mExportResults->setIcon(QIcon(":export_csv_w.png"));
    mExportResults->setIconOnly(true);
    mExportResults->setChecked(false);
    mExportResults->setToolTip(tr("Export stats to a CSV file"));


    mGraphHeightLab = new Label(tr("Zoom"), this);
    mGraphHeightLab->setAdjustText();
    mGraphHeightLab->setAlignment(Qt::AlignHCenter);
    mGraphHeightLab->setLight();
    mGraphHeightLab->setBackground(Painting::borderDark);

    mGraphHeightEdit = new LineEdit(this);
    mGraphHeightEdit->setText(locale().toString(100));

    QIntValidator* heightValidator = new QIntValidator(this);
    heightValidator->setRange(50, 500);
    mGraphHeightEdit->setValidator(heightValidator);
    mGraphHeightEdit->setAdjustText(true);

    mYZoom = new ScrollCompressor(this);
    mYZoom->setProp(0.5);
    mYZoom->showText(tr("Zoom"), true);

    mColorClipBut = new Button(tr("Color"), this);
    mColorClipBut->setIcon(QIcon(":color.png"));
    mColorClipBut->setFlatVertical();
    mColorClipBut->setToolTip(tr("Set Personal Densities Color"));
    mColorClipBut->setIconOnly(true);

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

    DoubleValidator* percentValidator = new DoubleValidator(this);
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
    connect(mScatterClipBut, &Button::clicked, this, &MultiCalibrationView::showScatter);
    connect(mFitClipBut, &Button::clicked, this, &MultiCalibrationView::showFit);

    connect(mExportResults, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &MultiCalibrationView::exportResults);

    connect(mGraphHeightEdit, &QLineEdit::textEdited, this, &MultiCalibrationView::updateGraphsSize);
    connect(mYZoom, &ScrollCompressor::valueChanged, this, &MultiCalibrationView::updateYZoom);

    connect(mColorClipBut, &Button::clicked, this, &MultiCalibrationView::changeCurveColor);

}

MultiCalibrationView::~MultiCalibrationView()
{
    
}

void MultiCalibrationView::setProject()
{
    auto state = MainWindow::getInstance()->getState();
    const QJsonObject &settings = state.value(STATE_SETTINGS).toObject();
    mSettings = StudyPeriodSettings::fromJson(settings);

    mTminDisplay = mSettings.getTminFormated() ;
    mTmaxDisplay = mSettings.getTmaxFormated();

    Scale scale(mTminDisplay, mTmaxDisplay);

    mMajorScale = scale.mark;
    mMinorScale = scale.tip;

    mStartEdit->setText(locale().toString(mTminDisplay));
    mEndEdit->setText(locale().toString(mTmaxDisplay));
    mHPDEdit->setText(locale().toString(95));

    mMajorScaleEdit->setText(locale().toString(mMajorScale));
    mMinorScaleEdit->setText(locale().toString(mMinorScale));
}

void MultiCalibrationView::resizeEvent(QResizeEvent* )
{
    updateLayout();
}


void MultiCalibrationView::paintEvent(QPaintEvent* e)
{
    (void) e;

    QPainter p(this);

    const int textHeight = int (1.2 * (fontMetrics().descent() + fontMetrics().ascent()) );
    const qreal yPosBottomBar0 = height() - 2*textHeight - 12;

    // drawing a background under button
    p.fillRect(width() - mButtonWidth, 0, mButtonWidth, yPosBottomBar0, Painting::borderDark);

    // Bottom Tools Bar
    p.fillRect(0, mStartLab->y() - 2,  width(), yPosBottomBar0 + 2, Painting::borderDark);

}

void MultiCalibrationView::setVisible(bool visible)
{
    mImageSaveBut->setVisible(visible);
    mImageClipBut->setVisible(visible);
    mStatClipBut->setVisible(visible);
    //frameSeparator->setVisible(visible);

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
    mButtonWidth  = int (1.7 * AppSettings::widthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);
    mButtonHeigth = int (1.7 * AppSettings::heigthUnit() * AppSettings::mIconSize/ APP_SETTINGS_DEFAULT_ICON_SIZE);

    mTminDisplay = mSettings.getTminFormated() ;
    mTmaxDisplay = mSettings.getTmaxFormated();

    Scale scale(mTminDisplay, mTmaxDisplay);

    mMajorScale = scale.mark;
    mMinorScale = scale.tip;

    mStartEdit->setText(locale().toString(mTminDisplay));
    mEndEdit->setText(locale().toString(mTmaxDisplay));
    mHPDEdit->setText(locale().toString(95));

    mMajorScaleEdit->setText(locale().toString(mMajorScale));
    mMinorScaleEdit->setText(locale().toString(mMinorScale));

    if (this->isVisible())
        updateGraphList();

}

void MultiCalibrationView::updateLayout()
{
    auto project = getProject_ptr();
    const int graphWidth = std::max(0, width() - mButtonWidth);

    const int x0 = graphWidth;
    const int margin = int(0.1 * mButtonWidth);
    const int xm = x0 + margin;

    const int textHeight = int (1.2 * (fontMetrics().descent() + fontMetrics().ascent()) );
    const int verticalSpacer = int (0.3 * AppSettings::heigthUnit());

    //Position of Widget
    int y  = 0;

    const bool curveModel = project ? project->isCurve(): false;
    mImageSaveBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mImageClipBut->height();
    mStatClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mStatClipBut->height() + 5;
    mScatterClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mStatClipBut->height() + 5;

    if (curveModel) {
        mFitClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
        y += mFitClipBut->height() + 5;

    } else {
        mFitClipBut->setGeometry(x0, y, 0, 0);
        y += 0;
    }
    mFitClipBut->setVisible(curveModel);

    mExportResults->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mExportResults->height() + 5;

    mColorClipBut->setGeometry(x0, y, mButtonWidth, mButtonHeigth);
    y += mColorClipBut->height();

    mGraphHeightLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mGraphHeightLab->height();
    mGraphHeightEdit->setGeometry(xm, y, mButtonWidth - 2 * margin, textHeight);
    y += mGraphHeightEdit->height() + verticalSpacer;


    // Bottom tools bar
    const qreal yPosBottomBar0 = height() - 2*textHeight - 12;
    const qreal yPosBottomBar1 = yPosBottomBar0 + textHeight + 2;

    mYZoom->setGeometry(x0, y, mButtonWidth, yPosBottomBar0 - y);

    const qreal labelWidth = std::min( fontMetrics().horizontalAdvance("-1000000"), width() /5);
    const qreal editWidth = labelWidth;
    const qreal marginBottomBar = (width()- 5.*labelWidth )/6.;

    qreal xShift = marginBottomBar;
    mStartLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mStartEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    xShift = labelWidth + 2*marginBottomBar;
    mEndLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mEndEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

    xShift = 2*labelWidth + 3*marginBottomBar;
    mMajorScaleLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mMajorScaleEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);
    mMajorScaleEdit->setText(locale().toString(mMajorScale));

    xShift = 3*labelWidth + 4*marginBottomBar;
    mMinorScaleLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mMinorScaleEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);
    mMinorScaleEdit->setText(locale().toString(mMinorScale));

    xShift = 4*labelWidth + 5*marginBottomBar;
    mHPDLab->setGeometry(xShift, yPosBottomBar0, labelWidth, textHeight);
    mHPDEdit->setGeometry(xShift, yPosBottomBar1, editWidth, textHeight);

//GraphViewResults::mHeightForVisibleAxis = 4 * AppSettings::heigthUnit();
    if (mStatClipBut->isChecked()) {
        mStatArea->show();
        mStatArea->setGeometry(0, 0, graphWidth, yPosBottomBar0);

        if (mDrawing)
            mDrawing->hide();

        mColorClipBut->setEnabled(false);
        mScatterClipBut->setEnabled(false);
        mFitClipBut->setEnabled(false);

    } else if (mFitClipBut->isChecked() && curveModel) {
        mStatArea->hide();

        mDrawing->setGeometry(0, 0, graphWidth, yPosBottomBar0);
        mDrawing->updateLayout();
        mDrawing->show();

        mColorClipBut->setEnabled(true);
        mStatClipBut->setEnabled(false);
        mScatterClipBut->setEnabled(true);

    } else if (mScatterClipBut->isChecked()) {
        mStatArea->hide();

        mDrawing->setGeometry(0, 0, graphWidth, yPosBottomBar0);
        mDrawing->updateLayout();
        mDrawing->show();

        mColorClipBut->setEnabled(true);
        mStatClipBut->setEnabled(false);
        mFitClipBut->setEnabled(true);

    } else {
        mStatArea->hide();

        mDrawing->setGeometry(0, 0, graphWidth, yPosBottomBar0);
        mDrawing->updateLayout();
        mDrawing->show();

        mStatClipBut->setEnabled(true);
        mScatterClipBut->setEnabled(true);
        mFitClipBut->setEnabled(true);
        mColorClipBut->setEnabled(true);

    }

}

void MultiCalibrationView::updateGraphList()
{

    if (mStatClipBut->isChecked()) {

        mDrawing->setVisible(false);
        mStatArea->setVisible(true);
        showStat();
        updateLayout();

    } else {

        const QJsonObject* state = getState_ptr();
        mSettings = StudyPeriodSettings::fromJson(state->value(STATE_SETTINGS).toObject());

        mDrawing->setVisible(true);
        mStatArea->setVisible(false);

        if (mDrawing) {
            delete mDrawing;
            mDrawing = nullptr;
        }

        if (mScatterClipBut->isChecked()) {
            mHeightForVisibleAxis = 8 * AppSettings::heigthUnit();
            mDrawing = scatterPlot(mThreshold);

        } else if (mFitClipBut->isChecked()) {
            mHeightForVisibleAxis = 8 * AppSettings::heigthUnit();
            mDrawing = fitPlot(mThreshold);

        }  else {
            mHeightForVisibleAxis = 4 * AppSettings::heigthUnit();
            mDrawing = multiCalibrationPlot(mThreshold);

        }
        const double origin = mHeightForVisibleAxis;
        const double prop = mYZoom->getProp();

        mGraphHeight = mScatterClipBut->isChecked() || mFitClipBut->isChecked()?  3*prop * origin * 2 : prop * origin * 2;

        mDrawing->setGraphHeight(mGraphHeight);
        mDrawing->show();
        mDrawing->setVisible(true);

        updateLayout();

        updateScroll();
    }

}

MultiCalibrationDrawing* MultiCalibrationView::multiCalibrationPlot(const double thres)
{
    const QJsonObject* state = getState_ptr();
    mSettings = StudyPeriodSettings::fromJson(state->value(STATE_SETTINGS).toObject());

    QColor penColor = mCurveCustomColor;
    QColor brushColor = mCurveCustomColor;
    brushColor.setAlpha(170);

    QList<GraphViewAbstract*> graphList;
    QList<QColor> colorList;
    QList<bool> listAxisVisible;
    const QJsonArray &events = state->value(STATE_EVENTS).toArray();

    QList<QJsonObject> selectedEvents;

    const bool curveModel = getProject_ptr()->isCurve();
    CurveSettings::ProcessType processType = static_cast<CurveSettings::ProcessType>(state->value(STATE_CURVE).toObject().value(STATE_CURVE_PROCESS_TYPE).toInt());

    for (auto&& ev : events) {
       QJsonObject jsonEv = ev.toObject();
       if (jsonEv.value(STATE_IS_SELECTED).toBool())
            selectedEvents.append(jsonEv);
    }


    // Sort Event by Position
    std::sort(selectedEvents.begin(), selectedEvents.end(), [] (QJsonObject ev1, QJsonObject ev2) {return (ev1.value(STATE_ITEM_Y).toDouble() < ev2.value(STATE_ITEM_Y).toDouble());});

    QJsonObject* preEvent = nullptr;

    for (QJsonObject &ev : selectedEvents) {

        const QColor event_color = QColor(ev.value(STATE_COLOR_RED).toInt(),
                              ev.value(STATE_COLOR_GREEN).toInt(),
                              ev.value(STATE_COLOR_BLUE).toInt());
        const QString curveDescription = curveModel ? Event::curveDescriptionFromJsonEvent(ev, processType): "";

        if (ev.value(STATE_EVENT_TYPE).toInt() == Event::eBound) {

            const double fixedValue = ev.value(STATE_EVENT_KNOWN_FIXED).toDouble();
            const double tFixedFormated = DateUtils::convertToAppSettingsFormat( fixedValue);

            const QString boundName = ev.value(STATE_NAME).toString();
            const QString valueStr = locale().toString(tFixedFormated) + " " + DateUtils::getAppSettingsFormatStr();
            graphList.append(new GraphTitle(tr("Bound : %1").arg(boundName),  curveDescription, QString("Fixed value : %1 ").arg(valueStr), this));
            colorList.append(event_color);

            GraphView* calibGraph = new GraphView(this);
            calibGraph->setRangeY(0., 1.);

            if (mUsePluginColor) {
                penColor = Qt::red;

            } else if (mUseEventColor) {
                penColor = event_color;

            } else {
                penColor = mCurveCustomColor;

            }
            brushColor = penColor;
            brushColor.setAlpha(170);

            GraphCurve calibCurve = horizontalSection( qMakePair(tFixedFormated, tFixedFormated), "Bound", penColor, QBrush(brushColor));
            calibCurve.mVisible = true;
            calibGraph->add_curve(calibCurve);

            calibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            calibGraph->setFormatFunctX(nullptr);
            calibGraph->setFormatFunctY(nullptr);

            calibGraph->setMarginRight(mMarginRight);
            calibGraph->setMarginTop(0);
            calibGraph->setMarginBottom(0);

            calibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
            calibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
            calibGraph->changeXScaleDivision(mMajorScale, mMinorScale);
            calibGraph->setOverArrow(GraphView::eNone);


            calibGraph->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            calibGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
            calibGraph->setYAxisMode(GraphView::eHidden);
            calibGraph->showYAxisLine(false);

            graphList.append(calibGraph);
            listAxisVisible.append(true);

            colorList.append(event_color);


        } else {
            const QJsonArray &dates = ev.value(STATE_EVENT_DATES).toArray();

            for (auto&& date : dates) {
                Date d (date.toObject());
                const QColor plugin_color = d.mPlugin->getColor();

                if (mUsePluginColor) {
                    penColor = plugin_color;

                } else if (mUseEventColor) {
                    penColor = event_color;

                } else {
                    penColor = mCurveCustomColor;

                }
                brushColor = penColor;
                brushColor.setAlpha(170);

                GraphCurve calibCurve;
                GraphView* calibGraph = new GraphView(this);

                 if (d.mIsValid && d.mCalibration!=nullptr && !d.mCalibration->mVector.empty()) {

                    calibCurve = densityCurve(d.getFormatedCalibToShow(), "Calibration", penColor);
                    calibCurve.mVisible = true;
                    calibGraph->add_curve(calibCurve);

                    // Drawing the wiggle
                    if (d.mDeltaType !=  Date::eDeltaNone) {

                        const std::map<double, double> &calibWiggle = normalize_map(d.getFormatedWiggleCalibToShow(), map_max(calibCurve.mData).value());
                        GraphCurve curveWiggle = densityCurve(calibWiggle, "Wiggle", Qt::blue, Qt::DashLine, Qt::NoBrush);
                        curveWiggle.mVisible = true;
                        calibGraph->add_curve(curveWiggle);
                    }

                }

                // Insert the Event Name only if different to the previous Event's name
                const QString eventName (ev.value(STATE_NAME).toString());

                listAxisVisible.append(true);
                if (&ev != preEvent) {
                    graphList.append(new GraphTitle(tr("Event : %1 ").arg(eventName), curveDescription, d.getQStringName(), this));
                    colorList.append(event_color);

                } else {
                    graphList.append(new GraphTitle("", d.getQStringName(), this));
                    colorList.append(event_color);

                    listAxisVisible[listAxisVisible.size()-2] = false;
                }

                preEvent =  &ev ;


                if (d.mIsValid && d.mCalibration != nullptr && !d.mCalibration->mVector.empty()) {
                    // hpd is calculate only on the study Period
                    const QMap<type_data, type_data> &subData = getMapDataInRange(calibCurve.mData, mSettings.getTminFormated(), mSettings.getTmaxFormated());

                    QList<QPair<double, QPair<double, double> > > intervals;
                    QMap<type_data, type_data> hpd (create_HPD_by_dichotomy(subData, intervals, thres));

                    GraphCurve hpdCurve;
                    hpdCurve.mName = "Calibration HPD";
                    hpdCurve.mPen = brushColor;
                    hpdCurve.mBrush = brushColor;

                    hpdCurve.mIsRectFromZero = true;
                    hpdCurve.mData = hpd;
                    hpdCurve.mVisible = true;
                    calibGraph->add_curve(hpdCurve);

                    // update max inside the display period , it's done with updateGraphZoom()

                    calibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
                    calibGraph->setFormatFunctX(nullptr);
                    calibGraph->setFormatFunctY(nullptr);

                    calibGraph->setMarginRight(mMarginRight);
                    calibGraph->setMarginTop(0);

                    calibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
                    calibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
                    calibGraph->changeXScaleDivision(mMajorScale, mMinorScale);

                    calibGraph->setYAxisMode(GraphView::eHidden);
                    calibGraph->showYAxisLine(false);

                }

                graphList.append(calibGraph);
                calibGraph = nullptr;
                colorList.append(event_color);

            }

        }
   }

   preEvent = nullptr;
   MultiCalibrationDrawing* multiDraw = new MultiCalibrationDrawing(this);

   multiDraw->showMarker();

   mGraphHeightEdit->blockSignals(true);
   mGraphHeightEdit->setText(mGraphHeightEdit->text());
   mGraphHeightEdit->blockSignals(false);
   multiDraw->setEventsColorList(colorList);
   multiDraw->setListAxisVisible(listAxisVisible);

   multiDraw->setGraphList(graphList); // must be after setEventsColorList() and setListAxisVisible()

   return std::move(multiDraw);
}

MultiCalibrationDrawing* MultiCalibrationView::scatterPlot(const double thres)
{
    QJsonObject* state = getState_ptr();

    mSettings = StudyPeriodSettings::fromJson(state->value(STATE_SETTINGS).toObject());

    mHPDEdit->setText(locale().toString(thres));

    QColor brushColor = mCurveCustomColor;
    brushColor.setAlpha(170);

    QList<GraphViewAbstract*> graphList;

    QList<QColor> colorList;
    QList<bool> listAxisVisible;
    const QJsonArray &events = state->value(STATE_EVENTS).toArray();

    QList<QJsonObject> selectedEvents;

    const CurveSettings cs (state->value(STATE_CURVE).toObject());
    CurveSettings::ProcessType processType = cs.mProcessType;

    for (const auto&& ev : events) {
       const QJsonObject jsonEv = ev.toObject();
       if (jsonEv.value(STATE_IS_SELECTED).toBool())
            selectedEvents.append(jsonEv);
    }

    GraphView* graph1 = nullptr;
    GraphView* graph2 = nullptr;
    GraphView* graph3 = nullptr;

    std::vector<CurveRefPts> curveDataPointsX, curveDataPointsY, curveDataPointsZ;
    CurveRefPts ptsX, ptsY, ptsZ;

    switch (processType) {
        case CurveSettings::eProcess_3D:
        case CurveSettings::eProcess_Vector:

            graph3 = new GraphView(this);
            graph3->showInfos(true);

            graph3->mLegendX = DateUtils::getAppSettingsFormatStr();
            graph3->setFormatFunctX(nullptr);
            graph3->setFormatFunctY(nullptr);

            graph3->setRangeX(mTminDisplay, mTmaxDisplay);
            graph3->setCurrentX(mTminDisplay, mTmaxDisplay);
            graph3->changeXScaleDivision(mMajorScale, mMinorScale);
            graph3->setOverArrow(GraphView::eNone);
            graph3->setTipXLab("t");


            graph3->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            graph3->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

            graph3->autoAdjustYScale(true);

            graph3->setYAxisMode(GraphView::eAllTicks);
            graph3->showYAxisSubTicks(true);
            graph3->setTipYLab(cs.Z_short_name());

            graph3->setMarginTop(graph3->fontMetrics().height()/2.);
            [[fallthrough]];
        case CurveSettings::eProcess_Spherical:
        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_2D:

            graph2 = new GraphView(this);
            graph2->showInfos(true);

            graph2->mLegendX = DateUtils::getAppSettingsFormatStr();
            graph2->setFormatFunctX(nullptr);
            graph2->setFormatFunctY(nullptr);

            graph2->setRangeX(mTminDisplay, mTmaxDisplay);
            graph2->setCurrentX(mTminDisplay, mTmaxDisplay);
            graph2->changeXScaleDivision(mMajorScale, mMinorScale);
            graph2->setOverArrow(GraphView::eNone);
            graph2->setTipXLab("t");

            graph2->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            graph2->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

            graph2->autoAdjustYScale(true);

            graph2->setYAxisMode(GraphView::eAllTicks);
            graph2->showYAxisSubTicks(true);
            graph2->setTipYLab(cs.Y_short_name());
            graph2->setMarginTop(graph2->fontMetrics().height()/2.);
            [[fallthrough]];
        case CurveSettings::eProcess_None:
        case CurveSettings::eProcess_Univariate:
        case CurveSettings::eProcess_Depth:
        case CurveSettings::eProcess_Inclination:
        case CurveSettings::eProcess_Declination:
        case CurveSettings::eProcess_Field:
        default:

            graph1 = new GraphView(this);
            graph1->showInfos(true);

            graph1->mLegendX = DateUtils::getAppSettingsFormatStr();
            graph1->setFormatFunctX(nullptr);
            graph1->setFormatFunctY(nullptr);

            graph1->setRangeX(mTminDisplay, mTmaxDisplay);
            graph1->setCurrentX(mTminDisplay, mTmaxDisplay);
            graph1->changeXScaleDivision(mMajorScale, mMinorScale);
            graph1->setOverArrow(GraphView::eNone);
            graph1->setTipXLab("t");


            graph1->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            graph1->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

            graph1->autoAdjustYScale(true);

            //graph1->setYAxisMode( GraphView::eAllTicks); // dans ce cas, c'est fait plus bas, car besoin des données
            graph1->showYAxisSubTicks(processType != CurveSettings::eProcess_None);

            graph1->setTipYLab(cs.X_short_name());
            graph1->setMarginTop(graph1->fontMetrics().height()/2.);

        break;
    }


    QMap <double, double> poly_data_X;
    QMap <double, double> poly_data_Y;
    QMap <double, double> poly_data_Z;

    QMap <double, double> poly_data_X_err;
    QMap <double, double> poly_data_Y_err;
    QMap <double, double> poly_data_Z_err;

    double X, errX, Y, errY, Z, errZ, tmin, tmax;
    for (const auto& sEvent : selectedEvents) {
        const double xIncDepth = sEvent.value(STATE_EVENT_X_INC_DEPTH).toDouble();
        const double s_XA95Depth = sEvent.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
        const double yDec = sEvent.value(STATE_EVENT_Y_DEC).toDouble();
        const double s_Y = sEvent.value(STATE_EVENT_SY).toDouble();
        const double zField = sEvent.value(STATE_EVENT_Z_F).toDouble();
        const double s_ZField = sEvent.value(STATE_EVENT_SZ_SF).toDouble();

        const QColor event_color (sEvent.value(STATE_COLOR_RED).toInt(),
                       sEvent.value(STATE_COLOR_GREEN).toInt(),
                       sEvent.value(STATE_COLOR_BLUE).toInt());
        // Same calcul within ResultsView::createByCurveGraph()
        switch (processType) {
            case CurveSettings::eProcess_Univariate:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;
            case CurveSettings::eProcess_Depth:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;
            case CurveSettings::eProcess_Field:
                X = zField;
                errX = 1.96*s_ZField;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;
            case CurveSettings::eProcess_Inclination:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;//ici diviser par 2,448
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;
            case CurveSettings::eProcess_Declination:
                X = yDec;
                errX = 1.96*(s_XA95Depth/2.448)/ cos(xIncDepth/180*3.14 );
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Unknwon_Dec:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;
                Y = zField;
                errY = s_ZField;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_2D:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = yDec;
                errY = 1.96*s_Y;
                Z = 0.;
                errZ = 0.;
                break;
            case CurveSettings::eProcess_Spherical:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;//ici diviser par 2,448
                Y = yDec;
                errY = errX/ cos(xIncDepth/180.*M_PI );
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Vector:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;//ici diviser par 2,448
                Y = yDec;
                errY = errX/ cos(xIncDepth/180.*M_PI );
                Z = zField;
                errZ = s_ZField;
                break;
            case CurveSettings::eProcess_3D:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = yDec;
                errY = 1.96*s_Y;
                Z = zField;
                errZ = 1.96*s_ZField;
                break;
            default:
                X = - sEvent.value(STATE_ITEM_Y).toDouble();
                errX = 0.;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;
        }

        if ( Event::Type (sEvent.value(STATE_EVENT_TYPE).toInt()) == Event::eBound) {
            const double bound = sEvent.value(STATE_EVENT_KNOWN_FIXED).toDouble();

            ptsX.Xmin = DateUtils::convertToAppSettingsFormat(bound);
            ptsX.Xmax = ptsX.Xmin;

            ptsX.Ymin = X - errX;
            ptsX.Ymax = X + errX;
            QColor penColor;
            if (mUsePluginColor) {
                penColor = Qt::red;

            } else if (mUseEventColor) {
                penColor = event_color;

            } else {
                penColor = mCurveCustomColor;

            }
            //brushColor = penColor;
            //brushColor.setAlpha(170);

            ptsX.color = penColor;
            ptsX.type = processType == CurveSettings::eProcess_None ?  CurveRefPts::ePoint : CurveRefPts::eRoundLine;

            ptsX.pen = QPen(Qt::black, 2, Qt::SolidLine);
            ptsX.brush = Qt::black;
            ptsX.name = "Ref Points";
            ptsX.comment = sEvent.value(STATE_NAME).toString();
            ptsX.setVisible(true);

            curveDataPointsX.push_back(ptsX);

            ptsY = ptsX;
            ptsY.Ymin = Y - errY;
            ptsY.Ymax = Y + errY;

            ptsY.pen = QPen(Qt::black, 2, Qt::SolidLine);
            ptsY.brush = Qt::black;
            ptsY.setVisible(true);
            curveDataPointsY.push_back(ptsY);

            ptsZ = ptsX;
            ptsZ.Ymin = Z - errZ;
            ptsZ.Ymax = Z + errZ;
            ptsZ.pen = QPen(Qt::black, 2, Qt::SolidLine);
            ptsZ.brush = Qt::black;
            ptsZ.setVisible(true);
            curveDataPointsZ.push_back(ptsZ);

            poly_data_X[bound] = X ;
            poly_data_Y[bound] = Y ;
            poly_data_Z[bound] = Z ;
            switch (processType) {
                case CurveSettings::eProcess_Inclination:
                case CurveSettings::eProcess_Declination:
                case CurveSettings::eProcess_Unknwon_Dec:
                case CurveSettings::eProcess_Spherical:
                case CurveSettings::eProcess_Vector:
                    poly_data_X_err[bound] = errX/1.96 ;
                    poly_data_Y_err[bound] = errY/1.96 ;
                    poly_data_Z_err[bound] = errZ/1.96 ;
                    break;

                case CurveSettings::eProcess_Univariate:
                case CurveSettings::eProcess_Depth:
                case CurveSettings::eProcess_Field:
                case CurveSettings::eProcess_2D:
                case CurveSettings::eProcess_3D:
                default:
                    poly_data_X_err[bound] = errX/1.96 ;
                    poly_data_Y_err[bound] = errY/1.96 ;
                    poly_data_Z_err[bound] = errZ/1.96 ;
                    break;
            }



        } else {

            const QJsonArray &dates = sEvent.value(STATE_EVENT_DATES).toArray();
            QColor penColor;
            for (const auto&& date : dates) {
                Date d (date.toObject());
                if (mUsePluginColor) {
                    penColor = d.mPlugin->getColor();

                } else if (mUseEventColor) {
                    penColor = event_color;

                } else {
                    penColor = mCurveCustomColor;

                }
                brushColor = penColor;
                brushColor.setAlpha(170);

                if (d.mIsValid && d.mCalibration!=nullptr && !d.mCalibration->mVector.empty()) {

                    d.autoSetTiSampler(true); // needed if calibration is not done

                    const std::map<double, double> &calibMap = d.getFormatedCalibMap();
                    // hpd is calculate only on the study Period

                    const std::map<double, double> &subData = getMapDataInRange(calibMap, mTminDisplay, mTmaxDisplay);

                    if (!subData.empty()) {

                        // hpd results
                        QList<QPair<double, QPair<double, double> > > intervals;
                        create_HPD_by_dichotomy(subData, intervals, thres);


                        CurveRefPts::PointType typePts;
                        tmin = intervals.first().second.first;
                        tmax = intervals.last().second.second;

                        poly_data_X[(tmin+tmax)/2.] = X ;
                        poly_data_Y[(tmin+tmax)/2.] = Y ;
                        poly_data_Z[(tmin+tmax)/2.] = Z ;
                        switch (processType) {
                            case CurveSettings::eProcess_Inclination:
                            case CurveSettings::eProcess_Declination:
                            case CurveSettings::eProcess_Unknwon_Dec:
                            case CurveSettings::eProcess_Spherical:
                            case CurveSettings::eProcess_Vector:
                                poly_data_X_err[(tmin+tmax)/2.] = errX/1.96 ;
                                poly_data_Y_err[(tmin+tmax)/2.] = errY/1.96 ;
                                poly_data_Z_err[(tmin+tmax)/2.] = errZ/1.96 ;
                                break;

                            case CurveSettings::eProcess_Univariate:
                            case CurveSettings::eProcess_Depth:
                            case CurveSettings::eProcess_Field:
                            case CurveSettings::eProcess_2D:
                            case CurveSettings::eProcess_3D:
                            default:
                                poly_data_X_err[(tmin+tmax)/2.] = errX/1.96 ;
                                poly_data_Y_err[(tmin+tmax)/2.] = errY/1.96 ;
                                poly_data_Z_err[(tmin+tmax)/2.] = errZ/1.96 ;
                                break;
                        }


                        if (intervals.size() == 1) {
                            //typePts = CurveRefPts::eCross;
                            typePts = processType == CurveSettings::eProcess_None ?  CurveRefPts::eLine : CurveRefPts::eCross;


                        } else {

                            //typePts = CurveRefPts::eDotLineCross;
                            typePts = processType == CurveSettings::eProcess_None ?  CurveRefPts::eDotLine : CurveRefPts::eDotLineCross;
                            ptsX.Xmin = tmin;
                            ptsX.Xmax = tmax;

                            if (ptsX.Xmin > ptsX.Xmax)
                                std::swap(ptsX.Xmin, ptsX.Xmax);

                            ptsX.Ymin = X - errX;
                            ptsX.Ymax = X + errX;
                            ptsX.color = penColor;
                            ptsX.type = typePts;
                            ptsX.pen = QPen(Qt::black, 2, Qt::SolidLine);
                            ptsX.brush = Qt::black;
                            ptsX.name = "Ref Points";
                            ptsX.comment = sEvent.value(STATE_NAME).toString();
                            ptsX.setVisible(true);
                            curveDataPointsX.push_back(ptsX);

                            ptsY = ptsX;
                            ptsY.Ymin = Y - errY;
                            ptsY.Ymax = Y + errY;
                            ptsY.pen = QPen(Qt::black, 2, Qt::SolidLine);
                            ptsY.brush = Qt::black;
                            ptsY.name = "Ref Points";
                            ptsY.setVisible(true);
                            curveDataPointsY.push_back(ptsY);


                            ptsZ = ptsX;
                            ptsZ.Ymin = Z - errZ;
                            ptsZ.Ymax = Z + errZ;
                            ptsZ.pen = QPen(Qt::black, 2, Qt::SolidLine);
                            ptsZ.brush = Qt::black;
                            ptsZ.name = "Ref Points";
                            ptsZ.setVisible(true);
                            curveDataPointsZ.push_back(ptsZ);

                            // to trace HPD
                            typePts = CurveRefPts::eLine;
                        }
                        // Trace HPD
                        for (const auto& h : intervals) {
                            tmin = h.second.first;
                            tmax = h.second.second;

                            ptsX.Xmin = tmin;
                            ptsX.Xmax = tmax;

                            if (ptsX.Xmin > ptsX.Xmax)
                                std::swap(ptsX.Xmin, ptsX.Xmax);

                            ptsX.Ymin = X - errX;
                            ptsX.Ymax = X + errX;
                            ptsX.color = penColor;
                            ptsX.type = typePts;
                            ptsX.pen = QPen(Qt::black, 2, Qt::SolidLine);
                            ptsX.brush = Qt::black;
                            ptsX.name = "Ref Points";
                            ptsX.comment = sEvent.value(STATE_NAME).toString();
                            ptsX.setVisible(true);
                            curveDataPointsX.push_back(ptsX);

                            ptsY = ptsX;
                            ptsY.Ymin = Y - errY;
                            ptsY.Ymax = Y + errY;
                            ptsY.pen = QPen(Qt::black, 2, Qt::SolidLine);
                            ptsY.brush = Qt::black;
                            ptsY.name = "Ref Points";
                            ptsY.setVisible(true);
                            curveDataPointsY.push_back(ptsY);

                            ptsZ = ptsX;
                            ptsZ.Ymin = Z - errZ;
                            ptsZ.Ymax = Z + errZ;
                            ptsZ.pen = QPen(Qt::black, 2, Qt::SolidLine);
                            ptsZ.brush = Qt::black;
                            ptsZ.name = "Ref Points";
                            ptsZ.setVisible(true);
                            curveDataPointsZ.push_back(ptsZ);

                        }

                    }


                }
            }
        }

    }


    switch (processType) {
        case CurveSettings::eProcess_Vector:
        case CurveSettings::eProcess_3D:
            graph3->set_points(curveDataPointsZ);
            graph3->setTipYLab(cs.Z_short_name());
    [[fallthrough]];
        case CurveSettings::eProcess_Spherical:
        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_2D:
            graph2->set_points(curveDataPointsY);
            graph2->setTipYLab(cs.Y_short_name());
    [[fallthrough]];
        default:
            graph1->setTipYLab(cs.X_short_name());
            graph1->set_points(curveDataPointsX);

            if (processType == CurveSettings::eProcess_None) {
                graph1->autoAdjustYScale(false);
                double Ymin = +INFINITY;
                double Ymax = -INFINITY;
                for (const auto &ptX : curveDataPointsX) {
                    Ymin = std::min(Ymin, ptX.Ymin);
                    Ymax = std::max(Ymax, ptX.Ymax);
                }
                const double min_plot = Ymin - 0.05*(Ymax-Ymin);
                const double max_plot = Ymax + 0.05*(Ymax-Ymin);
                graph1->setRangeY(min_plot, max_plot);
                graph1->setYAxisMode(GraphView::eMinMaxHidden);

            } else {
                graph1->autoAdjustYScale(true);
                graph1->setYAxisMode(GraphView::eAllTicks);

            }
            graph1->showYAxisSubTicks(processType != CurveSettings::eProcess_None);
            break;
    }

        // Print graphics in the right order
        switch (processType) {
        case CurveSettings::eProcess_Vector:
        case CurveSettings::eProcess_3D:
            graphList.append(new GraphTitle(cs.XLabel(), this));
            graphList.append(graph1);
            listAxisVisible.push_back(true);

            graphList.append(new GraphTitle(cs.YLabel(), this));
            graphList.append(graph2);
            listAxisVisible.push_back(true);

            graphList.append(new GraphTitle(cs.ZLabel(), this));
            graphList.append(graph3);
            listAxisVisible.push_back(true);
            break;

        case CurveSettings::eProcess_Spherical:
        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_2D:
            graphList.append(new GraphTitle(cs.XLabel(), this));
            graphList.append(graph1);
            listAxisVisible.push_back(true);

            graphList.append(new GraphTitle(cs.YLabel(), this));
            graphList.append(graph2);
            listAxisVisible.push_back(true);
            break;
        default:
            graphList.append(new GraphTitle(cs.XLabel(), this));
            graphList.append(graph1);
            listAxisVisible.push_back(true);
            break;
        }

    MultiCalibrationDrawing* scatterPlot = new MultiCalibrationDrawing(this);

    // must be put at the end to print the points above

    scatterPlot->showMarker();
    scatterPlot->setEventsColorList(colorList);
    scatterPlot->setListAxisVisible(listAxisVisible);

    scatterPlot->setGraphList(graphList); // must be after setEventsColorList() and setListAxisVisible()

    scatterPlot->setGraphHeight(3*mGraphHeight);
    graph1 = nullptr;
    graph2 = nullptr;
    graph3 = nullptr;

    return std::move(scatterPlot);

}


MultiCalibrationDrawing* MultiCalibrationView::fitPlot(const double thres)
{
    const QJsonObject* state = getState_ptr();

    //mSettings = StudyPeriodSettings::fromJson(state->value(STATE_SETTINGS).toObject());

    //mTminDisplay = mSettings.getTminFormated() ;
    //mTmaxDisplay = mSettings.getTmaxFormated();

    // setText doesn't emit signal textEdited, when the text is changed programmatically
    //mStartEdit->setText(locale().toString(mTminDisplay));
    //mEndEdit->setText(locale().toString(mTmaxDisplay));
    mHPDEdit->setText(locale().toString(thres));

    //mMajorScaleEdit->setText(locale().toString(mMajorScale));
    //mMinorScaleEdit->setText(locale().toString(mMinorScale));

    QColor brushColor = mCurveCustomColor;
    brushColor.setAlpha(170);

    QList<GraphViewAbstract*> graphList;

    QList<QColor> colorList;
    QList<bool> listAxisVisible;
    const QJsonArray &events = state->value(STATE_EVENTS).toArray();

    QList<QJsonObject> selectedEvents;

    const CurveSettings cs (state->value(STATE_CURVE).toObject());
    CurveSettings::ProcessType processType = cs.mProcessType;

    for (const auto&& ev : events) {
        const QJsonObject jsonEv = ev.toObject();
        if (jsonEv.value(STATE_IS_SELECTED).toBool())
            selectedEvents.append(jsonEv);
    }

    MultiCalibrationDrawing* fitPlot = new MultiCalibrationDrawing(this);

    if (selectedEvents.size()<3) {
        QMessageBox message(QMessageBox::Warning,
                            tr("Not enough points"),
                            tr("The calculation requires at least 3 points !"),
                            QMessageBox::Ok);

        message.setWindowModality(Qt::WindowModal);
        message.exec();
        return fitPlot;
    }

    GraphView* graph1 = nullptr;
    GraphView* graph2 = nullptr;
    GraphView* graph3 = nullptr;

    std::vector<CurveRefPts> curveDataPointsX, curveDataPointsY, curveDataPointsZ;
    CurveRefPts ptsX, ptsY, ptsZ;

    switch (processType) {
    case CurveSettings::eProcess_3D:
    case CurveSettings::eProcess_Vector:

            graph3 = new GraphView(this);
            graph3->showInfos(true);

            graph3->mLegendX = DateUtils::getAppSettingsFormatStr();
            graph3->setFormatFunctX(nullptr);
            graph3->setFormatFunctY(nullptr);

            graph3->setRangeX(mTminDisplay, mTmaxDisplay);
            graph3->setCurrentX(mTminDisplay, mTmaxDisplay);
            graph3->changeXScaleDivision(mMajorScale, mMinorScale);
            graph3->setOverArrow(GraphView::eNone);
            graph3->setTipXLab("t");

            graph3->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            graph3->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

            graph3->autoAdjustYScale(true);

            graph3->setYAxisMode(GraphView::eAllTicks);
            graph3->showYAxisSubTicks(true);
            graph3->setTipYLab(cs.Z_short_name());

            graph3->setMarginTop(graph3->fontMetrics().height()/2.);

            [[fallthrough]];
    case CurveSettings::eProcess_Spherical:
    case CurveSettings::eProcess_Unknwon_Dec:
    case CurveSettings::eProcess_2D:

            graph2 = new GraphView(this);
            graph2->showInfos(true);

            graph2->mLegendX = DateUtils::getAppSettingsFormatStr();
            graph2->setFormatFunctX(nullptr);
            graph2->setFormatFunctY(nullptr);

            graph2->setRangeX(mTminDisplay, mTmaxDisplay);
            graph2->setCurrentX(mTminDisplay, mTmaxDisplay);
            graph2->changeXScaleDivision(mMajorScale, mMinorScale);
            graph2->setOverArrow(GraphView::eNone);
            graph2->setTipXLab("t");

            graph2->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            graph2->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

            graph2->autoAdjustYScale(true);

            graph2->setYAxisMode(GraphView::eAllTicks);
            graph2->showYAxisSubTicks(true);
            graph2->setTipYLab(cs.Y_short_name());
            graph2->setMarginTop(graph2->fontMetrics().height()/2.);
            [[fallthrough]];
    case CurveSettings::eProcess_None:
    case CurveSettings::eProcess_Univariate:
    case CurveSettings::eProcess_Depth:
    case CurveSettings::eProcess_Inclination:
    case CurveSettings::eProcess_Declination:
    case CurveSettings::eProcess_Field:
    default:

            graph1 = new GraphView(this);
            graph1->showInfos(true);

            graph1->mLegendX = DateUtils::getAppSettingsFormatStr();
            graph1->setFormatFunctX(nullptr);
            graph1->setFormatFunctY(nullptr);

            graph1->setRangeX(mTminDisplay, mTmaxDisplay);
            graph1->setCurrentX(mTminDisplay, mTmaxDisplay);
            graph1->changeXScaleDivision(mMajorScale, mMinorScale);
            graph1->setOverArrow(GraphView::eNone);
            graph1->setTipXLab("t");

            graph1->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
            graph1->setYAxisSupport(AxisTool::AxisSupport::eAllTip);

            graph1->autoAdjustYScale(true);

            graph1->setYAxisMode( processType == CurveSettings::eProcess_None ? GraphView::eMinMaxHidden: GraphView::eAllTicks);
            graph1->showYAxisSubTicks(processType != CurveSettings::eProcess_None);

            graph1->setTipYLab(cs.X_short_name());
            graph1->setMarginTop(graph1->fontMetrics().height()/2.);

            break;
    }

    std::vector<double> vec_t;
    std::vector<double> vec_X, vec_Y, vec_Z;
    std::vector<double> vec_X_err, vec_Y_err, vec_Z_err;

    double X, errX, Y, errY, Z, errZ; //, tmin, tmax;
    for (const auto& sEvent : selectedEvents) {
            const double xIncDepth = sEvent.value(STATE_EVENT_X_INC_DEPTH).toDouble();
            const double s_XA95Depth = sEvent.value(STATE_EVENT_SX_ALPHA95_SDEPTH).toDouble();
            const double yDec = sEvent.value(STATE_EVENT_Y_DEC).toDouble();
            const double s_Y = sEvent.value(STATE_EVENT_SY).toDouble() ;
            const double zField = sEvent.value(STATE_EVENT_Z_F).toDouble();
            const double s_ZField = sEvent.value(STATE_EVENT_SZ_SF).toDouble();

            const QColor event_color (sEvent.value(STATE_COLOR_RED).toInt(),
                               sEvent.value(STATE_COLOR_GREEN).toInt(),
                               sEvent.value(STATE_COLOR_BLUE).toInt());
            // Same calcul within ResultsView::createByCurveGraph()
            switch (processType) {
            case CurveSettings::eProcess_Univariate:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Depth:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Field:
                X = zField;
                errX = 1.96*s_ZField;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Inclination:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Declination:
                X = yDec;
                errX = 1.96*(s_XA95Depth/2.448)/ cos(xIncDepth/180*M_PI );
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Unknwon_Dec:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;
                Y = zField;
                errY = s_ZField;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_2D:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = yDec;
                errY = 1.96*s_Y;
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Spherical:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;
                Y = yDec;
                errY = errX/ cos(xIncDepth/180.*M_PI );
                Z = 0.;
                errZ = 0.;
                break;

            case CurveSettings::eProcess_Vector:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth/2.448;
                Y = yDec;
                errY = errX/ cos(xIncDepth/180.*M_PI );
                Z = zField;
                errZ = s_ZField;
                break;

            case CurveSettings::eProcess_3D:
                X = xIncDepth;
                errX = 1.96*s_XA95Depth;
                Y = yDec;
                errY = 1.96*s_Y;
                Z = zField;
                errZ = 1.96*s_ZField;
                break;

            default:
                X = - sEvent.value(STATE_ITEM_Y).toDouble();
                errX = 0.;
                Y = 0.;
                errY = 0.;
                Z = 0.;
                errZ = 0.;
                break;
            }
            QColor penColor;
            if ( Event::Type (sEvent.value(STATE_EVENT_TYPE).toInt()) == Event::eBound) {
                const double bound = sEvent.value(STATE_EVENT_KNOWN_FIXED).toDouble();

                ptsX.Xmin = DateUtils::convertToAppSettingsFormat(bound);
                ptsX.Xmax = ptsX.Xmin;

                ptsX.Ymin = X - errX;
                ptsX.Ymax = X + errX;
                if (mUsePluginColor) {
                    penColor = Qt::red;

                } else if (mUseEventColor) {
                    penColor = event_color;

                } else {
                    penColor = mCurveCustomColor;

                }
                brushColor = penColor;
                brushColor.setAlpha(170);

                ptsX.color = penColor;
                ptsX.type = CurveRefPts::eRoundLine;

                ptsX.pen = QPen(Qt::black, 1, Qt::SolidLine);
                ptsX.brush = Qt::black;
                ptsX.name = "Ref Points";
                ptsX.comment = sEvent.value(STATE_NAME).toString();
                ptsX.setVisible(true);

                curveDataPointsX.push_back(ptsX);

                ptsY = ptsX;
                ptsY.Ymin = Y - errY;
                ptsY.Ymax = Y + errY;

                ptsY.pen = QPen(Qt::black, 1, Qt::SolidLine);
                ptsY.brush = Qt::black;
                ptsY.setVisible(true);
                curveDataPointsY.push_back(ptsY);

                ptsZ = ptsX;
                ptsZ.Ymin = Z - errZ;
                ptsZ.Ymax = Z + errZ;
                ptsZ.pen = QPen(Qt::black, 1, Qt::SolidLine);
                ptsZ.brush = Qt::black;
                ptsZ.setVisible(true);
                curveDataPointsZ.push_back(ptsZ);

                vec_t.push_back(bound);
                vec_X.push_back(X);
                vec_Y.push_back(Y);
                vec_Z.push_back(Z);

                switch (processType) {
                    case CurveSettings::eProcess_Inclination:
                    case CurveSettings::eProcess_Declination:
                    case CurveSettings::eProcess_Unknwon_Dec:
                    case CurveSettings::eProcess_Spherical:
                    case CurveSettings::eProcess_Vector:
                        vec_X_err.push_back(errX/1.96);
                        vec_Y_err.push_back(errY/1.96);
                        vec_Z_err.push_back(errZ/1.96);
                        break;

                    case CurveSettings::eProcess_Univariate:
                    case CurveSettings::eProcess_Depth:
                    case CurveSettings::eProcess_Field:
                    case CurveSettings::eProcess_2D:
                    case CurveSettings::eProcess_3D:
                    default:                        
                        vec_X_err.push_back(errX/1.96);
                        vec_Y_err.push_back(errY/1.96);
                        vec_Z_err.push_back(errZ/1.96);
                        break;
                }



            } else {

            const QJsonArray &dates = sEvent.value(STATE_EVENT_DATES).toArray();

            for (const auto&& date : dates) {
                Date d (date.toObject());

                if (d.mIsValid && d.mCalibration!=nullptr && !d.mCalibration->mVector.empty()) {

                    d.autoSetTiSampler(true); // needed if calibration is not done

                    const std::map<double, double> &calibMap = d.getFormatedCalibMap();
                    // hpd is calculate only on the study Period

                    const std::map<double, double> &subData = getMapDataInRange(calibMap, mSettings.getTminFormated(), mSettings.getTmaxFormated());

                    if (!subData.empty()) {
                        CurveRefPts::PointType typePts;
                        // hpd results
                        /*
                        QList<QPair<double, QPair<double, double> > > intervals ;
                        create_HPD_by_dichotomy(subData, intervals, thres);
                        tmin = intervals.first().second.first;
                        tmax = intervals.last().second.second;
                        const double tmid = (tmin+tmax)/2.0;
                        */

                        const double tmid = DateUtils::convertToAppSettingsFormat( sample_in_Repartition_date_fixe (d, mSettings));
                        vec_t.push_back(tmid);
                        vec_X.push_back(X);
                        vec_Y.push_back(Y);
                        vec_Z.push_back(Z);
                        switch (processType) {
                            case CurveSettings::eProcess_Inclination:
                            case CurveSettings::eProcess_Declination:
                            case CurveSettings::eProcess_Unknwon_Dec:
                            case CurveSettings::eProcess_Spherical:
                            case CurveSettings::eProcess_Vector:
                            vec_X_err.push_back(errX/1.96); // errX correspond à la taille de la barre d'erreur
                            vec_Y_err.push_back(errY/1.96);
                            vec_Z_err.push_back(errZ/1.96);
                                break;

                            case CurveSettings::eProcess_Univariate:
                            case CurveSettings::eProcess_Depth:
                            case CurveSettings::eProcess_Field:
                            case CurveSettings::eProcess_2D:
                            case CurveSettings::eProcess_3D:
                            default:
                                vec_X_err.push_back(errX/1.96);
                                vec_Y_err.push_back(errY/1.96);
                                vec_Z_err.push_back(errZ/1.96);
                                break;
                        }

                        typePts = CurveRefPts::eCross;

                        ptsX.Xmin = tmid;
                        ptsX.Xmax = ptsX.Xmin;
                        if (mSilverParam.use_error_measure) {
                            ptsX.Ymin = X - errX;
                            ptsX.Ymax = X + errX;

                        } else {
                            ptsX.Ymin = X;
                            ptsX.Ymax = X;
                        }

                        QColor penColor;
                        if (mUsePluginColor) {
                            penColor = Qt::red;

                        } else if (mUseEventColor) {
                            penColor = event_color;

                        } else {
                            penColor = mCurveCustomColor;

                        }
                        brushColor = penColor;
                        brushColor.setAlpha(170);

                        ptsX.color = penColor;
                        ptsX.type = typePts;
                        ptsX.pen = QPen(Qt::black, 1, Qt::SolidLine);
                        ptsX.brush = Qt::black;
                        ptsX.name = "Ref Points";
                        ptsX.comment = sEvent.value(STATE_NAME).toString();
                        ptsX.setVisible(true);
                        curveDataPointsX.push_back(ptsX);

                        ptsY = ptsX;
                        if (mSilverParam.use_error_measure) {
                            ptsY.Ymin = Y - errY;
                            ptsY.Ymax = Y + errY;

                        } else {
                            ptsY.Ymin = Y;
                            ptsY.Ymax = Y ;
                        }
                        ptsY.pen = QPen(Qt::black, 1, Qt::SolidLine);
                        ptsY.brush = Qt::black;
                        ptsY.name = "Ref Points";
                        ptsY.setVisible(true);
                        curveDataPointsY.push_back(ptsY);

                        ptsZ = ptsX;
                        if (mSilverParam.use_error_measure) {
                            ptsZ.Ymin = Z - errZ;
                            ptsZ.Ymax = Z + errZ;

                        } else {
                            ptsZ.Ymin = Z;
                            ptsZ.Ymax = Z;
                        }
                        ptsZ.pen = QPen(Qt::black, 1, Qt::SolidLine);
                        ptsZ.brush = Qt::black;
                        ptsZ.name = "Ref Points";
                        ptsZ.setVisible(true);
                        curveDataPointsZ.push_back(ptsZ);

                    }


                }
            }
            }

    }

    // test enought 3 valid points
    bool not_enought = false;
    switch (processType) {
        case CurveSettings::eProcess_Univariate:
        case CurveSettings::eProcess_Inclination:
        case CurveSettings::eProcess_Declination:
        case CurveSettings::eProcess_Field:
        case CurveSettings::eProcess_Depth:
            if (vec_X.size()<3) {
                not_enought = true;
                break;
            }
            break;
        case CurveSettings::eProcess_2D:
        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_Spherical:
            if (vec_Y.size()<3) {
                not_enought = true;
                break;
            }
            break;

        case CurveSettings::eProcess_3D:
            if (vec_Z.size()<3) {
                not_enought = true;
                break;
            }

        default:
            break;
    }


    if (not_enought) {
        QMessageBox message(QMessageBox::Warning,
                            tr("Not enough valid dat"),
                            tr("The calculation requires at least 3 points !"),
                            QMessageBox::Ok);

        message.setWindowModality(Qt::WindowModal);
        message.exec();
        return fitPlot;
    }

    const double tmin_poly = mSettings.getTminFormated();
    const double tmax_poly = mSettings .getTmaxFormated();
    const double step_poly = (tmax_poly-tmin_poly)/1000.0;

    // transfer of information from the SilvermanDialog database via cs

    if (!mSilverParam.use_error_measure) {
        vec_X_err = std::vector<double>(vec_X_err.size(), 1.0);
        vec_Y_err = std::vector<double>(vec_Y_err.size(), 1.0);
        vec_Z_err = std::vector<double>(vec_Z_err.size(), 1.0);

    } else {


       // vec_X_err = std::vector<double>(vec_X_err.size(), 1.0/vec_X_err.size()); // pour test ici à enlever
        // test null error forbidden
        std::vector<double>::iterator it;
        bool nullValue = false;
        switch (processType) {
            case CurveSettings::eProcess_3D:
                it = std::find_if (vec_Z_err.begin(), vec_Z_err.end(), [](double i){return i == 0.0;} );
                if (it != vec_Z_err.end()) {
                    nullValue = true;
                    break;
                }
                [[fallthrough]];
            case CurveSettings::eProcess_2D:
            case CurveSettings::eProcess_Unknwon_Dec:
            case CurveSettings::eProcess_Spherical:
                it = std::find_if (vec_Y_err.begin(), vec_Y_err.end(), [](double i){return i == 0.0;} );
                if (it != vec_Y_err.end()) {
                    nullValue = true;
                    break;
                }
                [[fallthrough]];
            case CurveSettings::eProcess_Univariate:
            case CurveSettings::eProcess_Inclination:
            case CurveSettings::eProcess_Declination:
            case CurveSettings::eProcess_Field:
            case CurveSettings::eProcess_Depth:
                it = std::find_if (vec_X_err.begin(), vec_X_err.end(), [](double i){return i == 0.0;} );
                if (it != vec_X_err.end()) {
                    nullValue = true;
                    break;
                }
            break;

            default:
            break;
        }
        if (nullValue) {
            QMessageBox message(QMessageBox::Warning,
                                tr("Some errors are zero"),
                                tr("The calculation cannot be performed with zero-measurement errors, weights cannot be zero !"),
                                QMessageBox::Ok);

            message.setWindowModality(Qt::WindowModal);
            message.exec();
            return fitPlot;
        }

    }

    std::pair<MCMCSpline, std::pair<double, double>> do_spline_res;
    switch (processType) {
        case CurveSettings::eProcess_Univariate:
        case CurveSettings::eProcess_Inclination:
        case CurveSettings::eProcess_Declination:
        case CurveSettings::eProcess_Field:
        case CurveSettings::eProcess_Depth:
            // recherche du smoothing
            do_spline_res = do_spline_composante(vec_t, vec_X, vec_X_err, tmin_poly, tmax_poly, mSilverParam);
         break;

        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_2D:
        case CurveSettings::eProcess_Spherical:
            do_spline_res = do_spline_composante(vec_t, vec_X, vec_X_err, tmin_poly, tmax_poly, mSilverParam, vec_Y, vec_Y_err);
        break;

        case CurveSettings::eProcess_Vector:
        case CurveSettings::eProcess_3D:
            do_spline_res = do_spline_composante(vec_t, vec_X, vec_X_err, tmin_poly, tmax_poly, mSilverParam, vec_Y, vec_Y_err, vec_Z, vec_Z_err);
        break;

        default:
        break;
    }


    // __________________________
    // Génération des courbes
    // __________________________

    MCMCSpline spline = do_spline_res.first;
    std::pair<double, double> lambda_Vg = do_spline_res.second;
    QString spline_info;
    auto lambda = lambda_Vg.first;
    // On peut rentrer "inf" comme valeur de lambda dans la boite ce qui force l'ajustement spline
    if (mSilverParam.lambda_fixed) {
        if (lambda == 0.0) {
            spline_info =  tr("Forced Spline Fitting ;");

        } else {
            spline_info =  tr("Fixed Smoothing = 10E%1 ;").arg(locale().toString(log10(lambda)));
        }
        mSilverParam.comment = "";

    } else {

        if (lambda == 0.0) {
            spline_info = QString::fromStdString(mSilverParam.comment);//   tr(" Spline Fitting ;");

        } else if (lambda == 10.0E20) {
            spline_info = QString::fromStdString(mSilverParam.comment);//   tr(" Linear regression ;");

        } else {
            spline_info += QString::fromStdString(mSilverParam.comment) + tr(" Smoothing = 10E%1 ;").arg(locale().toString(log10(lambda)));
        }

    }
    mSilverParam.comment = "";
    if (lambda != 0.0) {
        spline_info +=  tr(" Estimated std g = %1 ").arg(locale().toString(sqrt(lambda_Vg.second)));// + QString::fromStdString(mSilverParam.comment);
    }

    if (processType == CurveSettings::eProcess_Univariate ||
        processType == CurveSettings::eProcess_Inclination ||
        processType == CurveSettings::eProcess_Declination ||
        processType == CurveSettings::eProcess_Field ||
        processType == CurveSettings::eProcess_Depth ||
        processType == CurveSettings::eProcess_2D ||
        processType == CurveSettings::eProcess_Unknwon_Dec ||
        processType == CurveSettings::eProcess_Spherical ||
        processType == CurveSettings::eProcess_Vector ||
        processType == CurveSettings::eProcess_3D) {

            const auto &curves = composante_to_curve(spline.splineX, tmin_poly, tmax_poly, step_poly);

            if (!isnan(curves[0][0]) && !isnan(curves[1][0]) && !isnan(curves[2][0])) {
                GraphCurve curve_G = FunctionCurve(curves[0], "G", QColor(119, 95, 200) );
                GraphCurve curve_GEnv = shapeCurve(curves[2], curves[1], "G Env",
                                                         QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30));
                curve_G.mVisible = true;
                curve_GEnv.mVisible = true;
                graph1->add_curve(curve_G);
                graph1->add_curve(curve_GEnv);

            }
    }

    if (processType == CurveSettings::eProcess_2D ||
        processType == CurveSettings::eProcess_Unknwon_Dec ||
        processType == CurveSettings::eProcess_Spherical ||
        processType == CurveSettings::eProcess_Vector ||
        processType == CurveSettings::eProcess_3D)  {

            const auto &curves = composante_to_curve(spline.splineY, tmin_poly, tmax_poly, step_poly);

            if (!isnan(curves[0][0]) && !isnan(curves[1][0]) && !isnan(curves[2][0])) {
                GraphCurve curve_G = FunctionCurve(curves[0], "G", QColor(119, 95, 200) );
                GraphCurve curve_GEnv = shapeCurve(curves[2], curves[1], "G Env",
                                                          QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30));
                curve_G.mVisible = true;
                curve_GEnv.mVisible = true;
                graph2->add_curve(curve_G);
                graph2->add_curve(curve_GEnv);
            }
    }

    if (processType == CurveSettings::eProcess_3D || processType == CurveSettings::eProcess_Vector) {

            const auto &curves = composante_to_curve(spline.splineZ, tmin_poly, tmax_poly, step_poly);

            if (!isnan(curves[0][0]) && !isnan(curves[1][0]) && !isnan(curves[2][0])) {

                GraphCurve curve_G = FunctionCurve(curves[0], "G", QColor(119, 95, 200) );
                GraphCurve curve_GEnv = shapeCurve(curves[2], curves[1], "G Env",
                                                          QColor(180, 180, 180), Qt::DashLine, QColor(180, 180, 180, 30));
                curve_G.mVisible = true;
                curve_GEnv.mVisible = true;
                graph3->add_curve(curve_G);
                graph3->add_curve(curve_GEnv);
            }

    }

    // __________________________
    // Génération des points des graphes
    // __________________________

    switch (processType) {
        case CurveSettings::eProcess_Vector:
        case CurveSettings::eProcess_3D:
            graph3->set_points(curveDataPointsZ);
            graph3->setTipYLab(cs.Z_short_name());
            [[fallthrough]];
        case CurveSettings::eProcess_Spherical:
        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_2D:
            graph2->set_points(curveDataPointsY);
            graph2->setTipYLab(cs.Y_short_name());
            [[fallthrough]];
        default:
            graph1->setTipYLab(cs.X_short_name());
            graph1->set_points(curveDataPointsX);
            graph1->addInfo(spline_info);
            graph1->showInfos(true);

            graph1->setYAxisMode( processType == CurveSettings::eProcess_None ? GraphView::eMinMaxHidden: GraphView::eAllTicks);
            graph1->showYAxisSubTicks(processType != CurveSettings::eProcess_None);
        break;
    }

    // __________________________
    // Print graphics in the right order
    // __________________________

    switch (processType) {
        case CurveSettings::eProcess_Vector:
        case CurveSettings::eProcess_3D:
            graphList.append(new GraphTitle(cs.XLabel(), this));
            graphList.append(graph1);
            listAxisVisible.push_back(true);

            graphList.append(new GraphTitle(cs.YLabel(), this));
            graphList.append(graph2);
            listAxisVisible.push_back(true);

            graphList.append(new GraphTitle(cs.ZLabel(), this));
            graphList.append(graph3);
            listAxisVisible.push_back(true);
        break;

        case CurveSettings::eProcess_Spherical:
        case CurveSettings::eProcess_Unknwon_Dec:
        case CurveSettings::eProcess_2D:
            graphList.append(new GraphTitle(cs.XLabel(), this));
            graphList.append(graph1);
            listAxisVisible.push_back(true);

            graphList.append(new GraphTitle(cs.YLabel(), this));
            graphList.append(graph2);
            listAxisVisible.push_back(true);
        break;

        default:
            graphList.append(new GraphTitle(cs.XLabel(), this));
            graphList.append(graph1);
            listAxisVisible.push_back(true);
        break;
        }


    // must be put at the end to print the points above

    fitPlot->showMarker();
    fitPlot->setEventsColorList(colorList);
    fitPlot->setListAxisVisible(listAxisVisible);

    fitPlot->setGraphList(graphList); // must be after setEventsColorList() and setListAxisVisible()

    fitPlot->setGraphHeight(3*mGraphHeight);
    graph1 = nullptr;
    graph2 = nullptr;
    graph3 = nullptr;

    return std::move(fitPlot);

}

void MultiCalibrationView::updateMultiCalib()
{
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

    if (mStatClipBut ->isChecked()) {
        showStat();

    } else if (mScatterClipBut->isChecked()) {
        delete mDrawing;
        mDrawing = scatterPlot(mThreshold);
        updateGraphsZoom(); // update Y scale
        updateLayout();

    } else if (mFitClipBut->isChecked()) {
        delete mDrawing;
        mDrawing = fitPlot(mThreshold);
        updateGraphsZoom(); // update Y scale
        updateLayout();

    } else {
        QList<GraphView*> graphList = mDrawing->getGraphViewList();
        GraphCurve* calibCurve (nullptr);
        GraphCurve* hpdCurve (nullptr);
        for (GraphView* gr : graphList) {
            calibCurve = gr->getCurve("Calibration");
            // there is curve named "Calibration" in a Bound
            if (calibCurve) {
                // hpd is calculate only on the study Period
                QMap<type_data, type_data> subData = calibCurve->mData;
                subData = getMapDataInRange(subData, mSettings.getTminFormated(), mSettings.getTmaxFormated());

                QList<QPair<double, QPair<double, double> > > formated_intervals;
                const QMap<double, double> &hpd = QMap<double, double>(create_HPD_by_dichotomy(subData, formated_intervals, mThreshold));

                hpdCurve = gr->getCurve("Calibration HPD");
                hpdCurve->mData = hpd;

                gr->forceRefresh();
            }
        }
        calibCurve = nullptr;
        hpdCurve = nullptr;
    }


}

void MultiCalibrationView::updateGraphsSize(const QString &sizeStr)
{
    bool ok;
    const double val = locale().toDouble(sizeStr, &ok);
    if (ok) {
        const double origin = mHeightForVisibleAxis;
        const double prop =  val / 100.;

        mGraphHeight = prop * origin;
        mYZoom->setProp(prop /2.0, false);

    } else
        return;

    if (mDrawing) {
        if (mScatterClipBut->isChecked() || mFitClipBut->isChecked())
            mDrawing->setGraphHeight(3*mGraphHeight);
        else
            mDrawing->setGraphHeight(mGraphHeight);

        mDrawing->updateLayout();
    }

}

void MultiCalibrationView::updateYZoom(const double prop)
{
    const double origin = mHeightForVisibleAxis;

    mGraphHeight = int ( prop * origin * 2);

    const QString sizeText = QLocale().toString(mGraphHeight / origin * 100, 'f', 0);
    mGraphHeightEdit->blockSignals(true);
    mGraphHeightEdit->setText(sizeText);
    mGraphHeightEdit->blockSignals(false);

    if(mDrawing) {
        if (mScatterClipBut->isChecked() || mFitClipBut->isChecked())
            mDrawing->setGraphHeight(3*mGraphHeight);
        else
            mDrawing->setGraphHeight(mGraphHeight);

        mDrawing->updateLayout();
    }

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

    qreal textSize = fontMetrics().horizontalAdvance(mStartEdit->text()) + fontMetrics().horizontalAdvance("0");
    if (textSize > mStartEdit->width()) {
        const qreal fontRate = textSize / mStartEdit->width();
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mStartEdit->setFont(adaptedFont);

    } else
        mStartEdit->setFont(font());

    adaptedFont = font();

    textSize = fontMetrics().horizontalAdvance(mEndEdit->text()) + fontMetrics().horizontalAdvance("0");
    if (textSize > mEndEdit->width() ) {
        const qreal fontRate = textSize / mEndEdit->width();
        const qreal ptSiz = std::max(adaptedFont.pointSizeF() / fontRate, 1.);
        adaptedFont.setPointSizeF(ptSiz);
        mEndEdit->setFont(adaptedFont);
    } else
        mEndEdit->setFont(font());

    // Usefull when we set mStartEdit and mEndEdit at the begin of the display,
    // after a call to setDate
    if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay))
        return;
    else if (mTminDisplay < mTmaxDisplay)
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

    qreal textSize = fontMetrics().horizontalAdvance(str)  + fontMetrics().horizontalAdvance("0");
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

    textSize = fontMetrics().horizontalAdvance(str)  + fontMetrics().horizontalAdvance("0");
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
        const QList<GraphView*> &graphList = mDrawing->getGraphViewList();

        for (GraphView* gr : graphList) {
            if (!gr->has_curves() && !gr->has_points())
                continue;
            gr->changeXScaleDivision(mMajorScale, mMinorScale);
        }

    } else
        return;

}

void MultiCalibrationView::updateGraphsZoom()
{
    QFontMetricsF fm (mDrawing->font());
    //The same Name and same Value as in MultiCalibrationView::exportFullImage()
    mMarginRight = int (1.5 * floor(fm.horizontalAdvance(stringForGraph(mTmaxDisplay))/2) + 5);

    const QList<GraphView*> &graphList = mDrawing->getGraphViewList();
    qreal maxYLength = 0;

    const bool is_curve = getProject_ptr()->isCurve();
    for (GraphView* gr : graphList) {

        if (gr->has_curves() || gr->has_points()) {

            gr->setRangeX(mTminDisplay, mTmaxDisplay);
            gr->setCurrentX(mTminDisplay, mTmaxDisplay);

            // update max inside the display period (mTminDisplay, mTmaxDisplay)
            // Bound doesn't have curve named "Calibration", this curve name is "Bound"

            const GraphCurve* calibCurve = gr->getCurve("Calibration");
            if (calibCurve) {
                type_data yMax;
                const QMap<type_data, type_data> &subDisplay = getMapDataInRange(calibCurve->mData, mTminDisplay, mTmaxDisplay);
                if (subDisplay.isEmpty()) {
                    yMax = 0;
                } else {
                    yMax = map_max(subDisplay).value();
                }

                GraphCurve* wiggleCurve = gr->getCurve("Wiggle");
                if (wiggleCurve) {
                    const QMap<type_data, type_data> &subDisplayWiggle = getMapDataInRange(wiggleCurve->mData, mTminDisplay, mTmaxDisplay);
                    if (!subDisplayWiggle.isEmpty()) {
                        yMax = std::max(yMax, map_max(subDisplayWiggle).value());
                    }
                }

                if (yMax == 0.)
                    yMax = 1;

                gr->setRangeY(0., yMax);
            }

           if (gr->has_points()) {
                if (gr->autoAdjustY()) {
                    double yMin = gr->refPoints.at(0).Ymin;
                    double yMax = gr->refPoints.at(0).Ymax;

                    for (const auto& refP : gr->refPoints) {
                       yMin = std::min(yMin, refP.Ymin);
                       yMax = std::max(yMax, refP.Ymax);
                    }


                    Scale yScale;
                    yScale.findOptimal(yMin, yMax, 10);

                    if (is_curve) {
                        maxYLength = std::max({fm.horizontalAdvance(stringForGraph(yScale.max)),
                                               fm.horizontalAdvance(stringForGraph(yScale.min)),
                                               fm.horizontalAdvance(stringForGraph(yScale.min - yScale.mark)),
                                               fm.horizontalAdvance(stringForGraph(yScale.max + yScale.mark))
                                              });
                    }

                    gr->setRangeY(yMin, yMax);
                }
            }



            // ------------------------------------------------------------
            //  Show zones if calibrated data are outside study period
            // ------------------------------------------------------------
            gr->remove_all_zones();
            if (mTminDisplay < mSettings.getTminFormated()) {
                GraphZone zone;
                zone.mXStart = mTminDisplay;
                zone.mXEnd = mSettings.getTminFormated();
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mText = tr("Outside study period");
                gr->add_zone(zone);
            }
            if (mTmaxDisplay > mSettings.getTmaxFormated()) {
                GraphZone zone;
                zone.mXStart = mSettings.getTmaxFormated();
                zone.mXEnd = mTmaxDisplay;
                zone.mColor = QColor(217, 163, 69);
                zone.mColor.setAlpha(75);
                zone.mText = tr("Outside study period");
                gr->add_zone(zone);
            }

            calibCurve = nullptr;
        }
    }

    if ((mScatterClipBut->isChecked() || mFitClipBut->isChecked()) && is_curve)
        mMarginLeft = 1.5 * maxYLength + 10;
    else
        mMarginLeft = 1.3 * fm.horizontalAdvance(stringForGraph(mTminDisplay))/2. + 10;

    for (GraphViewAbstract* gr : *mDrawing->getGraphList()) {
        GraphView* gv = dynamic_cast<GraphView*>(gr);
        GraphTitle* gt = dynamic_cast<GraphTitle*>(gr);

        if (gv) {
            if (gv->has_curves() || gv->has_points()) {
                gv->setMarginRight(mMarginRight);
                gv->setMarginLeft(mMarginLeft);
            }
            gv->forceRefresh();

        } else if (gt) {
            gt->setMarginRight(mMarginRight);
            gt->setMarginLeft(mMarginLeft);
            gt->repaintGraph(true);
        }
        gv = nullptr;
        gt = nullptr;
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
    bool printAxis = (mGraphHeight < mHeightForVisibleAxis);
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
            axisWidget = nullptr;
        }
        if (axisLegend) {
            axisLegend->setParent(nullptr);
            delete axisLegend;
            axisLegend = nullptr;
        }
        widgetExport->resize(widgetExport->width() ,widgetExport->height() - axeHeight - legendHeight);
    } else
        widgetExport->resize(widgetExport->width() ,widgetExport->height() - legendHeight);

    widgetExport = nullptr;
    // Revert to default display :

    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    updateLayout();
}


void MultiCalibrationView::copyImage()
{
    if (mStatClipBut->isChecked()) {
        QApplication::clipboard()->setText(mStatArea->toPlainText());

    } else {
        mDrawing->hideMarker();
        mDrawing->repaint();
        QApplication::clipboard()->setPixmap(mDrawing->grab());
        mDrawing->showMarker();
    }
}

void MultiCalibrationView::changeCurveColor()
{
    MultiplotColorDialog dialog (mUsePluginColor, mUseEventColor, mCurveCustomColor);

    if (dialog.exec() == QDialog::Accepted) {

        mUsePluginColor = dialog.usePluginColor();
        mUseEventColor = dialog.useEventColor();
        mUseCustomColor = !mUsePluginColor & !mUseEventColor;
        mCurveCustomColor = dialog.getColor();

    }

    updateGraphList();

}

void MultiCalibrationView::copyText()
{
    QApplication::clipboard()->setText(mResultText.replace("<br>", "\r"));
}

void MultiCalibrationView::exportResults()
{
    const QString csvSep = AppSettings::mCSVCellSeparator;
    QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;

    csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);

    if (mFitClipBut->isChecked()) {
        save_map_as_csv(mSilverParam.tab_GCV, std::make_pair("lambda", "GCV"), "Save GCV ...", "GCV");
        save_map_as_csv(mSilverParam.tab_CV, std::make_pair("lambda", "CV"), "Save CV ...", "CV");
        // Export curves
        const QList<GraphView*> graphList = mDrawing->getGraphViewList();
        int i = 1;
        for (GraphView* gr : graphList) {
            if (gr->has_curves())
                gr->exportCurrentCurves (MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, 0, tr("Save Silverman Fit %1").arg(QString::number(i++)));
        }

        return;
    }

    const QString currentPath = MainWindow::getInstance()->getCurrentPath();
    const QString filePath = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                          tr("Export to file..."),
                                                          currentPath, "CSV (*.csv)");
    if (!filePath.isEmpty()) {

        const QJsonObject* state = getState_ptr();
        const bool isCurve = getProject_ptr()->isCurve();
        const CurveSettings::ProcessType processType = static_cast<CurveSettings::ProcessType>(state->value(STATE_CURVE).toObject().value(STATE_CURVE_PROCESS_TYPE).toInt());

        const QJsonArray &events = state->value(STATE_EVENTS).toArray();
        QList<QJsonObject> selectedEvents;

        for (auto &&ev : events) {
            QJsonObject jsonEv = ev.toObject();
            if (jsonEv.value(STATE_IS_SELECTED).toBool())
                selectedEvents.append(jsonEv);
        }

        // Sort Event by Position
        std::sort(selectedEvents.begin(), selectedEvents.end(), [] (QJsonObject ev1, QJsonObject ev2) {return (ev1.value(STATE_ITEM_Y).toDouble() < ev2.value(STATE_ITEM_Y).toDouble());});

        QList<QStringList> stats ;
        QStringList header;
        header << "Event";
        if (isCurve) {
            switch (processType) {
            case CurveSettings::eProcess_Vector:
                header << "Inclination";
                header << "alpha95";
                header << "Declination";
                header << "Field";
                header << "F Error";
                break;
            case CurveSettings::eProcess_2D:
                header << "X";
                header << "X Error";
                header << "Y";
                header << "Y Error";
                break;
            case CurveSettings::eProcess_3D:
                header << "X";
                header << "X Error";
                header << "Y";
                header << "Y Error";
                header << "Z";
                header << "Z Error";
                break;
            case CurveSettings::eProcess_Unknwon_Dec:
                header << "Inclination";
                header << "alpha95";
                header << "Field";
                header << "F Error";
                break;
            case CurveSettings::eProcess_Spherical:
                header << "Inclination";
                header << "alpha95";
                header << "Declination";
                break;

            case CurveSettings::eProcess_Depth:
                header << "Depth";
                header << "Depth Error";
                break;
            case CurveSettings::eProcess_Field:
                header << "Field";
                header << "Field Error";
                break;
            case CurveSettings::eProcess_Inclination:
                header << "Inclination";
                header << "alpha95";
                break;
            case CurveSettings::eProcess_Declination:
                header << "Declination";
                header << "alpha95";
                header << "Inclination";
                break;
            case CurveSettings::eProcess_Univariate:
                header << "Measure";
                header << "Error";
                break;
            default:
                break;
            }
        }
        header <<"Data type" << "Data Name" << "Data Description" << "MAP" << "Mean" << "Std";
        header <<"Q1" <<"Q2" << "Q3"<<"HPD total %";

        int maxHpd = 2;
        QList<double> curveParam;
        for (auto &&ev : selectedEvents) {

            // Insert the Event's Name only if different to the previous Event's name

            const QString eventName (ev.value(STATE_NAME).toString());
            QStringList curveParamList;
            if (isCurve) {
                curveParam = Event::curveParametersFromJsonEvent(ev, processType);
                for (auto param : curveParam) {
                    curveParamList.append(locale().toString(param));
                }
            }


            if ( Event::Type (ev.value(STATE_EVENT_TYPE).toInt()) == Event::eBound) {
                const double bound = ev.value(STATE_EVENT_KNOWN_FIXED).toDouble();
                QStringList statLine;
                statLine<<eventName;
                if (isCurve) {
                    statLine.append(curveParamList);
                }
                //statLine<<"Bound"<< locale().toString(bound) + " BC/AD";
                statLine<<"Bound"<< DateUtils::convertToAppSettingsFormatStr(bound, true);// + " " + DateUtils::getAppSettingsFormatStr();
                stats.append(statLine);

            } else {
                const QJsonArray dates = ev.value(STATE_EVENT_DATES).toArray();

                for (const auto date : dates) {
                    const QJsonObject &jdate = date.toObject();

                    Date d (jdate);
                    QStringList statLine;
                    statLine << eventName;
                    if (isCurve) {
                        statLine.append(curveParamList);
                    }
                    statLine << d.mPlugin->getName() << d.getQStringName() << QChar(34) + d.getDesc() + QChar(34); // For CSV mode, text recognition

                    const bool isUnif = false;// (d.mPlugin->getName() == "Unif");
                    if (d.mIsValid && !d.mCalibration->mVector.empty() && !isUnif) {

                        // d.autoSetTiSampler(true); // needed if calibration is not done
                        const std::map<double, double> &calibMap = d.getFormatedCalibMap();

                        // hpd is calculate only on the study Period
                        std::map<double, double> periodCalib = getMapDataInRange(calibMap, mSettings.getTminFormated(), mSettings.getTmaxFormated());
                        periodCalib = equal_areas(periodCalib, 1.);
                        DensityAnalysis results;
                        results.funcAnalysis = analyseFunction(periodCalib);

                        if (!periodCalib.empty()) {

                            statLine << csvLocal.toString(results.funcAnalysis.mode) << csvLocal.toString(results.funcAnalysis.mean) << csvLocal.toString(results.funcAnalysis.std);
                            statLine << csvLocal.toString(results.funcAnalysis.quartiles.Q1) << csvLocal.toString(results.funcAnalysis.quartiles.Q2) << csvLocal.toString(results.funcAnalysis.quartiles.Q3);

                            // hpd results
                            QList<QPair<double, QPair<double, double> > > formated_intervals;
                            create_HPD_by_dichotomy(periodCalib, formated_intervals, mThreshold);
                            const QString hpdStr = get_HPD_text(formated_intervals, DateUtils::getAppSettingsFormatStr(), nullptr, true);

                            const int nh = int(hpdStr.count(AppSettings::mCSVCellSeparator));
                            maxHpd = std::max(maxHpd, nh);
                            const double real_thresh = std::accumulate(formated_intervals.begin(), formated_intervals.end(), 0., [](double sum, QPair<double, QPair<double, double> > p) {return sum + p.first;});

                            statLine << csvLocal.toString(100. *real_thresh, 'f', 1) << hpdStr;

                        }

                    } else
                        statLine << "Stat Not Computable";

                    stats.append(statLine);
                }
            }

        }
        //
        int nbHPD = (maxHpd + 1)/3;
        for (int i = 0; i < nbHPD; ++i) {
            header << "HPD" + QString::number(i + 1) + " begin";
            header << "HPD" + QString::number(i + 1) + " end";
            header << "HPD" + QString::number(i + 1) + " %";
        }
        stats.insert(0, header);
        // _____________

        saveCsvTo(stats, filePath, csvSep, true);

    }

}

void MultiCalibrationView::showStat()
{
    if (mStatClipBut ->isChecked()) {
        if (mDrawing)
            mDrawing->setVisible(false);

        mStatArea->setVisible(true);
        mStatArea->setFont(font());

        // update Results from selected Event in JSON
        const QJsonObject* state = getState_ptr();
        const QJsonArray events = state->value(STATE_EVENTS).toArray();
        QList<QJsonObject> selectedEvents;

        const bool curveModel = getProject_ptr()->isCurve();
        const CurveSettings::ProcessType processType = static_cast<CurveSettings::ProcessType>(state->value(STATE_CURVE).toObject().value(STATE_CURVE_PROCESS_TYPE).toInt());

        for (auto&& ev : events) {
            QJsonObject jsonEv = ev.toObject();
            if (jsonEv.value(STATE_IS_SELECTED).toBool())
                selectedEvents.append(jsonEv);
        }

        // Sort Event by Position
        std::sort(selectedEvents.begin(), selectedEvents.end(), [] (QJsonObject ev1, QJsonObject ev2) {return (ev1.value(STATE_ITEM_Y).toDouble() < ev2.value(STATE_ITEM_Y).toDouble());});

        mResultText = "";
        for (auto& ev : selectedEvents) {

            const QString curveDescription = curveModel ? Event::curveDescriptionFromJsonEvent(ev, processType): "";

            // Insert the Event's Name only if different to the previous Event's name
            QString eventName (ev.value(STATE_NAME).toString() + " " + curveDescription);
            const QColor color = QColor(ev.value(STATE_COLOR_RED).toInt(),
                                        ev.value(STATE_COLOR_GREEN).toInt(),
                                        ev.value(STATE_COLOR_BLUE).toInt());
            const QColor nameColor = getContrastedColor(color);
            QString resultsStr = textBackgroundColor("<big>" + textColor(eventName, nameColor) + "</big> ", color);

            if ( Event::Type (ev.value(STATE_EVENT_TYPE).toInt()) == Event::eBound) {
                const double bound = ev.value(STATE_EVENT_KNOWN_FIXED).toDouble();
                //resultsStr += " <br><strong>"+ tr("Bound : %1").arg(locale().toString(bound)) +" BC/AD </strong><br>";
                resultsStr += " <br><strong>"+ tr("Bound : %1").arg(DateUtils::convertToAppSettingsFormatStr(bound)) +" " + DateUtils::getAppSettingsFormatStr() + "</strong><br>";

            } else {
                const QJsonArray dates = ev.value(STATE_EVENT_DATES).toArray();
                bool firstDate = true;

                for (auto&& date : dates) {
                    const QJsonObject jdate = date.toObject();

                    Date d (jdate);
                    // Adding a line between date result
                    if (firstDate) {
                        firstDate = false;
                    } else {
                        resultsStr += "<hr>";
                    }

                    resultsStr += "<br><strong>" + d.getQStringName() + "</strong> (" + d.mPlugin->getName() + ")" +"<br> <i>" + d.getDesc() + "</i><br> ";

                    if (d.mIsValid && d.mCalibration!=nullptr && !d.mCalibration->mVector.empty()) {

                        const bool isUnif = false; //(d.mPlugin->getName() == "Unif");

                        if (!isUnif) {
                            d.autoSetTiSampler(true); // needed if calibration is not done

                            const std::map<double, double> &calibMap = d.getFormatedCalibMap();
                            // hpd is calculate only on the study Period

                            std::map<double, double> subData = getMapDataInRange(calibMap, mSettings.getTminFormated(), mSettings.getTmaxFormated());
                            subData = equal_areas(subData, 1.);
                            DensityAnalysis results;
                            results.funcAnalysis = analyseFunction(subData);

                            if (!subData.empty()) {

                                resultsStr += "<br>" + FunctionStatToString(results.funcAnalysis);

                                // hpd results
                                QList<QPair<double, QPair<double, double> > > formated_intervals;
                                create_HPD_by_dichotomy(subData, formated_intervals, mThreshold);

                                const double real_thresh = std::accumulate(formated_intervals.begin(), formated_intervals.end(), 0., [](double sum, QPair<double, QPair<double, double> > p) {return sum + p.first;});

                                resultsStr += + "<br> HPD (" + locale().toString(100. * real_thresh, 'f', 1) + "%) : " + get_HPD_text(formated_intervals, DateUtils::getAppSettingsFormatStr(), nullptr) + "<br>";

                            } else
                                resultsStr += "<br>" + textBold(textRed(QObject::tr("Solutions exist outside study period") ))  + "<br>";

                        }

                    } else
                        resultsStr += + "<br> HPD  : " + tr("Not  computable")+ "<br>";

                }
            }
            mResultText += resultsStr ;
        }

        mStatArea->setHtml(mResultText);


    } else {
        updateGraphList(); // update appSettings
        mDrawing->setVisible(true);
        mStatArea->setVisible(false);
    }

   updateLayout();
}

void MultiCalibrationView::showScatter()
{
    if (mFitClipBut->isChecked())
        mFitClipBut->setCheckState(false);
    updateGraphList();
}

void MultiCalibrationView::showFit()
{
    if (mFitClipBut->isChecked()) {
        SilvermanDialog silverWindows (&mSilverParam, this);
        if (silverWindows.exec() == QDialog::Rejected) {
            mFitClipBut->setCheckState(false);
            return;
        }
        if (mScatterClipBut->isChecked())
            mScatterClipBut->setCheckState(false);
    }
    updateGraphList();
}

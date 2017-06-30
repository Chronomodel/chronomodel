#include "MultiCalibrationView.h"

#include "DoubleValidator.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"

#include <QPainter>
#include "Project.h"
#include <QJsonArray>

MultiCalibrationView::MultiCalibrationView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mButtonWidth(50),
mTminDisplay(-INFINITY),
mTmaxDisplay(INFINITY),
mThreshold(95),
mGraphHeight(105),
mCurveColor(Painting::mainColorDark)
{
    setMouseTracking(true);
    mDrawing = new MultiCalibrationDrawing(this);
    setMouseTracking(true);

    mImageSaveBut = new Button(tr("Save"), this);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));
    mImageSaveBut->setIconOnly(true);

    mImageClipBut = new Button(tr("Copy"), this);
    mImageClipBut->setIcon(QIcon(":picture_copy.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));
    mImageClipBut->setIconOnly(true);

    mResultsClipBut = new Button(tr("Stat"), this);
    mResultsClipBut->setIcon(QIcon(":stats_w.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));
    mResultsClipBut->setIconOnly(true);


    mGraphHeightLab = new Label(tr("Size"), this);
    mGraphHeightLab->setAlignment(Qt::AlignHCenter);
    mGraphHeightLab->setLight();

    mGraphHeightEdit = new LineEdit(this);
    mGraphHeightEdit->setText(locale().toString(mGraphHeight));

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
  //  frameSeparator->setStyleSheet("QFrame { border: 5px solid rgb(49, 112, 176); }");//same color as Painting::mainColorLight = QColor(49, 112, 176);


    mHPDLab = new Label(tr("HPD (%)"), this);
    mHPDLab->setAlignment(Qt::AlignHCenter);
    mHPDLab->setLight();

    mHPDEdit = new LineEdit(this);
    mHPDEdit->setText("95");

    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    percentValidator->setDecimals(3);
    mHPDEdit->setValidator(percentValidator);

    mStartLab = new Label(tr("Start"), this);
    mStartLab->setAlignment(Qt::AlignHCenter);
    mStartLab->setLight();

    mStartEdit = new LineEdit(this);
    mStartEdit->setText("-1000");

    mEndLab = new Label(tr("End"), this);
    mEndLab->setAlignment(Qt::AlignHCenter);
    mEndLab->setLight();

    mEndEdit = new LineEdit(this);
    mEndEdit->setText("1000");

    mHPDLab->raise();
    mHPDEdit->raise();

    //-------- DrawingView



    // Connection
    // setText doesn't emit signal textEdited, when the text is changed programmatically

    connect(mStartEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &MultiCalibrationView::updateScroll);
    connect(mEndEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &MultiCalibrationView::updateScroll);
    connect(mHPDEdit, &QLineEdit::textEdited, this, &MultiCalibrationView::updateHPDGraphs);
    connect(mImageSaveBut, &Button::clicked, this, &MultiCalibrationView::exportFullImage);
    connect(mImageClipBut, &Button::clicked, this, &MultiCalibrationView::copyImage);
    connect(mResultsClipBut, &Button::clicked, this, &MultiCalibrationView::copyText);
    connect(mGraphHeightEdit, &QLineEdit::textEdited, this, &MultiCalibrationView::updateGraphsSize);
    connect(mColorClipBut, &Button::clicked, this, &MultiCalibrationView::changeCurveColor);

    setVisible(false);
}

MultiCalibrationView::~MultiCalibrationView()
{

}

void MultiCalibrationView::setFont(const QFont &font)
{
    // we must force setFont on QLineEdi !!
    mHPDEdit->setFont(font);
    mStartEdit->setFont(font);
    mEndEdit->setFont(font);
    repaint();
}

void MultiCalibrationView::resizeEvent(QResizeEvent* )
{

    update();

}

void MultiCalibrationView::paintEvent(QPaintEvent* e)
{
    (void) e;


    //const int graphLeft (mButtonWidth);//mImageSaveBut->x() + mImageSaveBut->width());
    const int graphWidth (width() - mButtonWidth);

    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing); // not necessary
    // drawing a background under button
    p.fillRect(QRect(graphWidth, 0, mButtonWidth, height()), QColor(50, 50, 50));

    // drawing a background under curve
   // p.fillRect(QRect(0, 0, graphWidth, height()), Qt::green);

    updateLayout();

}

void MultiCalibrationView::setVisible(bool visible)
{
    mImageSaveBut->setVisible(visible);
    mImageClipBut->setVisible(visible);
    mResultsClipBut->setVisible(visible);
    frameSeparator->setVisible(visible);

    mStartLab->setVisible(visible);
    mStartEdit->setVisible(visible);
    mEndLab->setVisible(visible);
    mEndEdit->setVisible(visible);
    // Fill under distrib. of calibrated date only if typo :
   // const bool isTypo (mDate.mPlugin && (mDate.mPlugin->getName() == "Typo"));
    mHPDLab->setVisible(visible);
    mHPDEdit->setVisible(visible);

    QWidget::setVisible(visible);
}

void MultiCalibrationView::updateLayout()
{

    qreal x0 = width()-mButtonWidth;

    QFontMetrics fm (font());
    const int textHeight (fm.height() + 3);
    const int verticalSpacer (fm.height());

    //Position of Widget
    int y (0);

    mImageSaveBut->setGeometry(x0, y, mButtonWidth, mButtonWidth);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(x0, y, mButtonWidth, mButtonWidth);
    y += mImageClipBut->height();
    mResultsClipBut->setGeometry(x0, y, mButtonWidth, mButtonWidth);
    y += mResultsClipBut->height();

    mGraphHeightLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mGraphHeightLab->height();
    mGraphHeightEdit->setGeometry(x0+3, y, mButtonWidth-6, textHeight);
    y += mGraphHeightEdit->height() + verticalSpacer;

    mColorClipBut->setGeometry(x0, y, mButtonWidth, mButtonWidth);
    y += mColorClipBut->height();

    const int separatorHeight (height() - 8*textHeight - 8* verticalSpacer - 6*mButtonWidth);
    frameSeparator->setGeometry(x0, y, mButtonWidth, separatorHeight);
    y += frameSeparator->height() + verticalSpacer;

    mStartLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mStartLab->height();
    mStartEdit->setGeometry(x0+3, y, mButtonWidth-6, textHeight);
    y += mStartEdit->height() + verticalSpacer;
    mEndLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mEndLab->height();
    mEndEdit->setGeometry(x0+3, y, mButtonWidth-6, textHeight);
    y += mEndEdit->height() + 3*verticalSpacer;
    mHPDLab->setGeometry(x0, y, mButtonWidth, textHeight);
    y += mHPDLab->height();
    mHPDEdit->setGeometry(x0+3, y, mButtonWidth-6, textHeight);

    const int graphWidth = width() - mButtonWidth;

    mDrawing->setGeometry(0, 0, graphWidth, height());
    mDrawing->setGraphHeight(mGraphHeight);

    mDrawing->update();

}

void MultiCalibrationView::updateGraphList()
{
    QJsonObject state = mProject->state();
    mSettings = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());

    mTminDisplay = mSettings.getTminFormated() ;
    mTmaxDisplay = mSettings.getTmaxFormated();
   // mThreshold = 95.;
    // setText doesn't emit signal textEdited, when the text is changed programmatically
    mStartEdit->setText(locale().toString(mTminDisplay));
    mEndEdit->setText(locale().toString(mTmaxDisplay));
    mHPDEdit->setText(locale().toString(mThreshold));

    QColor penColor = mCurveColor;// Painting::mainColorDark;
    QColor brushColor = mCurveColor;//Painting::mainColorLight;
    brushColor.setAlpha(100);

    QList<GraphView*> graphList;
    QList<QColor> colorList;
    const QJsonArray events = state.value(STATE_EVENTS).toArray();


    const QFontMetrics fm (font());
    const int marginRight = (int) floor(fm.width(mStartEdit->text())/2) + 5;
    const int marginLeft = (int) floor(fm.width(mEndEdit->text())/2) + 5;

    QList<QJsonObject> selectedEvents;

    for (auto &&ev : events) {
       QJsonObject jsonEv = ev.toObject();
        if (jsonEv.value(STATE_IS_SELECTED).toBool())
            selectedEvents.append(jsonEv);
    }


    // Sort Event by Position
    std::sort(selectedEvents.begin(), selectedEvents.end(), [] (QJsonObject ev1, QJsonObject ev2) {return (ev1.value(STATE_ITEM_Y).toDouble() < ev2.value(STATE_ITEM_Y).toDouble());});

    QString preEventName ="";

    for (auto &&ev : selectedEvents) {
        const QJsonArray dates = ev.value(STATE_EVENT_DATES).toArray();
      //  mResultText += "<br>" + ev.value(STATE_NAME).toString() + "<br>";

        for (auto &&date : dates) {
            const QJsonObject jdate = date.toObject();

            Date d = Date::fromJson(jdate);
            d.autoSetTiSampler(true);

            QMap<double, double> calibMap = d.getFormatedCalibMap();



            GraphCurve calibCurve;
            calibCurve.mName = "Calibration";
            calibCurve.mPen.setColor(penColor);
            calibCurve.mIsHisto = false;
            calibCurve.mData = calibMap;


            const bool isTypo (d.mPlugin->getName() == "Typo");
            calibCurve.mIsRectFromZero = isTypo;
            calibCurve.mBrush = QBrush(Qt::NoBrush);// isTypo ? QBrush(brushColor) : QBrush(Qt::NoBrush);

            type_data yMax = map_max_value(calibCurve.mData);
            yMax = (yMax > 0.) ? yMax : 1.;

            GraphView* calibGraph = new GraphView(this);
            calibGraph->setRangeY(0., 1. * yMax);
            calibGraph->addCurve(calibCurve);

            // Insert the Event's Name only if different to the previous Event's name
            QString eventName (ev.value(STATE_NAME).toString());
            if (eventName != preEventName)
                calibGraph->addInfo(tr("Event") + " : "+ eventName);


            else
                calibGraph->addInfo("");
            preEventName = eventName;

            calibGraph->addInfo(d.mName);
            calibGraph->showInfos(true);

            QMap<type_data, type_data> hpd = create_HPD(calibCurve.mData, mThreshold);

            GraphCurve hpdCurve;
            hpdCurve.mName = "Calibration HPD";
            hpdCurve.mPen = penColor;
            hpdCurve.mBrush = brushColor;
            hpdCurve.mIsHisto = false;
            hpdCurve.mIsRectFromZero = true;
            hpdCurve.mData = hpd;
            calibGraph->addCurve(hpdCurve);

            yMax = map_max_value(hpdCurve.mData);

            calibGraph->setRangeY(0., 1. * yMax);

            calibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            calibGraph->setFormatFunctX(stringWithAppSettings);
            calibGraph->setFormatFunctY(nullptr);

            calibGraph->setMarginRight(marginRight);
            calibGraph->setMarginLeft(marginLeft);

            calibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
            calibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
            calibGraph->setRendering(GraphView::eHD);
            graphList.append(calibGraph);
            QColor color = QColor(ev.value(STATE_COLOR_RED).toInt(),
                                  ev.value(STATE_COLOR_GREEN).toInt(),
                                  ev.value(STATE_COLOR_BLUE).toInt());
            colorList.append(color);

        }
   }
   mDrawing->showMarker();
   mDrawing->setGraphHeight(mGraphHeight);
   mDrawing->setEventsColorList(colorList);
   mDrawing->setGraphList(graphList);

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
    double val = locale().toDouble(thres,&ok);
    if (ok)
        mThreshold = val;
    else
        return;

    QList<GraphView*> *graphList = mDrawing->getGraphList();

    for (GraphView* gr : *graphList) {
        GraphCurve* calibCurve = gr->getCurve("Calibration");

        QMap<type_data, type_data> hpd = create_HPD(calibCurve->mData, mThreshold);
        GraphCurve* hpdCurve = gr->getCurve("Calibration HPD");
        hpdCurve->mData = hpd;
        gr->forceRefresh();
    }

}

void MultiCalibrationView::updateGraphsSize(const QString &size)
{
    bool ok;
    double val = locale().toDouble(size,&ok);
    if (ok)
        mGraphHeight = val;
    else
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
        return;

    val = locale().toDouble(mEndEdit->text(),&ok);
    if (ok)
        mTmaxDisplay = val;
    else
        return;

    QFont adaptedFont (font());
    QFontMetricsF fm (font());
    qreal textSize = fm.width(mStartEdit->text());
    if (textSize > (mStartEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mStartEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mStartEdit->setFont(adaptedFont);
    }
    else
        mStartEdit->setFont(font());

    adaptedFont = font();
    fm = QFontMetrics(font());
    textSize = fm.width(mEndEdit->text());
    if (textSize > (mEndEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mEndEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mEndEdit->setFont(adaptedFont);
    }
    else
        mEndEdit->setFont(font());
//qDebug()<<"MultiCalibrationView::updateScroll()"<<mTminDisplay<<mTmaxDisplay;
    // usefull when we set mStartEdit and mEndEdit at the begin of the display,
    // after a call to setDate
    if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay))
        return;
    else if (mTminDisplay<mTmaxDisplay)
            updateGraphsZoom();
    else
        return;

}

void MultiCalibrationView::updateGraphsZoom()
{
    QList<GraphView*> *graphList = mDrawing->getGraphList();

    for (GraphView* gr : *graphList) {
        gr->setRangeX(mTminDisplay, mTmaxDisplay);
        gr->setCurrentX(mTminDisplay, mTmaxDisplay);
        gr->forceRefresh();
    }

    //mDrawing->forceRefresh();//updateLayout();
    //update();
}

void MultiCalibrationView::exportImage()
{
    mDrawing->hideMarker();
    mDrawing->update();
    QWidget* widgetExport = mDrawing->getGraphWidget();
    QFileInfo fileInfo = saveWidgetAsImage(widgetExport, widgetExport->rect(), tr("Save calibration image as..."),
                                           MainWindow::getInstance()->getCurrentPath(), MainWindow::getInstance()->getAppSettings());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());

    mDrawing->showMarker();

}

void MultiCalibrationView::exportFullImage()
{

    bool printAxis = (mGraphHeight <= 100.);

    QWidget* widgetExport = mDrawing->getGraphWidget();


    // --------------------------------------------------------------------

    AxisWidget* axisWidget = nullptr;
    QLabel* axisLegend = nullptr;
    int axeHeight (20);
    int legendHeight (20);

    if (printAxis) {
        widgetExport->setFixedHeight(widgetExport->height() + axeHeight + legendHeight );

        FormatFunc f = stringWithAppSettings;

        QFontMetricsF fmAxe (font());

        // 3 const : The same Name and same Value as in MultiCalibrationDrawing::updateLayout()
        const int marginRight = (int) floor(fmAxe.width(mStartEdit->text())/2) + 5;
        const int marginLeft = (int) floor(fmAxe.width(mEndEdit->text())/2) + 5;
        const int panelWidth (20);

        axisWidget = new AxisWidget(f, widgetExport);
        axisWidget->mMarginLeft = marginLeft + panelWidth;
        axisWidget->mMarginRight = marginRight;

        axisWidget->setGeometry(0, widgetExport->height() - axeHeight, widgetExport->width(), axeHeight);
        axisWidget->updateValues(widgetExport->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight, 50, mTminDisplay, mTmaxDisplay);

        axisWidget->mShowText = true;
        axisWidget->setAutoFillBackground(true);
        axisWidget->mShowSubs = true;
        axisWidget->mShowSubSubs = true;
        axisWidget->mShowArrow = true;
        axisWidget->mShowText = true;

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
                                           MainWindow::getInstance()->getCurrentPath(), MainWindow::getInstance()->getAppSettings());

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
        widgetExport->setFixedHeight(widgetExport->height() - axeHeight - legendHeight);
    }


    // Revert to default display :

    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());

}


void MultiCalibrationView::copyImage()
{
    mDrawing->hideMarker();
    mDrawing->repaint();
    QApplication::clipboard()->setPixmap(mDrawing->grab());
    mDrawing->showMarker();

}

void MultiCalibrationView::changeCurveColor()
{
    QColor color = QColorDialog::getColor(mCurveColor, qApp->activeWindow(), tr("Select Color"));
    if (color.isValid()) {
        mCurveColor = color;
        QList<GraphView*> *graphList = mDrawing->getGraphList();

        for (GraphView* gr : *graphList) {
            GraphCurve* calibCurve = gr->getCurve("Calibration");
            calibCurve->mPen.setColor(mCurveColor);

            GraphCurve* hpdCurve = gr->getCurve("Calibration HPD");
            hpdCurve->mPen.setColor(mCurveColor);
            const QColor brushColor (mCurveColor.red(),mCurveColor.green(), mCurveColor.blue(), 100);
            hpdCurve->mBrush = QBrush(brushColor);

            gr->forceRefresh();
        }
    }
}

void MultiCalibrationView::copyText()
{

    QList<GraphView*> *graphList = mDrawing->getGraphList();
    mResultText = "";

    for (GraphView* gr : *graphList) {
        GraphCurve* calibCurve = gr->getCurve("Calibration");

       QMap<double, double> calibMap = calibCurve->mData;
       //QString resultsStr = d.mName + " (" + d.mPlugin->getName() + ")" +"<br>" + d.getDesc() + "<br>";
       QString resultsStr = "<br>"+gr->getInfo();
       DensityAnalysis results;
       results.analysis = analyseFunction(calibMap);
    //   results.quartiles = quartilesForRepartition(d.getFormatedRepartition(), d.getFormatedTminCalib(), mSettings.mStep);
       resultsStr += densityAnalysisToString(results);


        GraphCurve* hpdCurve = gr->getCurve("Calibration HPD");

        QMap<type_data, type_data> hpd = hpdCurve->mData;
        double realThresh = map_area(hpd) / map_area(calibCurve->mData);

        resultsStr += + "<br> HPD (" + locale().toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormatStr(), stringWithAppSettings);

        mResultText += resultsStr + "<br>";
    }




    QApplication::clipboard()->setText(mResultText.replace("<br>", "\r"));
}

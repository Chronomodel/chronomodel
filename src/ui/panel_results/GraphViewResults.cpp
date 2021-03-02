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

#include "GraphViewResults.h"
#include "Button.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "MHVariable.h"
#include <QtWidgets>
#include <QtSvg>
#include <QMessageBox>
#include <QPointer>


//  Constructor / Destructor

int GraphViewResults::mHeightForVisibleAxis = int (4 * AppSettings::heigthUnit()); //look ResultsView::applyAppSettings()

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mCurrentTypeGraph(ePostDistrib),
mCurrentVariable(eTheta),
mShowAllChains(true),
mShowCredibility(false),
mShowCalib(false),
mShowWiggle(false),
mShowNumResults(false),
mIsSelected(false),
mShowSelectedRect(true),
mMainColor(Painting::borderDark),
mMargin(5),
mLineH(20),
mTopShift(0),
mGraphFont(font())
{
    setGeometry(QRect(0, 0, parentWidget()->width(), 20 * AppSettings::heigthUnit()));
    setMouseTracking(true);


    mGraph = new GraphView(this);
    mGraph->setMouseTracking(true);

    mGraph->setCanControlOpacity(true);
    mGraph->setCurvesOpacity(30);

    mGraph->showXAxisValues(true);
    mGraph->showXAxisArrow(true);
    mGraph->showXAxisTicks(true);
    mGraph->showXAxisSubTicks(true);
    mGraph->showXAxisValues(true);

    mGraph->showYAxisArrow(true);
    mGraph->showYAxisTicks(true);
    mGraph->showYAxisSubTicks(true);
    mGraph->showYAxisValues(true);

    mGraph->setXAxisMode(GraphView::eAllTicks);
    mGraph->setYAxisMode(GraphView::eMinMax);

    mGraph->setMargins(50, 10, 5, mGraphFont.pointSize() * 2.2); // make setMarginRight seMarginLeft ...
    mGraph->setRangeY(0, 1);

    mTextArea = new QTextEdit(this);
    mTextArea->setFrameStyle(QFrame::HLine);
    QPalette palette = mTextArea->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    mTextArea->setPalette(palette);

    mTextArea->setFontFamily(mGraphFont.family());
    mTextArea->setFontPointSize(mGraphFont.pointSizeF());
    mTextArea->setText(tr("Nothing to display"));
    mTextArea->setVisible(false);
    mTextArea->setReadOnly(true);

     /* OverLaySelect must be created after mGraph, because it must be refresh after/over the graph
      */
     mOverLaySelect = new Overlay (this);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));

}

GraphViewResults::~GraphViewResults()
{

}

void GraphViewResults::generateCurves(TypeGraph typeGraph, Variable variable)
{
    mCurrentTypeGraph = typeGraph;
    mCurrentVariable = variable;
}

void GraphViewResults::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    mShowAllChains = showAllChains;
    mShowChainList = showChainList;
    mShowCredibility = showCredibility;
    mShowCalib = showCalib;
    mShowWiggle = showWiggle;
   // update();
}


void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc, const QList<ChainSpecs>& chains)
{
    mMCMCSettings = mcmc;
    mChains = chains;
}

/**
 @brief set date range on all the study
 */
void GraphViewResults::setRange(type_data min, type_data max)
{
    mGraph->setRangeX(min, max);
    // we must update the size of the Graph if the font has changed between two toogle
    updateLayout();
}
void GraphViewResults::setCurrentX(type_data min, type_data max)
{
    mGraph->setCurrentX(min, max);
}

void GraphViewResults::zoom(type_data min, type_data max)
{
    mGraph->zoomX(min, max);
}

void GraphViewResults::setMainColor(const QColor& color)
{
    mMainColor = color;
    update();
}
//Export Image & Data

void GraphViewResults::setTitle(const QString& title)
{
    mTitle = title;
    update();
}

void GraphViewResults::saveAsImage()
{
    //GraphView::Rendering memoRendering= mGraph->getRendering();
    /* We can not have a svg graph in eSD Rendering Mode */
    //mGraph->setRendering(GraphView::eHD);
/*    QRect r(0, 0, mGraph->width(), mGraph->height());
    QFileInfo fileInfo = saveWidgetAsImage(mGraph, r, tr("Save graph image as..."),
                                           MainWindow::getInstance()->getCurrentPath());




    if(fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
 */
   // mGraph->setRendering(memoRendering);
    QString filter = QObject::tr("Image (*.png);;Photo (*.jpg);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph image as..."),
                                                    MainWindow::getInstance()->getCurrentPath(),
                                                    filter);
    QFileInfo fileInfo;
    fileInfo = QFileInfo(fileName);
    QString fileExtension = fileInfo.suffix();
    if (!fileName.isEmpty()) {
        //QFileInfo fileInfo = QFileInfo(fileName);
        bool asSvg = fileName.endsWith(".svg");
        if (asSvg) {
            if (mGraph)
                mGraph->saveAsSVG(fileName, mTitle, "GraphViewResults",true);

        } else {

            //---
            //GraphView::Rendering memoRendering= mGraph->getRendering();
            //setRendering(GraphView::eHD);
            const short pr = AppSettings::mPixelRatio;
            const int versionHeight = 20;

            QImage image(mGraph->width() * pr, (mGraph->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32

            if (image.isNull() )
                qDebug()<< " image width = 0";


            image.setDevicePixelRatio(pr);
            image.fill(Qt::transparent);

            QPainter p;
            p.begin(&image);
            p.setRenderHint(QPainter::Antialiasing);

            mGraph->render(&p, QPoint(0, 0), QRegion(0, 0, mGraph->width(), mGraph->height()));

            p.setPen(Qt::black);
            p.drawText(0, mGraph->height(), mGraph->width(), versionHeight,
                       Qt::AlignCenter,
                       qApp->applicationName() + " " + qApp->applicationVersion());

            p.end();

            if (fileExtension=="png") {
                 image.save(fileName, "png");
            }
            else if (fileExtension == "jpg") {
                int imageQuality = AppSettings::mImageQuality;
                image.save(fileName, "jpg",imageQuality);
            }
            else if (fileExtension == "bmp") {
                image.save(fileName, "bmp");
            }

            //mGraph->setRendering(memoRendering);
        }

    }
}

void GraphViewResults::imageToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap(mGraph->grab());
}

void GraphViewResults::resultsToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(mResultsText);
}
/**
 * @brief Export data from the visible curves, there is two kinds of data in a graphView: mData, QMap type and mDataVector, QVector type
 * @brief So there is two export functon exportCurrentVectorCurves and exportCurrentDensityCurves.
 * @brief In the Posterior Density tab, the Credibility bar is not save
 */
void GraphViewResults::saveGraphData() const
{
    const QString csvSep = AppSettings::mCSVCellSeparator;

    QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
    csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);

    int offset (0);

    if (mCurrentTypeGraph == eTrace || mCurrentTypeGraph == eAccept) {
        QMessageBox messageBox;
        messageBox.setWindowTitle(tr("Save all trace"));
        messageBox.setText(tr("Do you want the entire trace from the beginning of the process or only the acquisition part"));
        QAbstractButton *allTraceButton = messageBox.addButton(tr("All trace"), QMessageBox::YesRole);
        QAbstractButton *acquireTraceButton = messageBox.addButton(tr("Only acquired part"), QMessageBox::NoRole);

        messageBox.exec();
        if (messageBox.clickedButton() == allTraceButton)
            mGraph->exportCurrentVectorCurves(MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, false, 0);

        else if (messageBox.clickedButton() == acquireTraceButton) {
                int chainIdx = -1;
                for (int i (0); i<mShowChainList.size(); ++i)
                    if (mShowChainList.at(i)) chainIdx = i;

                if (chainIdx != -1) // We add 1 for the init
                    offset = 1 + mChains.at(chainIdx).mNumBurnIter + mChains.at(chainIdx).mBatchIndex * mChains.at(chainIdx).mNumBatchIter;

                mGraph->exportCurrentVectorCurves(MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, false, offset);
        }
        else return;
    }

    else if (mCurrentTypeGraph == eCorrel)
        mGraph->exportCurrentVectorCurves (MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep, false, 0);

    // All visible curves are saved in the same file, the credibility bar is not save
    else if (mCurrentTypeGraph == ePostDistrib)
        mGraph->exportCurrentDensityCurves (MainWindow::getInstance()->getCurrentPath(), csvLocal, csvSep,  mSettings.mStep);

}

void GraphViewResults::setNumericalResults (const QString& resultsHTML, const QString& resultsText)
{
    mResultsText = resultsText;
    mTextArea->setHtml(resultsHTML);
}

void GraphViewResults::showNumericalResults(const bool show)
{
    mShowNumResults = show;
    mTextArea->setVisible(show);
    updateLayout();
}

void GraphViewResults::setShowNumericalResults(const bool show)
{
    mShowNumResults = show;
    mTextArea->setVisible(show);
}

void GraphViewResults::setMarginLeft (qreal &m)
{
    mGraph->setMarginLeft(m);
}
void GraphViewResults::setMarginRight(qreal &m)
{
    mGraph->setMarginRight(m);
}

void GraphViewResults::setGraphFont(const QFont& font)
{
     // Recalcule mTopShift based on the new font, and position the graph according :
    mGraphFont = font;
    mTextArea->setFontFamily(font.family());
    mTextArea->setFontPointSize(font.pointSizeF());
    mGraph->setFont(font);
    updateLayout();
}

void GraphViewResults::setGraphsThickness(int value)
{
    mGraph->setGraphsThickness(value);
}

void GraphViewResults::setGraphsOpacity(int value)
{
    mGraph->setCurvesOpacity(value);
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    (void) e;
    updateLayout();
}

void GraphViewResults::updateLayout()
{
     // Define the rigth margin, according to the max on the scale
    QFont fontTitle (mGraphFont);
    fontTitle.setPointSizeF(mGraphFont.pointSizeF() * 1.1);
    QFontMetricsF fmTitle (fontTitle);
    mTopShift = int (2 * fmTitle.height()) ;

    QRect graphRect(0, int (mTopShift), width(), int (height() - mTopShift));
    QFontMetricsF fm (mGraphFont);

    if (mShowNumResults) {
        mGraph->setGeometry(graphRect.adjusted(0, 0, int (-width()/3. -1), 0 ));
        mTextArea->setGeometry(graphRect.adjusted(int (width()*2./3. + 2.), int (-mTopShift + 2 ), -2, -2));

    } else
        mGraph->setGeometry(graphRect);

    const bool axisVisible = (height() >= mHeightForVisibleAxis);

    if ((mGraph->hasCurve())) {
        mGraph->showXAxisValues(axisVisible);
        mGraph->setMarginBottom(axisVisible ? fm.ascent()* 2.0 : fm.ascent());
    }

    update();
}

void GraphViewResults::mousePressEvent(QMouseEvent *event)
{
    (void) event;
    setSelected(!isSelected());
    update();
    emit selected();
}

void GraphViewResults::paintEvent(QPaintEvent* )
{
    // write mTitle above the graph
    QFont fontTitle(mGraphFont);
    fontTitle.setPointSizeF(mGraphFont.pointSizeF()*1.1);
    QFontMetrics fmTitle(fontTitle);

    QPainter p;
    p.begin(this);
    p.fillRect(rect(), mGraph->getBackgroundColor());
    p.setFont(fontTitle);

    p.setPen(Qt::black);

    p.drawText(QRectF( 2 * AppSettings::widthUnit(), 0, fmTitle.boundingRect(mTitle).width(), mTopShift), Qt::AlignVCenter | Qt::AlignLeft, mTitle);

    p.setFont(QFont(mGraphFont.family(), mGraphFont.pointSize(), -1 , true));

    /*
     *  Write info at the right of the title line
     */

   QString graphInfo = mGraph->getInfo();
   if (!graphInfo.isEmpty()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        if (mShowNumResults)
             p.drawText(QRectF(width()*2/3. - fmTitle.boundingRect(graphInfo).width() - 3 * AppSettings::widthUnit(),  mTopShift - fmTitle.capHeight()-fmTitle.descent(), fmTitle.boundingRect(graphInfo).width(), mTopShift), Qt::AlignTop | Qt::AlignLeft, graphInfo);
         else
            p.drawText(QRectF(width() - fmTitle.boundingRect(graphInfo).width() - 3 * AppSettings::widthUnit(), mTopShift - fmTitle.capHeight()-fmTitle.descent() , fmTitle.boundingRect(graphInfo).width(), mTopShift), Qt::AlignTop | Qt::AlignLeft, graphInfo);
#else
       if (mShowNumResults)
            p.drawText(QRectF(width()*2/3. - fmTitle.width(graphInfo) - 3 * AppSettings::widthUnit(),  mTopShift - fmTitle.ascent()-fmTitle.descent(), fmTitle.width(graphInfo), mTopShift), Qt::AlignTop | Qt::AlignLeft, graphInfo);
        else
           p.drawText(QRectF(width() - fmTitle.width(graphInfo)  - 3 * AppSettings::widthUnit(), mTopShift - fmTitle.ascent()-fmTitle.descent() , fmTitle.width(graphInfo), mTopShift), Qt::AlignTop | Qt::AlignLeft, graphInfo);
#endif
   }

    p.setPen(QColor(105, 105, 105));
    if (mShowNumResults)
        p.drawRect(mTextArea->geometry().adjusted(-1, -1, 1, 1));

    p.end();

    if (mIsSelected && mShowSelectedRect) {
        mOverLaySelect->setGeometry(rect());
        mOverLaySelect->show();

    } else
        mOverLaySelect->hide();

}

 void GraphViewResults::setItemColor(const QColor& itemColor)
{
    mItemColor = itemColor;
}

void GraphViewResults::setItemTitle(const QString& itemTitle)
{
    mItemTitle = itemTitle;
}

/** Generate Typical curves for Chronomodel
 * */
GraphCurve GraphViewResults::generateDensityCurve(const QMap<double, double>& data,
                                                  const QString& name,
                                                  const QColor& lineColor,
                                                  const Qt::PenStyle penStyle,
                                                  const QBrush& brush) const{
    GraphCurve curve;
    curve.mName = name;
    if (!data.isEmpty()) {
        curve.mData = data;
        curve.mPen = QPen(lineColor, 1, penStyle);
        curve.mBrush = brush;
        curve.mIsHisto = false;
        curve.mIsRectFromZero = true; // for Unif-typo. calibs., invisible for others!
   }
    return curve;
}

GraphCurve GraphViewResults::generateHPDCurve(QMap<double, double> &data,
                                              const QString& name,
                                              const QColor& color) const{
    GraphCurve curve;
    curve.mName = name;
    curve.mData = data;
    curve.mPen = Qt::NoPen;// color;
    curve.mBrush = QBrush(color);
    curve.mIsHisto = false;
    curve.mIsRectFromZero = true;

    return curve;
}

GraphCurve GraphViewResults::generateSectionCurve(const QPair<double, double> &section,
                                                      const QString& name,
                                                      const QColor& color) const{
    GraphCurve curve;
    curve.mName = name;
    curve.mSections.append(section);
    curve.mPen.setColor(color);
    curve.mPen.setWidth(3);
    curve.mPen.setStyle(Qt::SolidLine);
    curve.mIsTopLineSections = true;

    return curve;
}

GraphCurve GraphViewResults::generateHorizontalLine(const double yValue,
                                                    const QString& name,
                                                    const QColor& color,
                                                    const Qt::PenStyle penStyle) const
{
    GraphCurve curve;
    curve.mName = name;
    curve.mIsHorizontalLine = true;
    curve.mHorizontalValue = yValue;
    curve.mPen.setStyle(penStyle);
    curve.mPen.setColor(color);
    return curve;
}

void GraphViewResults::generateTraceCurves(const QList<ChainSpecs> &chains,
                                           MetropolisVariable* variable,
                                           const QString& name)
{
    QString prefix = name.isEmpty() ? name : name + " ";

    for (int i = 0; i < chains.size(); ++i) {
        GraphCurve curve;
        curve.mUseVectorData = true;
        curve.mName = prefix + "Trace " + QString::number(i);
        curve.mDataVector = variable->fullTraceForChain(chains, i);
        curve.mPen.setColor(Painting::chainColors.at(i));
        curve.mIsHisto = false;
        mGraph->addCurve(curve);

        const double min ( vector_min_value(curve.mDataVector) );
        const double max ( vector_max_value(curve.mDataVector) );
        mGraph->setRangeY(floor(min), ceil(max));

        const Quartiles& quartiles = variable->mChainsResults.at(i).quartiles;
        
        GraphCurve curveQ1 = generateHorizontalLine(quartiles.Q1, prefix + "Q1 " + QString::number(i), Qt::green);
        mGraph->addCurve(curveQ1);
        
        GraphCurve curveQ2 = generateHorizontalLine(quartiles.Q2, prefix + "Q2 " + QString::number(i), Qt::red);
        mGraph->addCurve(curveQ2);

        GraphCurve curveQ3 = generateHorizontalLine(quartiles.Q3, prefix + "Q3 " + QString::number(i), Qt::green);
        mGraph->addCurve(curveQ3);
    }
}


void GraphViewResults::generateAcceptCurves(const QList<ChainSpecs> &chains,
                                            MHVariable* variable){
    for (int i = 0; i < chains.size(); ++i) {
        GraphCurve curve;
        curve.mName = "Accept " + QString::number(i);
        curve.mDataVector = variable->acceptationForChain(chains, i);
        curve.mPen.setColor(Painting::chainColors.at(i));
        curve.mUseVectorData = true;
        curve.mIsHisto = false;
        mGraph->addCurve(curve);
    }
}

void GraphViewResults::generateCorrelCurves(const QList<ChainSpecs> &chains,
                                            MHVariable* variable){
    for (int i = 0; i < chains.size(); ++i) {
        GraphCurve curve;
        curve.mName = "Correl " + QString::number(i);
        curve.mDataVector = variable->correlationForChain(i);
        // if there is no data, no curve to add.
        // It can append if there is not enought iteration, for example since a test
        if (curve.mDataVector.isEmpty())
            continue;

        curve.mUseVectorData = true;
        curve.mPen.setColor(Painting::chainColors.at(i));
        curve.mIsHisto = false;
        mGraph->addCurve(curve);

        //to do, we only need the totalIter number?
        const double n = variable->runRawTraceForChain(mChains, i).size();
        const double limit = 1.96 / sqrt(n);

        GraphCurve curveLimitLower = generateHorizontalLine(-limit,
                                                            "Correl Limit Lower " + QString::number(i),
                                                            Qt::red,
                                                            Qt::DotLine);
        GraphCurve curveLimitUpper = generateHorizontalLine(limit,
                                                            "Correl Limit Upper " + QString::number(i),
                                                            Qt::red,
                                                            Qt::DotLine);
        mGraph->addCurve(curveLimitLower);
        mGraph->addCurve(curveLimitUpper);
    }
}

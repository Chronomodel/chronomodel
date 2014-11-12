#include "GraphViewResults.h"
#include "GraphView.h"
#include "Button.h"
#include "ProjectManager.h"
#include "Project.h"
#include "Painting.h"
#include "QtUtilities.h"
#include <QtWidgets>
#include <QtSvg>



#pragma mark Constructor / Destructor

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mCurrentResult(eHisto),
mCurrentVariable(eTheta),
mMainColor(QColor(50, 50, 50)),
mMargin(5),
mLineH(20),
mGraphLeft(130)
{
    setMouseTracking(true);
    
    mGraph = new GraphView(this);
    
    mGraph->showAxis(false);
    mGraph->showScrollBar(false);
    mGraph->showYValues(true);
    mGraph->setRangeY(0, 1);
    
    mTextArea = new QTextEdit(this);
    QPalette palette = mTextArea->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 150));
    palette.setColor(QPalette::Text, Qt::white);
    mTextArea->setPalette(palette);
    QFont font = mTextArea->font();
    font.setPointSizeF(pointSize(11));
    mTextArea->setFont(font);
    mTextArea->setText(tr("Nothing to display"));
    mTextArea->setVisible(false);
    
    mImageSaveBut = new Button(tr("Save Image"), this);
    mImageClipBut = new Button(tr("Copy Image"), this);
    mResultsClipBut = new Button(tr("Copy results"), this);
    mDataSaveBut = new Button(tr("Save Graph Data"), this);
    
    mAnimation = new QPropertyAnimation();
    mAnimation->setPropertyName("geometry");
    mAnimation->setDuration(200);
    mAnimation->setTargetObject(this);
    mAnimation->setEasingCurve(QEasingCurve::Linear);
    
    connect(mImageSaveBut, SIGNAL(clicked()), this, SLOT(saveAsImage()));
    connect(mImageClipBut, SIGNAL(clicked()), this, SLOT(imageToClipboard()));
    connect(mResultsClipBut, SIGNAL(clicked()), this, SLOT(resultsToClipboard()));
    connect(mDataSaveBut, SIGNAL(clicked()), this, SLOT(saveGraphData()));
    
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
}

GraphViewResults::~GraphViewResults()
{
    
}

void GraphViewResults::setResultToShow(Result result, Variable variable)
{
    mCurrentResult = result;
    mCurrentVariable = variable;
    refresh();
}

void GraphViewResults::toggle(const QRect& targetGeometry)
{
    if(geometry() != targetGeometry)
    {
        //qDebug() << "Graph From : " << geometry() << ", To : " << targetGeometry;
        
        mAnimation->setStartValue(geometry());
        mAnimation->setEndValue(targetGeometry);
        mAnimation->start();
    }
}

void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc, const QList<Chain>& chains)
{
    mMCMCSettings = mcmc;
    mChains = chains;
}

void GraphViewResults::updateChains(bool showAll, const QList<bool>& showChainList)
{
    mShowAllChains = showAll;
    mShowChainList = showChainList;
    refresh();
}

void GraphViewResults::updateHPD(bool show, int threshold)
{
    mShowHPD = show;
    mThresholdHPD = threshold;
    refresh();
}

void GraphViewResults::setRange(float min, float max)
{
    mGraph->setRangeX(min, max);
}

void GraphViewResults::zoom(float min, float max)
{
    mGraph->zoomX(min, max);
}

void GraphViewResults::setMainColor(const QColor& color)
{
    mMainColor = color;
    update();
}

void GraphViewResults::saveAsImage()
{
    QString filter = tr("Image (*.png);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph image as..."),
                                                    ProjectManager::getCurrentPath(),
                                                    filter);
    if(!fileName.isEmpty())
    {
        bool asSvg = fileName.endsWith(".svg");
        if(asSvg)
        {
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            svgGen.setSize(mGraph->size());
            svgGen.setViewBox(QRect(0, 0, mGraph->width(), mGraph->height()));
            QPainter p(&svgGen);
            p.setRenderHint(QPainter::Antialiasing);
            mGraph->render(&p);
        }
        else
        {
            QImage image(mGraph->size(), QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            QPainter p(&image);
            p.setRenderHint(QPainter::Antialiasing);
            mGraph->render(&p);
            image.save(fileName, "PNG");
        }
        QFileInfo fileInfo(fileName);
        ProjectManager::setCurrentPath(fileInfo.dir().absolutePath());
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
    clipboard->setText(mResults);
}

void GraphViewResults::showNumericalValues(bool show)
{
    mTextArea->setVisible(show);
    if(show)
        mTextArea->raise();
}

void GraphViewResults::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, mMainColor.lighter());
    grad.setColorAt(1, mMainColor);
    //p.fillRect(rect(), grad);
    
    p.fillRect(rect(), mMainColor);
    QColor foreCol = getContrastedColor(mMainColor);
    p.setPen(foreCol);
    
    p.setPen(Qt::black);
    p.drawRect(0, 0, mGraphLeft, height());
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int bw = (mGraphLeft - 3*mMargin)/2;
    
    mImageSaveBut->setGeometry(mMargin, mMargin + mLineH, bw, mLineH);
    mImageClipBut->setGeometry(2*mMargin + bw, mMargin + mLineH, bw, mLineH);
    
    mResultsClipBut->setGeometry(mMargin, 2*mMargin + 2*mLineH, bw, mLineH);
    mDataSaveBut->setGeometry(2*mMargin + bw, 2*mMargin + 2*mLineH, bw, mLineH);

    mGraph->setGeometry(mGraphLeft, 0, width() - mGraphLeft, height());
    mTextArea->setGeometry(mGraph->geometry().adjusted(50, 10, 0, -10));
}

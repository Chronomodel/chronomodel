#include "GraphViewPhase.h"
#include "GraphView.h"
#include "Phase.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewPhase::GraphViewPhase(QWidget *parent):GraphViewResults(parent),
mPhase(0)
{
    
}

GraphViewPhase::~GraphViewPhase()
{
    
}

void GraphViewPhase::setPhase(Phase* phase)
{
    if(phase)
        mPhase = phase;
    update();
}


void GraphViewPhase::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
    
    QPainter p(this);
    
    if(mPhase)
    {
        QColor color = mPhase->mColor;
        bool isDark = colorIsDark(color);
        
        QRectF r(mMargin, mMargin, mLineH, mLineH);
        p.setBrush(color);
        p.setPen(Qt::black);
        p.drawRect(r);
        p.setPen(isDark ? Qt::white : Qt::black);
        p.drawText(r, Qt::AlignCenter, "P");
        
        r = QRectF(2*mMargin + mLineH,
                   mMargin,
                   mGraphLeft - 3*mMargin - mLineH,
                   mLineH);
        
        p.setPen(Qt::black);
        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, mPhase->mName);
    }
}

void GraphViewPhase::showHisto(bool showAlpha, bool showBeta, bool showPredict, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD)
{
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    mGraph->setRangeY(0, 1);
    mGraph->showInfos(false);
    
    if(mPhase)
    {
        QColor color = mPhase->mColor;
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        if(showAlpha)
        {
            if(showAllChains)
            {
                GraphCurve curve;
                curve.mName = "alpha full";
                curve.mPen.setColor(color);
                curve.mData = normalize_map(mPhase->mAlpha.fullHisto());
                mGraph->addCurve(curve);
                
                if(showHPD)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = "alpha HPD full";
                    curveHPD.mPen.setColor(color);
                    curveHPD.mFillUnder = true;
                    curveHPD.mData = normalize_map(mPhase->mAlpha.generateFullHPD(thresholdHPD));
                    mGraph->addCurve(curveHPD);
                }
            }
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col = mChainColors[i];
                    
                    GraphCurve curveChain;
                    curveChain.mName = QString("alpha chain " + QString::number(i));
                    curveChain.mPen.setColor(col);
                    curveChain.mData = normalize_map(mPhase->mAlpha.histoForChain(i));
                    mGraph->addCurve(curveChain);
                    
                    if(showHPD)
                    {
                        GraphCurve curveChainHPD;
                        curveChainHPD.mName = QString("alpha HPD chain " + QString::number(i));
                        curveChainHPD.mPen.setColor(col);
                        curveChainHPD.mFillUnder = true;
                        curveChainHPD.mData = normalize_map(mPhase->mAlpha.generateHPDForChain(i, thresholdHPD));
                        mGraph->addCurve(curveChainHPD);
                    }
                }
            }
        }
        if(showBeta)
        {
            if(showAllChains)
            {
                GraphCurve curve;
                curve.mName = QString("beta full");
                curve.mPen.setColor(color);
                curve.mData = normalize_map(mPhase->mBeta.fullHisto());
                mGraph->addCurve(curve);
                
                if(showHPD)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = "beta HPD full";
                    curveHPD.mPen.setColor(color);
                    curveHPD.mFillUnder = true;
                    curveHPD.mData = normalize_map(mPhase->mBeta.generateFullHPD(thresholdHPD));
                    mGraph->addCurve(curveHPD);
                }
            }
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col = mChainColors[i];
                    
                    GraphCurve curveChain;
                    curveChain.mName = QString("beta chain " + QString::number(i));
                    curveChain.mPen.setColor(col);
                    curveChain.mData = normalize_map(mPhase->mBeta.histoForChain(i));
                    mGraph->addCurve(curveChain);
                    
                    if(showHPD)
                    {
                        GraphCurve curveChainHPD;
                        curveChainHPD.mName = QString("beta HPD chain " + QString::number(i));
                        curveChainHPD.mPen.setColor(col);
                        curveChainHPD.mFillUnder = true;
                        curveChainHPD.mData = normalize_map(mPhase->mBeta.generateHPDForChain(i, thresholdHPD));
                        mGraph->addCurve(curveChainHPD);
                    }
                }
            }
        }
        if(showPredict)
        {
            if(showAllChains)
            {
                GraphCurve curve;
                curve.mName = QString("predict full");
                curve.mPen.setColor(color);
                curve.mData = normalize_map(mPhase->mThetaPredict.fullHisto());
                mGraph->addCurve(curve);
                
                if(showHPD)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = "predict HPD full";
                    curveHPD.mPen.setColor(color);
                    curveHPD.mFillUnder = true;
                    curveHPD.mData = normalize_map(mPhase->mThetaPredict.generateFullHPD(thresholdHPD));
                    mGraph->addCurve(curveHPD);
                }
            }
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col = mChainColors[i];
                    
                    GraphCurve curveChain;
                    curveChain.mName = QString("predict chain " + QString::number(i));
                    curveChain.mPen.setColor(col);
                    curveChain.mData = normalize_map(mPhase->mThetaPredict.histoForChain(i));
                    mGraph->addCurve(curveChain);

                    if(showHPD)
                    {
                        GraphCurve curveChainHPD;
                        curveChainHPD.mName = QString("predict HPD chain " + QString::number(i));
                        curveChainHPD.mPen.setColor(col);
                        curveChainHPD.mFillUnder = true;
                        curveChainHPD.mData = normalize_map(mPhase->mThetaPredict.generateHPDForChain(i, thresholdHPD));
                        mGraph->addCurve(curveChainHPD);
                    }
                }
            }
        }
    }
}

void GraphViewPhase::showTrace(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList)
{
    mGraph->setRangeX(0, mMCMCSettings.mNumRunIter);
    mGraph->showInfos(false);
    
    if(mPhase)
    {
        QColor color(mPhase->mColor);
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        float min = 999999;
        float max = -999999;
        
        if(showAlpha)
        {
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col = mChainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("alpha trace chain " + QString::number(i));
                    curve.mUseVectorData = true;
                    curve.mDataVector = mPhase->mAlpha.traceForChain(i, showChainList.size());
                    curve.mPen.setColor(col);
                    mGraph->addCurve(curve);
                    
                    min = qMin(vector_min_value(curve.mDataVector), min);
                    max = qMax(vector_max_value(curve.mDataVector), max);
                }
            }
        }
        if(showBeta)
        {
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col = mChainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("beta trace chain " + QString::number(i));
                    curve.mUseVectorData = true;
                    curve.mDataVector = mPhase->mBeta.traceForChain(i, showChainList.size());
                    curve.mPen.setColor(col);
                    mGraph->addCurve(curve);
                    
                    min = qMin(vector_min_value(curve.mDataVector), min);
                    max = qMax(vector_max_value(curve.mDataVector), max);
                }
            }
        }
        if(showPredict)
        {
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col = mChainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("predict trace chain " + QString::number(i));
                    curve.mUseVectorData = true;
                    curve.mDataVector = mPhase->mThetaPredict.traceForChain(i, showChainList.size());
                    curve.mPen.setColor(col);
                    mGraph->addCurve(curve);
                    
                    min = qMin(vector_min_value(curve.mDataVector), min);
                    max = qMax(vector_max_value(curve.mDataVector), max);
                }
            }
        }
        if(min < max)
            mGraph->setRangeY(min, max);
        else
            mGraph->setRangeY(0, 1);
    }
}

void GraphViewPhase::showAccept(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList)
{
    Q_UNUSED(showAlpha);
    Q_UNUSED(showBeta);
    Q_UNUSED(showPredict);
    Q_UNUSED(showChainList);
    
    mGraph->setRangeX(0, mMCMCSettings.mNumRunIter);
    mGraph->setRangeY(0, 100);
    mGraph->clearInfos();
    mGraph->showInfos(true);
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    
    // ?????? No accept for alpha & beta : Metropolis, not MH
    
    /*QStringList infos;
    
    if(mPhase)
    {
        QColor color(mPhase->mRed, mPhase->mGreen, mPhase->mBlue);
        
        mGraph->addDashedLine("target", 44, QColor(180, 10, 20));
        
        if(showAlpha)
        {
            for(int i=0; i<showChainList.size(); ++i)
            {
                if(showChainList[i])
                {
                    QColor col(20 + i*20, 50 + i*20, 80 + i*20);
                    QList<double> accept = mPhase->mAlpha.acceptationForChain(i, showChainList.size());
                    mGraph->addCurve(std::string(QString("alpha accept history chain " + QString::number(i)).toUtf8()),
                                     accept, col, false, false);
                    
                    if(accept.size() > 0)
                    {
                        QString info(tr("Alpha Chain") + " " + QString::number(i+1) + " : " + QString::number(accept[accept.size()-1]));
                        mGraph->addInfo(info);
                    }
                }
            }
        }
        
        
    }
     */
}

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

#include "ModelCurve.h"

#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "StateKeys.h"
#include "StdUtilities.h"
#include "MainWindow.h"
#include "version.h"

#include <QFile>
#include <QFileDialog>
#include <qdatastream.h>
#include <qapplication.h>


#include <math.h>


ModelCurve::ModelCurve():
    Model(),
    mLambdaSpline(),
    mS02Vg(0),
    mSO2_beta(0),
    compute_Y(false),
    compute_XYZ(false),
    compute_X_only(false)
{
    mLambdaSpline.setName(std::string("LambdaSpline of Curve"));
    mLambdaSpline.mSupport = MetropolisVariable::eR;
    mLambdaSpline.mFormat = DateUtils::eNumeric;
    mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

}

ModelCurve::ModelCurve(const QJsonObject& json):
    Model(json),
    mLambdaSpline(),
    mS02Vg(0),
    mSO2_beta(0),
    compute_Y(false),
    compute_XYZ(false),
    compute_X_only(false)
{
    mLambdaSpline.setName(std::string("LambdaSpline of Curve"));
    mLambdaSpline.mSupport = MetropolisVariable::eR;
    mLambdaSpline.mFormat = DateUtils::eNumeric;
    mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

    settings_from_Json(json);

}

ModelCurve::~ModelCurve()
{
    qDebug() << "[ModelCurve::~ModelCurve]";
}

QJsonObject ModelCurve::toJson() const
{
    QJsonObject json = Model::toJson();
    json[STATE_CURVE] = mCurveSettings.toJson();
    
    return json;
}

void ModelCurve::fromJson(const QJsonObject &json)
{
    Model::fromJson(json);
    settings_from_Json(json);
}

void ModelCurve::setProject()
{

    mCurveName.clear();
    mCurveLongName.clear();

    if (!getProject_ptr() || !getProject_ptr()->isCurve()) {
        return;
    }

    const QString xLabel = mCurveSettings.XLabel();
    const QString yLabel = mCurveSettings.YLabel();
    const QString zLabel = mCurveSettings.ZLabel();

    const QString x_short_name = mCurveSettings.X_short_name();
    const QString y_short_name = mCurveSettings.Y_short_name();
    const QString z_short_name = mCurveSettings.Z_short_name();

    switch (mCurveSettings.mProcessType) {

    case CurveSettings::eProcess_2D:
    case CurveSettings::eProcess_Spherical:
    case CurveSettings::eProcess_Unknwon_Dec:
        mCurveName.append({x_short_name, y_short_name});
        mCurveLongName.append({xLabel, yLabel});
        break;

    case CurveSettings::eProcess_3D:
    case CurveSettings::eProcess_Vector:
        mCurveName.append({x_short_name, y_short_name, z_short_name});
        mCurveLongName.append({xLabel, yLabel, zLabel});
        break;

    case CurveSettings::eProcess_None:
    case CurveSettings::eProcess_Univariate:
    case CurveSettings::eProcess_Inclination:
    case CurveSettings::eProcess_Declination:
    case CurveSettings::eProcess_Field:
    case CurveSettings::eProcess_Depth:
    default:
        mCurveName.append(x_short_name);
        mCurveLongName.append(xLabel);
        break;
    }

}

/**
 * @brief ResultsView::updateModel Update Design
 */
void ModelCurve::updateDesignFromJson()
{
    if (!getProject_ptr())
        return;
    setProject();// update mCurveName
    const QJsonObject *state = getProject_ptr()->state_ptr();
    const QJsonArray events = state->value(STATE_EVENTS).toArray();
    const QJsonArray phases = state->value(STATE_PHASES).toArray();

    QJsonArray::const_iterator iterJSONEvent = events.constBegin();
    while (iterJSONEvent != events.constEnd()) {
        const QJsonObject eventJSON = (*iterJSONEvent).toObject();
        const int eventId = eventJSON.value(STATE_ID).toInt();
        const QJsonArray dates = eventJSON.value(STATE_EVENT_DATES).toArray();

        auto iterEvent = mEvents.begin();
        while (iterEvent != mEvents.end()) {
            if ((*iterEvent)->mId == eventId) {
                (*iterEvent)->setName(eventJSON.value(STATE_NAME).toString());
                (*iterEvent)->mItemX = eventJSON.value(STATE_ITEM_X).toDouble();
                (*iterEvent)->mItemY = eventJSON.value(STATE_ITEM_Y).toDouble();
                (*iterEvent)->mIsSelected = eventJSON.value(STATE_IS_SELECTED).toBool();
                (*iterEvent)->mColor = QColor(eventJSON.value(STATE_COLOR_RED).toInt(),
                                              eventJSON.value(STATE_COLOR_GREEN).toInt(),
                                              eventJSON.value(STATE_COLOR_BLUE).toInt());

                for (size_t k = 0; k<(*iterEvent)->mDates.size(); ++k) {
                    Date& d = (*iterEvent)->mDates[k];
                    for (auto &&dateVal : dates) {
                        const QJsonObject& date = dateVal.toObject();
                        const int dateId = date.value(STATE_ID).toInt();

                        if (dateId == d.mId) {
                            d.setName(date.value(STATE_NAME).toString());
                            d.mColor = (*iterEvent)->mColor;
                            break;
                        }
                    }
                }
                break;
            }
            ++iterEvent;
        }
        ++iterJSONEvent;
    }

    QJsonArray::const_iterator iterJSONPhase = phases.constBegin();
    while (iterJSONPhase != phases.constEnd()) {
        const QJsonObject phaseJSON = (*iterJSONPhase).toObject();
        const int phaseId = phaseJSON.value(STATE_ID).toInt();

        for (const auto& p : mPhases) {
            if (p->mId == phaseId) {
                p->setName(phaseJSON.value(STATE_NAME).toString());
                p->mItemX = phaseJSON.value(STATE_ITEM_X).toDouble();
                p->mItemY = phaseJSON.value(STATE_ITEM_Y).toDouble();
                p->mColor = QColor(phaseJSON.value(STATE_COLOR_RED).toInt(),
                                   phaseJSON.value(STATE_COLOR_GREEN).toInt(),
                                   phaseJSON.value(STATE_COLOR_BLUE).toInt());
                p->mIsSelected = phaseJSON.value(STATE_IS_SELECTED).toBool();
                break;
            }
        }
        ++iterJSONPhase;
    }

    std::sort(mEvents.begin(), mEvents.end(), sortEvents);
    std::sort(mPhases.begin(), mPhases.end(), sortPhases);

    for (const auto& p : mPhases ) {
        std::sort(p->mEvents.begin(), p->mEvents.end(), sortEvents);
    }
}

void ModelCurve::settings_from_Json(const QJsonObject &json)
{
    if (json.contains(STATE_CURVE)) {
        const QJsonObject &settings = json.value(STATE_CURVE).toObject();
        mCurveSettings = CurveSettings::fromJson(settings);
    }

    is_curve = mCurveSettings.mProcessType != CurveSettings::eProcess_None;

    for (std::shared_ptr<Event> &event: mEvents) {
        if (event->type() ==  Event::eBound)
            event->mTheta.mSamplerProposal = MHVariable::eFixe;

        else if (event->type() ==  Event::eDefault) {
                if (is_curve && mCurveSettings.mTimeType == CurveSettings::eModeFixed) {
                    event->mTheta.mSamplerProposal = MHVariable::eFixe;
                    for (Date &d : event->mDates) {
                        d.mTi.mSamplerProposal = MHVariable::eFixe;
                        d.mSigmaTi.mSamplerProposal = MHVariable::eFixe;
                    }
                }

        }

        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
            event->mVg.mSamplerProposal = MHVariable::eFixe;

        else if (event->mPointType == Event::eNode)
            event->mVg.mSamplerProposal = MHVariable::eFixe;
        else
            event->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    }

    mLambdaSpline.setName(std::string("LambdaSpline of Curve"));
    if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed)
        mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
    else
        mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

    compute_XYZ = mCurveSettings.mProcessType == CurveSettings::eProcess_Vector ||
                mCurveSettings.mProcessType == CurveSettings::eProcess_Spherical ||
                mCurveSettings.mProcessType == CurveSettings::eProcess_3D;

    compute_Y = compute_XYZ ||
                mCurveSettings.mProcessType == CurveSettings::eProcess_2D ||
                mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec;

    compute_X_only = mCurveSettings.mProcessType == CurveSettings::eProcess_Univariate ||
                     mCurveSettings.mProcessType == CurveSettings::eProcess_Depth ||
                     mCurveSettings.mProcessType == CurveSettings::eProcess_Inclination ||
                     mCurveSettings.mProcessType == CurveSettings::eProcess_Declination ||
                     mCurveSettings.mProcessType == CurveSettings::eProcess_Field;

}

// Date files read / write
/** @Brief Save .res file, the result of computation and compress it
*
* */
void ModelCurve::saveToStream(QDataStream *out) const
{
    Model::saveToStream(out);

    if (!is_curve)
        return;
    /* -----------------------------------------------------
    *   Write curve data
    * ----------------------------------------------------- */
    // -----------------------------------------------------
    //  Write events VG
    // -----------------------------------------------------

    for (const std::shared_ptr<Event> &event : mEvents)
        *out << event->mVg;

    *out << mLambdaSpline;
    //*out << mS02Vg;

    *out << (quint32) mSplinesTrace.size();
    for (auto& splin : mSplinesTrace)
        *out << splin;

    *out << mPosteriorMeanG;

    *out << (quint32) mPosteriorMeanGByChain.size();
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *out << pMByChain;


}
/** @Brief Read the .res file, it's the result of the saved computation
*
* */
bool ModelCurve::loadFromStream_v323(QDataStream* in)
{

    bool ok = Model::loadFromStream_v323(in);

    if (!is_curve || !ok) {
        return ok;
    }
    /* -----------------------------------------------------
    *  Read events VG
    *----------------------------------------------------- */

    for (auto&& e : mEvents)
        *in >> e->mVg;

    /* -----------------------------------------------------
    *   Read curve data
    * ----------------------------------------------------- */
    quint32 tmp32;
    *in >> mLambdaSpline;

    *in >> mS02Vg;

    *in >> tmp32;
    mSplinesTrace.resize(tmp32);
    for (auto& splin : mSplinesTrace)
        *in >> splin;

    *in >> mPosteriorMeanG;

    *in >> tmp32;
    mPosteriorMeanGByChain.resize(tmp32);

    for (auto& pMByChain : mPosteriorMeanGByChain)
        *in >> pMByChain;

    return true;

}

bool ModelCurve::loadFromStream_v324(QDataStream* in)
{
    bool ok = Model::loadFromStream_v324(in);

    if (!is_curve || !ok) {
        return ok;
    }

    for (auto&& e : mEvents)
        *in >> e->mVg;

    quint32 tmp32;
    *in >> mLambdaSpline;

    *in >> mS02Vg;

    *in >> tmp32;
    mSplinesTrace.resize(tmp32);
    for (auto& splin : mSplinesTrace)
        *in >> splin;

    *in >> mPosteriorMeanG;

    *in >> tmp32;
    mPosteriorMeanGByChain.resize(tmp32);
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *in >> pMByChain;

    return true;
}

bool ModelCurve::loadFromStream_v328(QDataStream* in)
{
    bool ok = Model::loadFromStream_v328(in);

    if (!is_curve || !ok) {
        return ok;
    }

    for (auto&& e : mEvents)
        e->mVg.load_stream(*in);


    quint32 tmp32;
    mLambdaSpline.load_stream(*in);

    //*in >> mS02Vg;
    MHVariable oldSO2Vg;
    oldSO2Vg.load_stream(*in);

    *in >> tmp32;
    mSplinesTrace.resize(tmp32);
    for (auto& splin : mSplinesTrace)
        *in >> splin;

    *in >> mPosteriorMeanG;

    *in >> tmp32;
    mPosteriorMeanGByChain.resize(tmp32);
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *in >> pMByChain;

    return true;
}

bool ModelCurve::loadFromStream_v330(QDataStream* in)
{
    bool ok = Model::loadFromStream_v330(in);

    if (in->version()!= QDataStream::Qt_6_4) {
        std::cout << "[ModelCurve::loadFromStream_v330] Bad QDataStream version" << std::endl;
        return false;
    }

    // Gérer l'erreur de lecture ici
    if (in->status() != QDataStream::Ok) {
        std::cout << "[ModelCurve::loadFromStream_v330] QDataStream Status Error  =" << in->status() << std::endl;
        return false;
    }

    if (!is_curve || !ok) {
        return ok;
    }

    try {
        for (auto&& e : mEvents)
            e->mVg.load_stream(*in);


        quint32 tmp32;
        mLambdaSpline.load_stream(*in);

        // mS02Vg.load_stream(*in);

        *in >> tmp32;
        mSplinesTrace.resize(tmp32);
        for (auto& splin : mSplinesTrace)
            *in >> splin;

        *in >> mPosteriorMeanG;

        *in >> tmp32;
        mPosteriorMeanGByChain.resize(tmp32);
        for (auto& pMByChain : mPosteriorMeanGByChain)
            *in >> pMByChain;

        return true;

    } catch (...) {
        //std::cout << "[ModelCurve::loadFromStream_v330] error" << std::endl;
        return false;
    }


}


bool ModelCurve::loadFromStream_v335(QDataStream* in)
{
    bool ok = Model::loadFromStream_v330(in);

    // Gérer l'erreur de lecture ici
    if (in->status() != QDataStream::Ok) {
        qDebug() << "[ModelCurve::loadFromStream_v335]  erreur de flux ; in->status()=" << in->status();
        // throw std::runtime_error("Error reading from stream");
        // return;
    }


    if (!is_curve || !ok) {
        return ok;
    }

    try {
        for (auto&& e : mEvents)
            e->mVg.load_stream(*in);


        quint32 tmp32;
        mLambdaSpline.load_stream(*in);

        //*in >> mS02Vg;
        //mS02Vg.load_stream(*in);

        *in >> tmp32;
        mSplinesTrace.resize(tmp32);
        for (auto& splin : mSplinesTrace)
            *in >> splin;

        *in >> mPosteriorMeanG;

        *in >> tmp32;
        mPosteriorMeanGByChain.resize(tmp32);
        for (auto& pMByChain : mPosteriorMeanGByChain)
            *in >> pMByChain;

        return true;

    } catch (...) {
        std::cout << "[loadFromStream_v335] error" << std::endl;
        return false;
    }


}

void ModelCurve::saveMapToFile(QFile *file, const QString csvSep, const CurveMap& map)
{
    QTextStream output(file);
    const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
    output<<"# " + version + "\r";

    output << "# Date Format : "+ DateUtils::getAppSettingsFormatStr() + "\r";

   // auto mapG = map.data;

    const double stepX = (map.maxX() - map.minX()) / (map.column() - 1);
    const double stepY = (map.maxY() - map.minY()) / (map.row() - 1);

    /*  Export with date format, the data are stored with the BC/AD format,
     * So you have to reverse the export when the format is in Age
     */
    bool isDateFormat = DateUtils::convertToAppSettingsFormat(map.minX()) < DateUtils::convertToAppSettingsFormat(map.maxX());

    // Header
    if (isDateFormat) {
        output << "Y / Date"<< csvSep;
        for (unsigned c = 0; c < map.column(); ++c)  {
            output << stringForCSV(DateUtils::convertToAppSettingsFormat(map.minX() + c * stepX)) << csvSep;
        }
        output << "\r";

        for (int r = map.row() - 1; r >-1 ; --r)  {

            output << stringForCSV(map.minY() + r * stepY)  << csvSep;
            for (unsigned c = 0; c < map.column(); ++c)  {
                output << stringForCSV(map(c, r), true) << csvSep;
            }
            output << "\r";
        }
    } else {
        output << "Y / Age"<< csvSep;
        for (int c = map.column() - 1; c >-1; --c)  {
            output << stringForCSV(DateUtils::convertToAppSettingsFormat(map.minX() + c * stepX)) << csvSep;
        }
        output << "\r";

        for (int r = map.row() - 1; r >-1 ; --r)  {

            output << stringForCSV(map.minY() + r * stepY)  << csvSep;
            for (int c = map.column() - 1; c >-1; --c)   {
                output << stringForCSV(map(c, r), true) << csvSep;
            }
            output << "\r";
        }
    }
    file->close();
}

void ModelCurve::updateFormatSettings()
{
    Model::updateFormatSettings();

    if (is_curve) {
        for (std::shared_ptr<Event> &event : mEvents)
            event->mVg.setFormat(DateUtils::eNumeric);

        mLambdaSpline.setFormat(DateUtils::eNumeric);
    }
}

/**
 * @brief Model::generateModelLog
 * @return Return a QString with the recall of the Model with the data MCMC Methode, and constraint
 */
void ModelCurve::generateModelLog()
{
    mLogModel = ModelUtilities::modelDescriptionHTML(shared_from_this());
}

void ModelCurve::generateResultsLog()
{
    Model::generateResultsLog();
    if (is_curve) {
        QString log;
        log += ModelUtilities::curveResultsHTML(shared_from_this());
        log += "<hr>";

        mLogResults += log;
    }
}

void ModelCurve::generatePosteriorDensities(const std::vector<ChainSpecs> &chains, int fftLen, double bandwidth)
{
    Model::generatePosteriorDensities(chains, fftLen, bandwidth);
    if (is_curve) {
        for (std::shared_ptr<Event> &event : mEvents) {
            if (event->mVg.mSamplerProposal != MHVariable::eFixe)
                event->mVg.generateHistos(chains, fftLen, bandwidth);
        }

        mLambdaSpline.generateHistos(chains, fftLen, bandwidth);
    }

}

void ModelCurve::generateCorrelations(const std::vector<ChainSpecs> &chains)
{
#ifdef DEBUG
    qDebug()<<"[ModelCurve::generateCorrelations] in progress";
    QElapsedTimer t;
    t.start();
#endif
    Model::generateCorrelations(chains);
    if (is_curve) {
        for (auto&& event : mEvents )
            if (event->mVg.mSamplerProposal != MHVariable::eFixe)
                event->mVg.generateCorrelations(chains);

        if (mLambdaSpline.mSamplerProposal != MHVariable::eFixe)
            mLambdaSpline.generateCorrelations(chains);

    }
#ifdef DEBUG
    qDebug() <<  "=> [ModelCurve::generateCorrelations] done in " + DHMS(t.elapsed());
#endif
}

void ModelCurve::generateNumericalResults(const std::vector<ChainSpecs> &chains)
{
    Model::generateNumericalResults(chains);

    if (is_curve) {
        for (std::shared_ptr<Event>& event : mEvents) {
            if (event->mVg.mSamplerProposal != MHVariable::eFixe)
                event->mVg.generateNumericalResults(chains);
        }
        if (mLambdaSpline.mSamplerProposal != MHVariable::eFixe)
            mLambdaSpline.generateNumericalResults(chains);

    }
}

void ModelCurve::clearThreshold()
{
    Model::clearThreshold();

    if (getProject_ptr()->isCurve()) {
        for (std::shared_ptr<Event>& event : mEvents)
            event->mVg.mThresholdUsed = -1.0;

        mLambdaSpline.mThresholdUsed = -1.0;
    }
}

void ModelCurve::generateCredibility(const double thresh)
{
#ifdef DEBUG
    qDebug()<<QString("[ModelCurve::generateCredibility] Treshold = %1 %; in progress").arg(thresh);
    QElapsedTimer t;
    t.start();
#endif
    Model::generateCredibility(thresh);

    if (getProject_ptr()->isCurve()) {
        for (const auto& event : mEvents) {
            event->mVg.generateCredibility(mChains, thresh);
        }
        mLambdaSpline.generateCredibility(mChains, thresh);

    }
#ifdef DEBUG
    qDebug() <<  "[ModelCurve::generateCredibility] done in " + DHMS(t.elapsed()) ;
#endif
}

void ModelCurve::generateHPD(const double thresh)
{
    Model::generateHPD(thresh);

    if (getProject_ptr()->isCurve()) {
        for (const auto& event : mEvents) {
            if (event->pointType() != Event::eNode) {
                if (event->mVg.mSamplerProposal != MHVariable::eFixe)
                    event->mVg.generateHPD(thresh);
            }
        };

        if (mLambdaSpline.mSamplerProposal != MHVariable::eFixe)
            mLambdaSpline.generateHPD(thresh);

    }
}

void ModelCurve::remove_smoothed_densities()
{
    Model::remove_smoothed_densities();
    if (getProject_ptr()->isCurve()) {
        for (std::shared_ptr<Event>& event : mEvents) {
            if (event->type() != Event::eBound) {
                event->mVg.remove_smoothed_densities();
            }
        }

        mLambdaSpline.remove_smoothed_densities();

    }
}

void ModelCurve::clearCredibilityAndHPD()
{
    Model::clearCredibilityAndHPD();
    if (getProject_ptr()->isCurve()) {
        for (std::shared_ptr<Event>& event : mEvents) {
            if (event->type() != Event::eBound) {
                event->mVg.mFormatedHPD.clear();
                event->mVg.mFormatedCredibility = std::pair<double, double>(1, -1);
            }
        }

        mLambdaSpline.mFormatedHPD.clear();
        mLambdaSpline.mFormatedCredibility = std::pair<double, double>(1, -1);

    }
}



void ModelCurve::clearTraces()
{
    Model::clearTraces();

    if (getProject_ptr()->isCurve()) {
        mLambdaSpline.clear();
        mPosteriorMeanG.clear();
        for (auto &pbc : mPosteriorMeanGByChain) {
            pbc.clear();
        }
        mPosteriorMeanGByChain.clear();
    }

    for (auto &s : mSplinesTrace) {
        s.clear();
    }
    mSplinesTrace.clear();
}

void ModelCurve::clear()
{
    clearTraces();
    clearCredibilityAndHPD();
    remove_smoothed_densities();
    Model::clear();
}

void ModelCurve::shrink_to_fit() noexcept
{
    Model::shrink_to_fit();

    if (getProject_ptr()->isCurve()) {
        mLambdaSpline.shrink_to_fit();
        for (auto &pbc : mPosteriorMeanGByChain) {
            pbc.shrink_to_fit();
        }

        mPosteriorMeanGByChain.shrink_to_fit();

        mPosteriorMeanG.shrink_to_fit();

    }

    for (auto &s : mSplinesTrace) {
        s.shrink_to_fit();
    }
    mSplinesTrace.shrink_to_fit();
}


void ModelCurve::clear_and_shrink() noexcept
{
    Model::clear_and_shrink();
    if (getProject_ptr()->isCurve()) {
        mLambdaSpline.clear_and_shrink();
        mPosteriorMeanG.clear_and_shrink();
        for (auto &pbc : mPosteriorMeanGByChain) {
            pbc.clear_and_shrink();
        }
        mPosteriorMeanGByChain.clear();
        mPosteriorMeanGByChain.shrink_to_fit();
    }

    for (auto &s : mSplinesTrace) {
        s.clear_and_shrink();
    }
    mSplinesTrace.clear();
    mSplinesTrace.shrink_to_fit();

}

void ModelCurve::setThresholdToAllModel(const double threshold)
{
    Model::setThresholdToAllModel(threshold);
    if (getProject_ptr()->isCurve()) {
        for (std::shared_ptr<Event>& event : mEvents)
            event->mVg.mThresholdUsed = mThreshold;

        mLambdaSpline.mThresholdUsed = mThreshold;
    }
}

#pragma mark Loop
void ModelCurve::memo_accept(const unsigned i_chain)
{
    Model::memo_accept(i_chain);

    if (getProject_ptr()->isCurve()) {
    /* --------------------------------------------------------------
     *  D -  Memo S02 Vg - not Bayesian
     * -------------------------------------------------------------- */

    /* --------------------------------------------------------------
     *  E -  Memo Vg
     * -------------------------------------------------------------- */
    for (auto&& event : mEvents) {
        if (event->mVg.mSamplerProposal != MHVariable::eFixe) {
            event->mVg.memo_accept(i_chain);
        }
    }

    /* --------------------------------------------------------------
     * F - Memo Lambda
     * -------------------------------------------------------------- */
    // On stocke le log10 de Lambda Spline pour afficher les résultats a posteriori
    if (mLambdaSpline.mSamplerProposal != MHVariable::eFixe) {
        mLambdaSpline.memo_accept(i_chain);
    }

    }
}

/**
 * Idem Chronomodel + initialisation des variables aléatoires VG (events) et Lambda Spline (global)
 * TODO : initialisation des résultats g(t), g'(t), g"(t)
 */
void ModelCurve::initVariablesForChain()
{
    Model::initVariablesForChain();

    if (getProject_ptr()->isCurve()) {
    // today we have the same acceptBufferLen for every chain
    const int acceptBufferLen =  mChains.at(0).mIterPerBatch;
    /*int initReserve = 0;

    for (auto& c: mChains) {
        initReserve += ( 1 + (c.mMaxBatchs*c.mIterPerBatch) + c.mIterPerBurn + (c.mIterPerAquisition/c.mThinningInterval) );
    }
    */
    for (std::shared_ptr<Event>& event : mEvents) {
        //event->mVg.clear();
        //event->mVg.reserve(initReserve);
        event->mVg.mNbValuesAccepted.resize(mChains.size());
        //event->mVg.mLastAccepts.reserve(acceptBufferLen);
        event->mVg.mLastAcceptsLength = acceptBufferLen;
    }

    //mLambdaSpline.clear();
    //mLambdaSpline.reserve(initReserve);
    mLambdaSpline.mNbValuesAccepted.resize(mChains.size());
    //mLambdaSpline.mLastAccepts.reserve(acceptBufferLen);
    mLambdaSpline.mLastAcceptsLength = acceptBufferLen;


    // Ré-initialisation du stockage des splines
    //mSplinesTrace.clear();

    // Ré-initialisation des résultats
    //mPosteriorMeanGByChain.clear();
    }
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteX()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gx);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteY()
{
    QList<PosteriorMeanGComposante> composantes;

    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gy);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteZ()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gz);

    return composantes;
}
/*
void ModelCurve::valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, const Model &model)
{
    const unsigned long n = spline.vecThetaReduced.size();
    const t_reduceTime tReduce =  model.reduceTime(t);
    const t_reduceTime t1 = spline.vecThetaReduced.at(0);
    const t_reduceTime tn = spline.vecThetaReduced.at(n-1);
    GP = 0.;
    GS = 0.;
    double h;

     // The first derivative is always constant outside the interval [t1,tn].
     if (tReduce < t1) {
         const t_reduceTime t2 = spline.vecThetaReduced.at(1);

         // ValeurGPrime
         GP = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
         GP -= (t2 - t1) * spline.vecGamma.at(1) / 6.;

         // ValeurG
         G = spline.vecG.at(0) - (t1 - tReduce) * GP;

         // valeurErrG
         varG = spline.vecVarG.at(0);

         // valeurGSeconde
         //GS = 0.;

     } else if (tReduce >= tn) {

         const t_reduceTime tn1 = spline.vecThetaReduced.at(n-2);

         // valeurErrG
         varG = spline.vecVarG.at(n-1);

         // ValeurGPrime
         GP = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
         GP += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;

         // valeurGSeconde
         //GS =0.;

         // ValeurG
         G = spline.vecG.at(n-1) + (tReduce - tn) * GP;


     } else {
        double err1, err2;
         for (; i0 < n-1; ++i0) {
             const t_reduceTime ti1 = spline.vecThetaReduced.at(i0);
             const t_reduceTime ti2 = spline.vecThetaReduced.at(i0 + 1);
             h = ti2 - ti1;

             if ((tReduce >= ti1) && (tReduce < ti2)) {

                 const double gi1 = spline.vecG.at(i0);
                 const double gi2 = spline.vecG.at(i0 + 1);
                 const double gamma1 = spline.vecGamma.at(i0);
                 const double gamma2 = spline.vecGamma.at(i0 + 1);

                 // ValeurG

                 G = ( (tReduce-ti1)*gi2 + (ti2-tReduce)*gi1 ) /h;
                  // Smoothing part :
                 G -= (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);

                 err1 = sqrt(spline.vecVarG.at(i0));
                 err2 = sqrt(spline.vecVarG.at(i0 + 1));
                 varG = pow(err1 + ((tReduce-ti1) / (ti2-ti1)) * (err2 - err1) , 2.l);
#ifdef DEBUG
                 if (std::isnan(varG))
                    qDebug()<< "[ModelCurve] varG is nan ??"<<ti1<<ti2;
#endif
                 GP = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                 GP += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                 // valeurGSeconde
                 GS = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;


                 break;
             }
         }

     }

     // Value slope correction
     GP /=(model.mSettings.mTmax - model.mSettings.mTmin);
     GS /= pow(model.mSettings.mTmax - model.mSettings.mTmin, 2.);
}
*/
void ModelCurve::valeurs_G_varG_on_i(const MCMCSplineComposante &spline, double &G, double &varG, unsigned long &i)
{
    const double n = spline.vecThetaReduced.size();
    if ((i > 0) && (i < n-1)) {

        const t_reduceTime tReduce =  spline.vecThetaReduced.at(i); //(t - tmin) / (tmax - tmin);

        const t_reduceTime ti1 = spline.vecThetaReduced.at(i);
        const t_reduceTime ti2 = spline.vecThetaReduced.at(i + 1);
        const double h = ti2 - ti1;
        const double gi1 = spline.vecG.at(i);

        // ValeurG
        G =  (ti2-tReduce)*gi1 /h;

        varG = spline.vecVarG.at(i);

    } else if (i == 0 || i == n-1) {
        G = spline.vecG.at(i);
        varG = spline.vecVarG.at(i);

    } else  {
        G = 0;
        varG = 0;
    }

}



/**
 * @brief ModelCurve::exportMeanGComposanteToReferenceCurves
 * @param defaultPath
 * @param locale
 * @param csvSep
 * @param step
 */
void ModelCurve::exportMeanGComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString& defaultPath, QLocale csvLocale, const QString& csvSep) const
{
    QString filter = QObject::tr("CSV (*.csv)");
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    QObject::tr("Save Ref. Curve as..."),
                                                    defaultPath,
                                                    filter);
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug()<<"ModelCurve::exportMeanGComposanteToReferenceCurves";

        QList<QStringList> rows;
        QStringList list;
        // 1 -Create the header
        list << "X Axis";
        list << "G";
        list << "err G";
        double xMin = mSettings.mTmin;
        double xMax = mSettings.mTmax;

        const double step = (xMax - xMin)/(pMeanCompoXYZ.vecG.size() - 1);

        rows<<list;
        //rows.reserve(pMeanCompoXYZ.vecG.size());

        // 3 - Create Row, with each curve
        //  Create data in row

        csvLocale.setNumberOptions(QLocale::OmitGroupSeparator);
        for (unsigned long i = 0; i < pMeanCompoXYZ.vecG.size(); ++i) {
            const double x = i*step + xMin;
            list.clear();

            list << csvLocale.toString(x);
            // Il doit y avoir au moins trois courbes G, GSup, Ginf et nous exportons G et ErrG
            const double xi = pMeanCompoXYZ.vecG[i]; // G
            const double var_xi =  pMeanCompoXYZ.vecVarG[i];
            list<<csvLocale.toString(xi, 'g', 15);
            list<<csvLocale.toString(sqrt(var_xi), 'g', 15);

            rows<<list;
        }

        // 4 - Save Qlist

        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        const QString projectName = QObject::tr("Project filename : %1").arg(MainWindow::getInstance()->getNameProject());

        output << "# "+ version + "\r";
        output << "# "+ projectName + "\r";
        output << "# BC/AD \r";//DateUtils::getAppSettingsFormatStr() + "\r";

        for (auto& row : rows) {
            output << row.join(csvSep);
            output << "\r";
        }
        file.close();
    }

}

std::vector<MCMCSpline> ModelCurve::fullRunSplineTrace(const std::vector<ChainSpecs>& chains)
{
  // Calcul reserve space
    int reserveSize = 0;

    for (const ChainSpecs& chain : chains)
        reserveSize += chain.mRealyAccepted;

    std::vector<MCMCSpline> splineRunTrace(reserveSize);

    int shift = 0;
    int shiftTrace = 0;

    for (const ChainSpecs& chain : chains) {
        // we add 1 for the init
        const int burnAdaptSize = 0;//1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
        const int runTraceSize = chain.mRealyAccepted; // there is only accepted curves
        const int firstRunPosition = shift + burnAdaptSize;
        std::copy(mSplinesTrace.begin() + firstRunPosition , mSplinesTrace.begin() + firstRunPosition + runTraceSize , splineRunTrace.begin() + shiftTrace);

        shiftTrace += runTraceSize;
        shift = firstRunPosition +runTraceSize;
    }
    return splineRunTrace;
}

std::vector<MCMCSpline> ModelCurve::runSplineTraceForChain(const std::vector<ChainSpecs> &chains, const size_t index)
{
    size_t shift = 0;

    for (size_t i = 0; i<chains.size(); i++) {
        const auto &chain = chains.at(i);
        // we add 1 for the init
        //const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
        const int burnAdaptSize = 0; // there is only accepted curves
        const int runTraceSize = chain.mRealyAccepted;
        const int firstRunPosition = shift + burnAdaptSize;

        if (i == index) {
            return std::vector<MCMCSpline> (mSplinesTrace.begin() + firstRunPosition , mSplinesTrace.begin() + firstRunPosition + runTraceSize);
        }
        shift = firstRunPosition + runTraceSize;
    }
    return std::vector<MCMCSpline> ();
}

#pragma mark memo_PosteriorG_XYZ() for 3.3.0
#if VERSION_MAJOR == 3 && VERSION_MINOR >= 2 && VERSION_PATCH >= 0
void ModelCurve::memo_PosteriorG_XYZ(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted)
{
    CurveMap& curveMap_X = postG.gx.mapG;
    CurveMap& curveMap_Y = postG.gy.mapG;
    CurveMap& curveMap_Z = postG.gz.mapG;

    const int nbPtsX = curveMap_X.column(); // identique à toutes les maps

    const int nbPtsY_X = curveMap_X.row();
    const int nbPtsY_Y = curveMap_Y.row();
    const int nbPtsY_Z = curveMap_Z.row();

    const double ymin_X = curveMap_X.minY();
    const double ymax_X = curveMap_X.maxY();

    const double ymin_Y = curveMap_Y.minY();
    const double ymax_Y = curveMap_Y.maxY();

    const double ymin_Z = curveMap_Z.minY();
    const double ymax_Z = curveMap_Z.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);

    const double stepY_X = (ymax_X - ymin_X) / (nbPtsY_X - 1);
    const double stepY_Y = (ymax_Y - ymin_Y) / (nbPtsY_Y - 1);
    const double stepY_Z = (ymax_Z - ymin_Z) / (nbPtsY_Z - 1);
    // variable for GP

    CurveMap& curveMap_GP_X = postG.gx.mapGP;
    CurveMap& curveMap_GP_Y = postG.gy.mapGP;
    CurveMap& curveMap_GP_Z = postG.gz.mapGP;

    // const int nbPts_GP_X = curveMap_GP_ZF->column(); // identique à toutes les maps

    const int nbPtsY_GP_X = curveMap_GP_X.row();
    const int nbPtsY_GP_Y = curveMap_GP_Y.row();
    const int nbPtsY_GP_Z = curveMap_GP_Z.row();

    const double ymin_GP_X = curveMap_GP_X.minY();
    const double ymax_GP_X = curveMap_GP_X.maxY();

    const double ymin_GP_Y = curveMap_GP_Y.minY();
    const double ymax_GP_Y = curveMap_GP_Y.maxY();

    const double ymin_GP_Z = curveMap_GP_Z.minY();
    const double ymax_GP_Z = curveMap_GP_Z.maxY();

    //const double step_GP_T = (mSettings.mTmax - mSettings.mTmin) / (nbPts_GP_X - 1);

    const double step_GP_X = (ymax_GP_X - ymin_GP_X) / (nbPtsY_GP_X - 1);
    const double step_GP_Y = (ymax_GP_Y - ymin_GP_Y) / (nbPtsY_GP_Y - 1);
    const double step_GP_Z = (ymax_GP_Z - ymin_GP_Z) / (nbPtsY_GP_Z - 1);

    // 2 - Variables temporaires
    /*
     * LEs calculs de variances ne se calculent que dans l'espace X, Y, Z
     * Il faut convertir juste au moment de la sauvegarde
     * */

    // référence sur la variance globale à dessiner
    std::vector<double> &vecVarG_X = postG.gx.vecVarG;
    std::vector<double> &vecVarG_Y = postG.gy.vecVarG;
    std::vector<double> &vecVarG_Z = postG.gz.vecVarG;

    // Variables temporaires
    // erreur inter spline
    std::vector<double> &vecVarianceG_X = postG.gx.vecVarianceG;
    std::vector<double> &vecVarianceG_Y = postG.gy.vecVarianceG;
    std::vector<double> &vecVarianceG_Z = postG.gz.vecVarianceG;
    // erreur intra spline
    std::vector<double> &vecVarIntraG_X = postG.gx.vecVarErrG;
    std::vector<double> &vecVarIntraG_Y = postG.gy.vecVarErrG;
    std::vector<double> &vecVarIntraG_Z = postG.gz.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG_X = postG.gx.vecG.begin();
    std::vector<double>::iterator itVecGP_X = postG.gx.vecGP.begin();
    std::vector<double>::iterator itVecGS_X = postG.gx.vecGS.begin();

    std::vector<double>::iterator itVecG_Y = postG.gy.vecG.begin();
    std::vector<double>::iterator itVecGP_Y = postG.gy.vecGP.begin();
    std::vector<double>::iterator itVecGS_Y = postG.gy.vecGS.begin();

    std::vector<double>::iterator itVecG_Z = postG.gz.vecG.begin();
    std::vector<double>::iterator itVecGP_Z = postG.gz.vecGP.begin();
    std::vector<double>::iterator itVecGS_Z = postG.gz.vecGS.begin();

    // Pointeur
    // erreur inter spline, dans l'espace X, Y, Z
    std::vector<double>::iterator itVecVarianceG_X = postG.gx.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_Y = postG.gy.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_Z = postG.gz.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarIntraG_X = postG.gx.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarIntraG_Y = postG.gy.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarIntraG_Z = postG.gz.vecVarErrG.begin();

    // inter derivate variance

    double t;
    double gx, gpx, gsx, varIntraGx = 0;
    double gy, gpy, gsy, varIntraGy = 0;
    double gz, gpz, gsz, varIntraGz = 0;

    const double n = realyAccepted;
    double prevMeanG_X, prevMeanG_Y, prevMeanG_Z;

    constexpr double k = 3.0; // Le nombre de fois sigma G, pour le calcul de la densité

    int  idxYErrMin, idxYErrMax;


    // 3 - Calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idx_t = 0; idx_t < nbPtsX ; ++idx_t) {
        t = (double)idx_t * stepT + mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, spline.splineX, gx, varIntraGx, gpx, gsx, i0, mSettings.mTmin, mSettings.mTmax);
        valeurs_G_VarG_GP_GS(t, spline.splineY, gy, varIntraGy, gpy, gsy, i0, mSettings.mTmin, mSettings.mTmax);

        if (compute_XYZ)
            valeurs_G_VarG_GP_GS(t, spline.splineZ, gz, varIntraGz, gpz, gsz, i0, mSettings.mTmin, mSettings.mTmax);


        // -- Calcul Mean

        prevMeanG_X = *itVecG_X;
        *itVecG_X +=  (gx - prevMeanG_X)/n;
        *itVecGP_X +=  (gpx - *itVecGP_X)/n;
        *itVecGS_X +=  (gsx - *itVecGS_X)/n;

        prevMeanG_Y = *itVecG_Y;
        *itVecG_Y +=  (gy - prevMeanG_Y)/n;
        *itVecGP_Y +=  (gpy - *itVecGP_Y)/n;
        *itVecGS_Y +=  (gsy - *itVecGS_Y)/n;


        if (compute_XYZ) {
            prevMeanG_Z = *itVecG_Z;
            *itVecG_Z +=  (gz - prevMeanG_Z)/n;

            *itVecGP_Z +=  (gpz - *itVecGP_Z)/n;
            *itVecGS_Z +=  (gsz - *itVecGS_Z)/n;
        }


        // Erreur inter spline
        *itVecVarianceG_X +=  (gx - prevMeanG_X)*(gx - *itVecG_X);
        *itVecVarianceG_Y +=  (gy - prevMeanG_Y)*(gy - *itVecG_Y);
        // Erreur intra spline
        *itVecVarIntraG_X += (varIntraGx - *itVecVarIntraG_X) / n  ;
        *itVecVarIntraG_Y += (varIntraGy - *itVecVarIntraG_Y) / n  ;

        if (compute_XYZ) {
            // erreur inter spline
            *itVecVarianceG_Z +=  (gz - prevMeanG_Z)*(gz - *itVecG_Z);
            // erreur intra spline
            *itVecVarIntraG_Z += (varIntraGz - *itVecVarIntraG_Z) / n  ;
        }


        ++itVecG_X;
        ++itVecGP_X;
        ++itVecGS_X;
        ++itVecVarianceG_X;
        ++itVecVarIntraG_X;

        ++itVecG_Y;
        ++itVecGP_Y;
        ++itVecGS_Y;
        ++itVecVarianceG_Y;
        ++itVecVarIntraG_Y;

        if (compute_XYZ) {
            ++itVecG_Z;
            ++itVecGP_Z;
            ++itVecGS_Z;
            ++itVecVarianceG_Z;
            ++itVecVarIntraG_Z;
        }

        // -------- Calcul map on XInc ----------
        double stdMap_X = sqrt(varIntraGx);


        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        //idxYErrMin = std::clamp( int((gx - k*stdMap_X - ymin_X) / stepY_X), 0, nbPtsY_X - 1);
        idxYErrMin = floor((gx - k*stdMap_X - ymin_X)/stepY_X + 0.5);
        if (idxYErrMin < 0) {
            idxYErrMin = 0;
        } else if (idxYErrMin > nbPtsY_X - 1) {
            idxYErrMin = nbPtsY_X - 1;
        }

        idxYErrMax = floor((gx + k*stdMap_X - ymin_X)/stepY_X + 0.5);
        if (idxYErrMax < 0) {
            idxYErrMax = 0;
        } else if (idxYErrMax > nbPtsY_X - 1) {
            idxYErrMax = nbPtsY_X-1;
        }

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_X - 1) {
            ++curveMap_X(idx_t, idxYErrMin);
            curveMap_X.setMaxValue(std::max(curveMap_X.maxValue(), curveMap_X.at(idx_t, idxYErrMin)) );

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_X) {
            double* ptr_Ymin = curveMap_X.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_X.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_X + ymin_X;
                double b = (idErr + 0.5) * stepY_X + ymin_X;
                double surfG = diff_erf(a, b, gx, stdMap_X );
                *ptr_idErr = (*ptr_idErr) + surfG;
                curveMap_X.setMaxValue(std::max(curveMap_X.maxValue(), *ptr_idErr));

                idErr++;
            }


        }

        //double idx_GP_X = std::clamp( int((gpx - ymin_GP_X) / step_GP_X), 0, nbPtsY_GP_X - 1);

        double idx_GP_X = floor((gpx - ymin_GP_X)/step_GP_X + 0.5);
        if (idx_GP_X < 0) {
            idx_GP_X = 0;
        } else if (idx_GP_X > nbPtsY_GP_X - 1) {
            idx_GP_X = nbPtsY_GP_X - 1;
        }
        ++curveMap_GP_X(idx_t, idx_GP_X);
        curveMap_GP_X.setMaxValue(std::max(curveMap_GP_X.maxValue(), curveMap_GP_X.at(idx_t, idx_GP_X)));



        // -- Calcul map on YDec

        //auto stdIntraGy = sqrt(varIntraGy);
        double stdMap_Y = sqrt(varIntraGy);

        // Ajout densité erreur sur Y
        /* Il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
        * https://en.wikipedia.org/wiki/Error_function
        */
        idxYErrMin = std::clamp( int((gy - k*stdMap_Y - ymin_Y) / stepY_Y), 0, nbPtsY_Y -1);
        idxYErrMax = std::clamp( int((gy + k*stdMap_Y - ymin_Y) / stepY_Y), 0, nbPtsY_Y -1);


        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_Y - 1) {

            ++curveMap_Y(idx_t, idxYErrMin); // correction à faire dans finalize/nbIter ;

            curveMap_Y.setMaxValue(std::max(curveMap_Y.maxValue(), curveMap_Y.at(idx_t, idxYErrMin)) );

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_Y) {
            double* ptr_Ymin = curveMap_Y.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_Y.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_Y + ymin_Y;
                double b = (idErr + 0.5) * stepY_Y + ymin_Y;
                double surfG = diff_erf(a, b, gy, stdMap_Y );

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_Y.setMaxValue(std::max(curveMap_Y.maxValue(), *ptr_idErr));

                idErr++;
            }

        }

        double idx_GP_Y = std::clamp( int((gpy - ymin_GP_Y) / step_GP_Y), 0, nbPtsY_GP_Y - 1);
        ++curveMap_GP_Y(idx_t, idx_GP_Y);
        curveMap_GP_Y.setMaxValue(std::max(curveMap_GP_Y.maxValue(), curveMap_GP_Y.at(idx_t, idx_GP_Y)));


        if (compute_XYZ) {

            // -- Calcul map on ZF

            // curveMap = curveMap_ZF;//postG.gz.mapG;
            //const auto stdIntraGz = sqrt(varIntraGz);
            double stdMap_Z = sqrt(varIntraGz);

            // ajout densité erreur sur Y
            /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
             * https://en.wikipedia.org/wiki/Error_function
             */
            //idxYErrMin = std::clamp( int((gz - k * stdMap_Z - ymin_Z) / stepY_Z), 0, nbPtsY_Z-1);
            //idxYErrMax = std::clamp( int((gz + k * stdMap_Z - ymin_Z) / stepY_Z), 0, nbPtsY_Z-1);

            idxYErrMin = floor((gz - k*stdMap_Z - ymin_Z)/stepY_Z + 0.5);
            if (idxYErrMin < 0) {
                idxYErrMin = 0;
            } else if (idxYErrMin > nbPtsY_Z - 1) {
                idxYErrMin = nbPtsY_Z - 1;
            }

            idxYErrMax = floor((gz + k*stdMap_Z - ymin_Z)/stepY_Z + 0.5);
            if (idxYErrMax < 0) {
                idxYErrMax = 0;
            } else if (idxYErrMax > nbPtsY_Z - 1) {
                idxYErrMax = nbPtsY_Z - 1;
            }

            if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_Z - 1) {

                ++curveMap_Z(idx_t, idxYErrMin);

                curveMap_Z.setMaxValue(std::max(curveMap_Z.maxValue(), curveMap_Z.at(idx_t, idxYErrMin)));


            } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_Z) {
                double* ptr_Ymin = curveMap_Z.dataPtr(idx_t, idxYErrMin);
                double* ptr_Ymax = curveMap_Z.dataPtr(idx_t, idxYErrMax);

                int idErr = idxYErrMin;
                for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                    double a = (idErr - 0.5) * stepY_Z + ymin_Z;
                    double b = (idErr + 0.5) * stepY_Z + ymin_Z;
                    double surfG = diff_erf(a, b, gz, stdMap_Z );

                    *ptr_idErr = (*ptr_idErr) + surfG;

                    curveMap_Z.setMaxValue(std::max(curveMap_Z.maxValue(), *ptr_idErr));

                    idErr++;
                }

            }
            //double idx_GP_Z = std::clamp( int((gpz - ymin_GP_Z) / step_GP_Z), 0, nbPtsY_GP_Z - 1);
            double idx_GP_Z = floor((gpz - ymin_GP_Z)/step_GP_Z + 0.5);
            if (idxYErrMax < 0) {
                idxYErrMax = 0;
            } else if (idxYErrMax > nbPtsY_GP_Z - 1) {
                idxYErrMax = nbPtsY_GP_Z - 1;
            }
            ++curveMap_GP_Z(idx_t, idx_GP_Z);
            curveMap_GP_Z.setMaxValue(std::max(curveMap_GP_Z.maxValue(), curveMap_GP_Z.at(idx_t, idx_GP_Z)));

        }


    }

    for (size_t tIdx = 0; tIdx < vecVarG_X.size(); ++tIdx) {
        vecVarG_X[tIdx] = vecVarianceG_X.at(tIdx) / n + vecVarIntraG_X.at(tIdx);
        vecVarG_Y[tIdx] = vecVarianceG_Y.at(tIdx) / n + vecVarIntraG_Y.at(tIdx);
        vecVarG_Z[tIdx] = vecVarianceG_Z.at(tIdx) / n + vecVarIntraG_Z.at(tIdx);
    }

}

// variances calculées dans l'espace X, Y, Z
void ModelCurve::memo_PosteriorG_IDF(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted)
{
    constexpr double rad = M_PI / 180.0;
    constexpr double deg = 180.0 * M_1_PI;
    constexpr double deg2 = deg * deg;

    CurveMap& curveMap_Inc = postG.gx.mapG;
    CurveMap& curveMap_Dec = postG.gy.mapG;
    CurveMap& curveMap_F = postG.gz.mapG;

    const int nbPtsX = curveMap_Inc.column(); // identique à toutes les maps

    const int nbPtsY_Inc = curveMap_Inc.row();
    const int nbPtsY_Dec = curveMap_Dec.row();
    const int nbPtsY_F = curveMap_F.row();

    const double ymin_Inc = curveMap_Inc.minY();
    const double ymax_Inc = curveMap_Inc.maxY();

    const double ymin_Dec = curveMap_Dec.minY();
    const double ymax_Dec = curveMap_Dec.maxY();

    const double ymin_F = curveMap_F.minY();
    const double ymax_F = curveMap_F.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);

    const double stepY_Inc = (ymax_Inc - ymin_Inc) / (nbPtsY_Inc - 1);
    const double stepY_Dec = (ymax_Dec - ymin_Dec) / (nbPtsY_Dec - 1);
    const double stepY_F = (ymax_F - ymin_F) / (nbPtsY_F - 1);
    // variable for GP

    CurveMap& curveMap_GP_Inc = postG.gx.mapGP;
    CurveMap& curveMap_GP_Dec = postG.gy.mapGP;
    CurveMap& curveMap_GP_F = postG.gz.mapGP;

    // const int nbPts_GP_X = curveMap_GP_ZF->column(); // identique à toutes les maps

    const int nbPtsY_GP_Inc = curveMap_GP_Inc.row();
    const int nbPtsY_GP_Dec = curveMap_GP_Dec.row();
    const int nbPtsY_GP_F = curveMap_GP_F.row();

    const double ymin_GP_Inc = curveMap_GP_Inc.minY();
    const double ymax_GP_Inc = curveMap_GP_Inc.maxY();

    const double ymin_GP_Dec = curveMap_GP_Dec.minY();
    const double ymax_GP_Dec = curveMap_GP_Dec.maxY();

    const double ymin_GP_F = curveMap_GP_F.minY();
    const double ymax_GP_F = curveMap_GP_F.maxY();


    const double step_GP_Inc = (ymax_GP_Inc - ymin_GP_Inc) / (nbPtsY_GP_Inc - 1);
    const double step_GP_Dec = (ymax_GP_Dec - ymin_GP_Dec) / (nbPtsY_GP_Dec - 1);
    const double step_GP_F = (ymax_GP_F - ymin_GP_F) / (nbPtsY_GP_F - 1);

    // 2 - Variables temporaires
    /*
     * LEs calculs de variances ne se calculent que dans l'espace X, Y, Z
     * Il faut convertir juste au moment de la sauvegarde
     * */

    // référence sur la variance globale à dessiner
    std::vector<double> &vecVarG_Inc = postG.gx.vecVarG;
    std::vector<double> &vecVarG_Dec = postG.gy.vecVarG;
    std::vector<double> &vecVarG_F = postG.gz.vecVarG;

    // Variables temporaires
    // erreur inter spline
    std::vector<double> &vecVarianceG_X = postG.gx.vecVarianceG;
    std::vector<double> &vecVarianceG_Y = postG.gy.vecVarianceG;
    std::vector<double> &vecVarianceG_Z = postG.gz.vecVarianceG;
    // erreur intra spline
    std::vector<double> &vecVarIntraG_X = postG.gx.vecVarErrG;
    std::vector<double> &vecVarIntraG_Y = postG.gy.vecVarErrG;
    std::vector<double> &vecVarIntraG_Z = postG.gz.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG_Inc = postG.gx.vecG.begin();
    std::vector<double>::iterator itVecGP_Inc = postG.gx.vecGP.begin();
    std::vector<double>::iterator itVecGS_Inc = postG.gx.vecGS.begin();

    std::vector<double>::iterator itVecG_Dec = postG.gy.vecG.begin();
    std::vector<double>::iterator itVecGP_Dec = postG.gy.vecGP.begin();
    std::vector<double>::iterator itVecGS_Dec = postG.gy.vecGS.begin();

    std::vector<double>::iterator itVecG_F = postG.gz.vecG.begin();
    std::vector<double>::iterator itVecGP_F = postG.gz.vecGP.begin();
    std::vector<double>::iterator itVecGS_F = postG.gz.vecGS.begin();

    // Pointeur
    // erreur inter spline, dans l'espace X, Y, Z
    std::vector<double>::iterator itVecVarianceG_X = postG.gx.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_Y = postG.gy.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_Z = postG.gz.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarIntraG_X = postG.gx.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarIntraG_Y = postG.gy.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarIntraG_Z = postG.gz.vecVarErrG.begin();

    // inter derivate variance

    double t;
    double gx, gpx, gsx, varIntraGx = 0;
    double gy, gpy, gsy, varIntraGy = 0;
    double gz, gpz, gsz, varIntraGz = 0;

    double Inc, Dec, F;
    double dpInc, dpDec, dpF; // prime derivatives
    double dsInc, dsDec, dsF; // seconde derivatives


    double n = realyAccepted;

    constexpr double k = 3.0; // Le nombre de fois sigma G, pour le calcul de la densité

    int  idxYErrMin, idxYErrMax;


    // 3 - Calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idx_t = 0; idx_t < nbPtsX ; ++idx_t) {
        t = (double)idx_t * stepT + mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, spline.splineX, gx, varIntraGx, gpx, gsx, i0, mSettings.mTmin, mSettings.mTmax);
        valeurs_G_VarG_GP_GS(t, spline.splineY, gy, varIntraGy, gpy, gsy, i0, mSettings.mTmin, mSettings.mTmax);
        valeurs_G_VarG_GP_GS(t, spline.splineZ, gz, varIntraGz, gpz, gsz, i0, mSettings.mTmin, mSettings.mTmax);


        // -- Calcul Mean
        convertToIDF(gx, gy, gz, Inc, Dec, F);

        computeDerivatives(gx, gy, gz, gpx, gpy, gpz,
                           dpInc, dpDec, dpF);

        computeSecondDerivatives(gx, gy, gz, gpx, gpy, gpz, gsx, gsy, gsz,
                                 dsInc, dsDec, dsF);

        double prevMeanG_Inc= *itVecG_Inc;
        double prevMeanG_Dec = *itVecG_Dec;
        double prevMeanG_F = *itVecG_F;

        double prevMeanG_X, prevMeanG_Y, prevMeanG_Z;
        convertToXYZ(prevMeanG_Inc, prevMeanG_Dec, prevMeanG_F,
                     prevMeanG_X, prevMeanG_Y, prevMeanG_Z );

        *itVecG_Inc +=  (Inc - prevMeanG_Inc)/n;
        *itVecGP_Inc +=  (dpInc - *itVecGP_Inc)/n;
        *itVecGS_Inc +=  (dsInc - *itVecGS_Inc)/n;

        *itVecG_Dec +=  (Dec - prevMeanG_Dec)/n;
        *itVecGP_Dec +=  (dpDec - *itVecGP_Dec)/n;
        *itVecGS_Dec +=  (dsDec - *itVecGS_Dec)/n;

        *itVecG_F +=  (F - prevMeanG_F)/n;

        *itVecGP_F +=  (dpF - *itVecGP_F)/n;
        *itVecGS_F +=  (dsF - *itVecGS_F)/n;

        // Variance inter spline
        double meanG_X, meanG_Y, meanG_Z;
        convertToXYZ(*itVecG_Inc, *itVecG_Dec, *itVecG_F,
                     meanG_X, meanG_Y, meanG_Z );

        *itVecVarianceG_X +=  (gx - prevMeanG_X)*(gx - meanG_X);
        *itVecVarianceG_Y +=  (gy - prevMeanG_Y)*(gy - meanG_Y);
        *itVecVarianceG_Z +=  (gz - prevMeanG_Z)*(gz - meanG_Z);

        // Variance moyenne intra spline
        *itVecVarIntraG_X += (varIntraGx - *itVecVarIntraG_X) / n  ;
        *itVecVarIntraG_Y += (varIntraGy - *itVecVarIntraG_Y) / n  ;
        *itVecVarIntraG_Z += (varIntraGz - *itVecVarIntraG_Z) / n  ;

        // increment pointeurs
        ++itVecG_Inc;
        ++itVecGP_Inc;
        ++itVecGS_Inc;
        ++itVecVarianceG_X;
        ++itVecVarIntraG_X;

        ++itVecG_Dec;
        ++itVecGP_Dec;
        ++itVecGS_Dec;
        ++itVecVarianceG_Y;
        ++itVecVarIntraG_Y;

        ++itVecG_F;
        ++itVecGP_F;
        ++itVecGS_F;
        ++itVecVarianceG_Z;
        ++itVecVarIntraG_Z;


        // -------- Calcul map on XInc ----------

        double std_IntraF = sqrt((varIntraGx + varIntraGy + varIntraGz)/3.0);

        double std_IntraInc = std_IntraF * deg / F ;
        double std_IntraDec = std_IntraF * deg / (F * cos(Inc * rad)) ;



        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        //idxYErrMin = std::clamp( int((Inc - k * std_IntraInc - ymin_Inc) / stepY_Inc), 0, nbPtsY_Inc - 1);
        //idxYErrMax = std::clamp( int((Inc + k * std_IntraInc - ymin_Inc) / stepY_Inc), 0, nbPtsY_Inc - 1);
        idxYErrMin = floor((Inc - k*std_IntraInc - ymin_Inc)/stepY_Inc + 0.5);
        if (idxYErrMin < 0) {
            idxYErrMin = 0;
        } else if (idxYErrMin > nbPtsY_Inc - 1) {
            idxYErrMin = nbPtsY_Inc - 1;
        }

        idxYErrMax = floor((Inc + k*std_IntraInc - ymin_Inc)/stepY_Inc + 0.5);
        if (idxYErrMax < 0) {
            idxYErrMax = 0;
        } else if (idxYErrMax > nbPtsY_Inc - 1) {
            idxYErrMax = nbPtsY_Inc - 1;
        }

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_Inc - 1) {

            ++curveMap_Inc(idx_t, idxYErrMin); // correction à faire dans finalize/nbIter ;

            curveMap_Inc.setMaxValue(std::max(curveMap_Inc.maxValue(), curveMap_Inc.at(idx_t, idxYErrMin)));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_Inc) {
            double* ptr_Ymin = curveMap_Inc.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_Inc.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_Inc + ymin_Inc;
                double b = (idErr + 0.5) * stepY_Inc + ymin_Inc;
                double surfG = diff_erf(a, b, Inc, std_IntraInc );// correction à faire dans finalyze /nbIter;

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_Inc.setMaxValue( std::max(curveMap_Inc.maxValue(), *ptr_idErr));

                idErr++;
            }

        }
        //double idx_GP_Inc = std::clamp( int((dpInc - ymin_GP_Inc) / step_GP_Inc), 0, nbPtsY_GP_Inc - 1);
        double idx_GP_Inc = floor((dpInc - ymin_GP_Inc)/step_GP_Inc + 0.5);
        if (idx_GP_Inc < 0) {
            idx_GP_Inc = 0;
        } else if (idx_GP_Inc > nbPtsY_GP_Inc - 1) {
            idx_GP_Inc = nbPtsY_GP_Inc - 1;
        }

        ++curveMap_GP_Inc(idx_t, idx_GP_Inc);
        curveMap_GP_Inc.setMaxValue(std::max(curveMap_GP_Inc.maxValue(), curveMap_GP_Inc.at(idx_t, idx_GP_Inc)));



        // -- Calcul map on YDec

        // Ajout densité erreur sur Y
        /* Il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
        * https://en.wikipedia.org/wiki/Error_function
        */
        //idxYErrMin = std::clamp( int((Dec - k * std_IntraDec - ymin_Dec) / stepY_Dec), 0, nbPtsY_Dec -1);
        //idxYErrMax = std::clamp( int((Dec + k * std_IntraDec - ymin_Dec) / stepY_Dec), 0, nbPtsY_Dec -1);
        idxYErrMin = floor((Dec - k * std_IntraDec - ymin_Dec)/stepY_Dec + 0.5);
        if (idxYErrMin < 0) {
            idxYErrMin = 0;
        } else if (idxYErrMin > nbPtsY_Dec - 1) {
            idxYErrMin = nbPtsY_Dec - 1;
        }

        idxYErrMax = floor((Dec + k * std_IntraDec - ymin_Dec)/stepY_Dec + 0.5);
        if (idxYErrMax < 0) {
            idxYErrMax = 0;
        } else if (idxYErrMax > nbPtsY_Dec - 1) {
            idxYErrMax = nbPtsY_Dec - 1;
        }

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_Dec - 1) {

            ++curveMap_Dec(idx_t, idxYErrMin); // correction à faire dans finalize/nbIter ;

            curveMap_Dec.setMaxValue(std::max(curveMap_Dec.maxValue(), curveMap_Dec.at(idx_t, idxYErrMin)));

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_Dec) {
            double* ptr_Ymin = curveMap_Dec.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_Dec.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_Dec + ymin_Dec;
                double b = (idErr + 0.5) * stepY_Dec + ymin_Dec;
                double surfG = diff_erf(a, b, Dec, std_IntraDec );

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_Dec.setMaxValue(std::max(curveMap_Dec.maxValue(), *ptr_idErr));

                idErr++;
            }

        }

        //double idx_GP_Dec = std::clamp( int((dpDec - ymin_GP_Dec) / step_GP_Dec), 0, nbPtsY_GP_Dec - 1);
        double idx_GP_Dec = floor((dpDec - ymin_GP_Dec)/step_GP_Dec + 0.5);
        if (idx_GP_Dec < 0) {
            idx_GP_Dec = 0;
        } else if (idx_GP_Dec > nbPtsY_GP_Dec - 1) {
            idx_GP_Dec = nbPtsY_GP_Dec - 1;
        }
        ++curveMap_GP_Dec(idx_t, idx_GP_Dec);
        curveMap_GP_Dec.setMaxValue( std::max(curveMap_GP_Dec.maxValue(), curveMap_GP_Dec.at(idx_t, idx_GP_Dec)));



        // -- Calcul map on F

        // ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
             * https://en.wikipedia.org/wiki/Error_function
             */
        //idxYErrMin = std::clamp( int((F - k * std_IntraF - ymin_F) / stepY_F), 0, nbPtsY_F - 1);
        //idxYErrMax = std::clamp( int((F + k * std_IntraF - ymin_F) / stepY_F), 0, nbPtsY_F - 1);
        idxYErrMin = floor((F - k * std_IntraF - ymin_F)/stepY_F + 0.5);
        if (idxYErrMin < 0) {
            idxYErrMin = 0;
        } else if (idxYErrMin > nbPtsY_F - 1) {
            idxYErrMin = nbPtsY_F - 1;
        }

        idxYErrMax = floor((F + k * std_IntraF - ymin_F)/stepY_F + 0.5);
        if (idxYErrMax < 0) {
            idxYErrMax = 0;
        } else if (idxYErrMax > nbPtsY_F - 1) {
            idxYErrMax = nbPtsY_F - 1;
        }

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_F - 1) {

            ++curveMap_F(idx_t, idxYErrMin);

            curveMap_F.setMaxValue(std::max(curveMap_F.maxValue(), curveMap_F.at(idx_t, idxYErrMin)));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_F) {
            double* ptr_Ymin = curveMap_F.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_F.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_F + ymin_F;
                double b = (idErr + 0.5) * stepY_F + ymin_F;
                double surfG = diff_erf(a, b, F, std_IntraF );

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_F.setMaxValue(std::max(curveMap_F.maxValue(), *ptr_idErr));

                idErr++;
            }

        }
        //double idx_GP_F = std::clamp( int((dpF - ymin_GP_F) / step_GP_F), 0, nbPtsY_GP_F - 1);
        double idx_GP_F = floor((dpF - ymin_GP_F)/step_GP_F + 0.5);
        if (idx_GP_F < 0) {
            idx_GP_F = 0;
        } else if (idx_GP_F > nbPtsY_GP_F - 1) {
            idx_GP_F = nbPtsY_GP_F - 1;
        }
        ++curveMap_GP_F(idx_t, idx_GP_F);
        curveMap_GP_F.setMaxValue(std::max(curveMap_GP_F.maxValue(), curveMap_GP_F.at(idx_t, idx_GP_F)));

    }

    // référence sur la moyenne globale à dessiner
    std::vector<double> &vecG_Inc = postG.gx.vecG;
    //std::vector<double> &vecG_Dec = postG.gy.vecG;
    std::vector<double> &vecG_F = postG.gz.vecG;

    for (size_t tIdx = 0; tIdx < vecVarG_Inc.size(); ++tIdx) {

        double var_X = vecVarianceG_X.at(tIdx) / n + vecVarIntraG_X.at(tIdx);
        double var_Y = vecVarianceG_Y.at(tIdx) / n + vecVarIntraG_Y.at(tIdx);
        double var_Z = vecVarianceG_Z.at(tIdx) / n + vecVarIntraG_Z.at(tIdx);

        double var_F = (var_X + var_Y + var_Z) / 3.0; // Hypothèse : les variances sont indépendantes et identiquement distribuées.

        double F_mean = vecG_F.at(tIdx);
        double Inc_mean = vecG_Inc.at(tIdx) * rad ;

        double var_Inc = var_F / (F_mean * F_mean) ;
        double var_Dec = var_Inc / std::pow(cos(Inc_mean), 2) ;

        vecVarG_Inc[tIdx] = var_Inc * deg2;
        vecVarG_Dec[tIdx] = var_Dec * deg2;
        vecVarG_F[tIdx] = var_F;

    }

}

// old version, variances calculées dans l'espace I, D, F
void ModelCurve::memo_PosteriorG_IDF_old(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted)
{
    constexpr double rad = M_PI / 180.0;
    constexpr double deg = 180.0 * M_1_PI;
    //constexpr double deg2 = deg * deg;

    CurveMap& curveMap_Inc = postG.gx.mapG;
    CurveMap& curveMap_Dec = postG.gy.mapG;
    CurveMap& curveMap_F = postG.gz.mapG;

    const int nbPtsX = curveMap_Inc.column(); // identique à toutes les maps

    const int nbPtsY_Inc = curveMap_Inc.row();
    const int nbPtsY_Dec = curveMap_Dec.row();
    const int nbPtsY_F = curveMap_F.row();

    const double ymin_Inc = curveMap_Inc.minY();
    const double ymax_Inc = curveMap_Inc.maxY();

    const double ymin_Dec = curveMap_Dec.minY();
    const double ymax_Dec = curveMap_Dec.maxY();

    const double ymin_F = curveMap_F.minY();
    const double ymax_F = curveMap_F.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);

    const double stepY_Inc = (ymax_Inc - ymin_Inc) / (nbPtsY_Inc - 1);
    const double stepY_Dec = (ymax_Dec - ymin_Dec) / (nbPtsY_Dec - 1);
    const double stepY_F = (ymax_F - ymin_F) / (nbPtsY_F - 1);
    // variable for GP

    CurveMap& curveMap_GP_Inc = postG.gx.mapGP;
    CurveMap& curveMap_GP_Dec = postG.gy.mapGP;
    CurveMap& curveMap_GP_F = postG.gz.mapGP;

    // const int nbPts_GP_X = curveMap_GP_ZF->column(); // identique à toutes les maps

    const int nbPtsY_GP_Inc = curveMap_GP_Inc.row();
    const int nbPtsY_GP_Dec = curveMap_GP_Dec.row();
    const int nbPtsY_GP_F = curveMap_GP_F.row();

    const double ymin_GP_Inc = curveMap_GP_Inc.minY();
    const double ymax_GP_Inc = curveMap_GP_Inc.maxY();

    const double ymin_GP_Dec = curveMap_GP_Dec.minY();
    const double ymax_GP_Dec = curveMap_GP_Dec.maxY();

    const double ymin_GP_F = curveMap_GP_F.minY();
    const double ymax_GP_F = curveMap_GP_F.maxY();


    const double step_GP_Inc = (ymax_GP_Inc - ymin_GP_Inc) / (nbPtsY_GP_Inc - 1);
    const double step_GP_Dec = (ymax_GP_Dec - ymin_GP_Dec) / (nbPtsY_GP_Dec - 1);
    const double step_GP_F = (ymax_GP_F - ymin_GP_F) / (nbPtsY_GP_F - 1);

    // 2 - Variables temporaires
    /*
     * LEs calculs de variances ne se calculent que dans l'espace X, Y, Z
     * Il faut convertir juste au moment de la sauvegarde
     * */

    // référence sur la variance globale à dessiner
    std::vector<double> &vecVarG_Inc = postG.gx.vecVarG;
    std::vector<double> &vecVarG_Dec = postG.gy.vecVarG;
    std::vector<double> &vecVarG_F = postG.gz.vecVarG;

    // Variables temporaires
    // erreur inter spline
    std::vector<double> &vecVarianceG_Inc = postG.gx.vecVarianceG;
    std::vector<double> &vecVarianceG_Dec = postG.gy.vecVarianceG;
    std::vector<double> &vecVarianceG_F = postG.gz.vecVarianceG;
    // erreur intra spline
    std::vector<double> &vecVarIntraG_Inc = postG.gx.vecVarErrG;
    std::vector<double> &vecVarIntraG_Dec = postG.gy.vecVarErrG;
    std::vector<double> &vecVarIntraG_F = postG.gz.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG_Inc = postG.gx.vecG.begin();
    std::vector<double>::iterator itVecGP_Inc = postG.gx.vecGP.begin();
    std::vector<double>::iterator itVecGS_Inc = postG.gx.vecGS.begin();

    std::vector<double>::iterator itVecG_Dec = postG.gy.vecG.begin();
    std::vector<double>::iterator itVecGP_Dec = postG.gy.vecGP.begin();
    std::vector<double>::iterator itVecGS_Dec = postG.gy.vecGS.begin();

    std::vector<double>::iterator itVecG_F = postG.gz.vecG.begin();
    std::vector<double>::iterator itVecGP_F = postG.gz.vecGP.begin();
    std::vector<double>::iterator itVecGS_F = postG.gz.vecGS.begin();

    // Pointeur
    // erreur inter spline, dans l'espace I, D, F
    std::vector<double>::iterator itVecVarianceG_Inc = postG.gx.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_Dec = postG.gy.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_F = postG.gz.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarIntraG_Inc = postG.gx.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarIntraG_Dec = postG.gy.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarIntraG_F = postG.gz.vecVarErrG.begin();

    // inter derivate variance

    double t;
    double gx, gpx, gsx, varIntraGx = 0;
    double gy, gpy, gsy, varIntraGy = 0;
    double gz, gpz, gsz, varIntraGz = 0;

    double Inc, Dec, F;
    double dpInc, dpDec, dpF; // prime derivatives
    double dsInc, dsDec, dsF; // seconde derivatives


    double n = realyAccepted;

    constexpr double k = 3.0; // Le nombre de fois sigma G, pour le calcul de la densité

    int  idxYErrMin, idxYErrMax;


    // 3 - Calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idx_t = 0; idx_t < nbPtsX ; ++idx_t) {
        t = (double)idx_t * stepT + mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, spline.splineX, gx, varIntraGx, gpx, gsx, i0, mSettings.mTmin, mSettings.mTmax);
        valeurs_G_VarG_GP_GS(t, spline.splineY, gy, varIntraGy, gpy, gsy, i0, mSettings.mTmin, mSettings.mTmax);
        valeurs_G_VarG_GP_GS(t, spline.splineZ, gz, varIntraGz, gpz, gsz, i0, mSettings.mTmin, mSettings.mTmax);


        // -- Calcul Mean
        convertToIDF(gx, gy, gz, Inc, Dec, F);

        computeDerivatives(gx, gy, gz, gpx, gpy, gpz,
                           dpInc, dpDec, dpF);

        computeSecondDerivatives(gx, gy, gz, gpx, gpy, gpz, gsx, gsy, gsz,
                                 dsInc, dsDec, dsF);

        double prevMeanG_Inc= *itVecG_Inc;
        double prevMeanG_Dec = *itVecG_Dec;
        double prevMeanG_F = *itVecG_F;

        *itVecG_Inc +=  (Inc - prevMeanG_Inc)/n;
        *itVecGP_Inc +=  (dpInc - *itVecGP_Inc)/n;
        *itVecGS_Inc +=  (dsInc - *itVecGS_Inc)/n;

        *itVecG_Dec +=  (Dec - prevMeanG_Dec)/n;
        *itVecGP_Dec +=  (dpDec - *itVecGP_Dec)/n;
        *itVecGS_Dec +=  (dsDec - *itVecGS_Dec)/n;

        *itVecG_F +=  (F - prevMeanG_F)/n;

        *itVecGP_F +=  (dpF - *itVecGP_F)/n;
        *itVecGS_F +=  (dsF - *itVecGS_F)/n;


        // Variance inter spline
        double meanG_X, meanG_Y, meanG_Z;
        convertToXYZ(prevMeanG_Inc, prevMeanG_Dec, prevMeanG_F,
                     meanG_X, meanG_Y, meanG_Z );

        *itVecVarianceG_Inc +=  (Inc - prevMeanG_Inc)*(Inc - *itVecG_Inc);
        *itVecVarianceG_Dec +=  (Dec - prevMeanG_Dec)*(Dec - *itVecG_Dec);
        *itVecVarianceG_F +=  (F - prevMeanG_F)*(F - *itVecG_F);

        // Variance moyenne intra spline

        double std_IntraF = sqrt((varIntraGx + varIntraGy + varIntraGz)/3.0);

        double std_IntraInc = std_IntraF * deg / F ;
        double std_IntraDec = std_IntraF * deg / (F * cos(Inc * rad)) ;


        *itVecVarIntraG_Inc += (std_IntraInc*std_IntraInc - *itVecVarIntraG_Inc) / n  ;
        *itVecVarIntraG_Dec += (std_IntraDec*std_IntraDec - *itVecVarIntraG_Dec) / n  ;
        *itVecVarIntraG_F += (std_IntraF*std_IntraF - *itVecVarIntraG_F) / n  ;

        // increment pointeurs
        ++itVecG_Inc;
        ++itVecGP_Inc;
        ++itVecGS_Inc;
        ++itVecVarianceG_Inc;
        ++itVecVarIntraG_Inc;

        ++itVecG_Dec;
        ++itVecGP_Dec;
        ++itVecGS_Dec;
        ++itVecVarianceG_Dec;
        ++itVecVarIntraG_Dec;

        ++itVecG_F;
        ++itVecGP_F;
        ++itVecGS_F;
        ++itVecVarianceG_F;
        ++itVecVarIntraG_F;


        // -------- Calcul map on XInc ----------


        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        idxYErrMin = std::clamp( int((Inc - k * std_IntraInc - ymin_Inc) / stepY_Inc), 0, nbPtsY_Inc - 1);
        idxYErrMax = std::clamp( int((Inc + k * std_IntraInc - ymin_Inc) / stepY_Inc), 0, nbPtsY_Inc - 1);


        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_Inc - 1) {

            ++curveMap_Inc(idx_t, idxYErrMin); // correction à faire dans finalize/nbIter ;

            curveMap_Inc.setMaxValue(std::max(curveMap_Inc.maxValue(), curveMap_Inc.at(idx_t, idxYErrMin)));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_Inc) {
            double* ptr_Ymin = curveMap_Inc.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_Inc.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_Inc + ymin_Inc;
                double b = (idErr + 0.5) * stepY_Inc + ymin_Inc;
                double surfG = diff_erf(a, b, Inc, std_IntraInc );// correction à faire dans finalyze /nbIter;

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_Inc.setMaxValue(std::max(curveMap_Inc.maxValue(), *ptr_idErr));

                idErr++;
            }

        }
        double idx_GP_Inc = std::clamp( int((dpInc - ymin_GP_Inc) / step_GP_Inc), 0, nbPtsY_GP_Inc - 1);

        ++curveMap_GP_Inc(idx_t, idx_GP_Inc);
        curveMap_GP_Inc.setMaxValue(std::max(curveMap_GP_Inc.maxValue(), curveMap_GP_Inc.at(idx_t, idx_GP_Inc)));



        // -- Calcul map on YDec

        // Ajout densité erreur sur Y
        /* Il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
        * https://en.wikipedia.org/wiki/Error_function
        */
        idxYErrMin = std::clamp( int((Dec - k * std_IntraDec - ymin_Dec) / stepY_Dec), 0, nbPtsY_Dec -1);
        idxYErrMax = std::clamp( int((Dec + k * std_IntraDec - ymin_Dec) / stepY_Dec), 0, nbPtsY_Dec -1);


        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_Dec - 1) {

            ++curveMap_Dec(idx_t, idxYErrMin); // correction à faire dans finalize/nbIter ;

            curveMap_Dec.setMaxValue(std::max(curveMap_Dec.maxValue(), curveMap_Dec.at(idx_t, idxYErrMin)));

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_Dec) {
            double* ptr_Ymin = curveMap_Dec.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_Dec.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_Dec + ymin_Dec;
                double b = (idErr + 0.5) * stepY_Dec + ymin_Dec;
                double surfG = diff_erf(a, b, Dec, std_IntraDec );

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_Dec.setMaxValue(std::max(curveMap_Dec.maxValue(), *ptr_idErr));

                idErr++;
            }

        }

        double idx_GP_Dec = std::clamp( int((dpDec - ymin_GP_Dec) / step_GP_Dec), 0, nbPtsY_GP_Dec - 1);
        ++curveMap_GP_Dec(idx_t, idx_GP_Dec);
        curveMap_GP_Dec.setMaxValue( std::max(curveMap_GP_Dec.maxValue(), curveMap_GP_Dec.at(idx_t, idx_GP_Dec)));



        // -- Calcul map on F

        // ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
             * https://en.wikipedia.org/wiki/Error_function
             */
        idxYErrMin = std::clamp( int((F - k * std_IntraF - ymin_F) / stepY_F), 0, nbPtsY_F - 1);
        idxYErrMax = std::clamp( int((F + k * std_IntraF - ymin_F) / stepY_F), 0, nbPtsY_F - 1);


        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_F - 1) {

            ++curveMap_F(idx_t, idxYErrMin);

            curveMap_F.setMaxValue(std::max(curveMap_F.maxValue(), curveMap_F.at(idx_t, idxYErrMin)));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_F) {
            double* ptr_Ymin = curveMap_F.dataPtr(idx_t, idxYErrMin);
            double* ptr_Ymax = curveMap_F.dataPtr(idx_t, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_F + ymin_F;
                double b = (idErr + 0.5) * stepY_F + ymin_F;
                double surfG = diff_erf(a, b, F, std_IntraF );

                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap_F.setMaxValue(std::max(curveMap_F.maxValue(), *ptr_idErr));

                idErr++;
            }

        }
        double idx_GP_F = std::clamp( int((dpF - ymin_GP_F) / step_GP_F), 0, nbPtsY_GP_F - 1);
        ++curveMap_GP_F(idx_t, idx_GP_F);
        curveMap_GP_F.setMaxValue(std::max(curveMap_GP_F.maxValue(), curveMap_GP_F.at(idx_t, idx_GP_F)));


    }


    for (size_t tIdx = 0; tIdx < vecVarG_Inc.size(); ++tIdx) {

        double var_Inc = vecVarianceG_Inc.at(tIdx) / n + vecVarIntraG_Inc.at(tIdx);
        double var_Dec = vecVarianceG_Dec.at(tIdx) / n + vecVarIntraG_Dec.at(tIdx);
        double var_F = vecVarianceG_F.at(tIdx) / n + vecVarIntraG_F.at(tIdx);

        vecVarG_Inc[tIdx] = var_Inc;
        vecVarG_Dec[tIdx] = var_Dec;
        vecVarG_F[tIdx] = var_F;

    }

}
#endif

#pragma mark memo_PosteriorG_3D_335()
#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
void ModelCurve::memo_PosteriorG_3D_335(PosteriorMeanG &postG, const MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted)
{

    CurveMap& curveMap_XInc = postG.gx.mapG;
    CurveMap& curveMap_YDec = postG.gy.mapG;
    CurveMap& curveMap_ZF = postG.gz.mapG;

    const int nbPtsX = curveMap_XInc.column(); // identique à toutes les maps

    const int nbPtsY_XInc = curveMap_XInc.row();
    const int nbPtsY_YDec = curveMap_YDec.row();
    const int nbPtsY_ZF = curveMap_ZF.row();

    const double ymin_XInc = curveMap_XInc.minY();
    const double ymax_XInc = curveMap_XInc.maxY();

    const double ymin_YDec = curveMap_YDec.minY();
    const double ymax_YDec = curveMap_YDec.maxY();

    const double ymin_ZF = curveMap_ZF.minY();
    const double ymax_ZF = curveMap_ZF.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);

    const double stepY_XInc = (ymax_XInc - ymin_XInc) / (nbPtsY_XInc - 1);
    const double stepY_YDec = (ymax_YDec - ymin_YDec) / (nbPtsY_YDec - 1);
    const double stepY_ZF = (ymax_ZF - ymin_ZF) / (nbPtsY_ZF - 1);
    // variable for GP

    CurveMap& curveMap_GP_XInc = postG.gx.mapGP;
    CurveMap& curveMap_GP_YDec = postG.gy.mapGP;
    CurveMap& curveMap_GP_ZF = postG.gz.mapGP;

    // const int nbPts_GP_X = curveMap_GP_ZF->column(); // identique à toutes les maps

    const int nbPtsY_GP_XInc = curveMap_GP_XInc.row();
    const int nbPtsY_GP_YDec = curveMap_GP_YDec.row();
    const int nbPtsY_GP_ZF = curveMap_GP_ZF.row();

    const double ymin_GP_XInc = curveMap_GP_XInc.minY();
    const double ymax_GP_XInc = curveMap_GP_XInc.maxY();

    const double ymin_GP_YDec = curveMap_GP_YDec.minY();
    const double ymax_GP_YDec = curveMap_GP_YDec.maxY();

    const double ymin_GP_ZF = curveMap_GP_ZF.minY();
    const double ymax_GP_ZF = curveMap_GP_ZF.maxY();

    const double step_GP_XInc = (ymax_GP_XInc - ymin_GP_XInc) / (nbPtsY_GP_XInc - 1);
    const double step_GP_YDec = (ymax_GP_YDec - ymin_GP_YDec) / (nbPtsY_GP_YDec - 1);
    const double step_GP_ZF = (ymax_GP_ZF - ymin_GP_ZF) / (nbPtsY_GP_ZF - 1);

    // 2 - Variables temporaires

    //Pointeur sur tableau des moyennes
    //double* g = postG.gx.vecG.data();
    double* vecG_XInc = postG.gx.vecG.data();
    double* vecGP_XInc = postG.gx.vecGP.data();
    double* vecGS_XInc = postG.gx.vecGS.data();

    double* vecG_YDec = postG.gy.vecG.data();
    double* vecGP_YDec = postG.gy.vecGP.data();
    double* vecGS_YDec = postG.gy.vecGS.data();

    double* vecG_ZF = postG.gz.vecG.data();
    double* vecGP_ZF = postG.gz.vecGP.data();
    double* vecGS_ZF = postG.gz.vecGS.data();

    // référence sur variance

    double* vecVarG_XInc = postG.gx.vecVarG.data();
    double* vecVarG_YDec = postG.gy.vecVarG.data();
    double* vecVarG_ZF = postG.gz.vecVarG.data();

    double t;
    double gx, gpx, gsx;
    double gy, gpy, gsy;
    double gz, gpz, gsz;

    double n = realyAccepted;

    // 3 - Calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idx_t = 0; idx_t < nbPtsX ; ++idx_t) {
        t = static_cast<double>(idx_t) * stepT + mSettings.mTmin ;
        valeurs_G_GP_GS(t, spline.splineX, gx, gpx, gsx, i0, mSettings.mTmin, mSettings.mTmax);
        // i0 augmente jusqu'à trouver le bon temps dans valeur_G_GP_GS()
        // ici i0 doit correspondre exactement au bon t
        valeurs_G_GP_GS(t, spline.splineY, gy, gpy, gsy, i0, mSettings.mTmin, mSettings.mTmax);

        // Conversion IDF
        if (curveType == CurveSettings::eProcess_Vector ||  curveType == CurveSettings::eProcess_Spherical) {
            valeurs_G_GP_GS(t, spline.splineZ, gz, gpz, gsz, i0, mSettings.mTmin, mSettings.mTmax);

            double F, Inc, Dec;
            convertToIDF(gx, gy, gz, Inc, Dec, F);

            double dF, dInc, dDec;
            computeDerivatives(gx, gy, gz, gpx, gpy, gpz, dInc, dDec, dF);

            double d2Fdt2, d2Incdt2, d2Decdt2;
            computeSecondDerivatives(gx, gy, gz, gpx, gpy, gpz, gsx, gsy, gsz,
                                      d2Incdt2, d2Decdt2, d2Fdt2);
            gx = Inc;
            gy = Dec;
            gz = F;

            gpx = dInc;
            gpy = dDec;
            gpz = dF;


            gsx = d2Incdt2;
            gsy = d2Decdt2;
            gsz = d2Fdt2;


        } else if (compute_XYZ)
            valeurs_G_GP_GS(t, spline.splineZ, gz, gpz, gsz, i0, mSettings.mTmin, mSettings.mTmax);


        // -- Calcul Mean on XInc
        {

            vecGP_XInc[idx_t] +=  (gpx - vecGP_XInc[idx_t])/n;
            vecGS_XInc[idx_t] +=  (gsx - vecGS_XInc[idx_t])/n;

            // Version numériquement plus stable
            double delta = gx - vecG_XInc[idx_t]; // = (g - prevMeanG_ZF)

            vecG_XInc[idx_t] += delta / n;
            double delta2 = gx - vecG_XInc[idx_t];

            vecVarG_XInc[idx_t] = vecVarG_XInc[idx_t] * (n-1) + delta * delta2;

            // -- Calcul map on XInc ymin_XInc

            if (ymin_XInc <= gx && gx <= ymax_XInc) {
                int idx_Y = floor((gx - ymin_XInc)/stepY_XInc + 0.5);
                if (idx_Y < 0) {
                    idx_Y = 0;
                } else if (idx_Y > nbPtsY_XInc - 1) {
                    idx_Y = nbPtsY_XInc - 1;

                }

                ++curveMap_XInc(idx_t, idx_Y);
                curveMap_XInc.setMaxValue(std::max(curveMap_XInc.maxValue(), curveMap_XInc.at(idx_t, idx_Y)));
            }
            if (ymin_GP_XInc <= gpx && gpx <= ymax_GP_XInc) {
                int idx_YGP = floor((gpx - ymin_GP_XInc)/step_GP_XInc + 0.5);
                if (idx_YGP < 0) {
                    idx_YGP = 0;
                } else if (idx_YGP > nbPtsY_GP_XInc - 1) {
                    idx_YGP = nbPtsY_GP_XInc - 1;

                }
                ++curveMap_GP_XInc(idx_t, idx_YGP);
                curveMap_GP_XInc.setMaxValue(std::max(curveMap_GP_XInc.maxValue(), curveMap_GP_XInc.at(idx_t, idx_YGP)));
            }


            // -- Calcul Mean on YDec

            vecGP_YDec[idx_t] +=  (gpy - vecGP_YDec[idx_t])/n;
            vecGS_YDec[idx_t] +=  (gsy - vecGS_YDec[idx_t])/n;
            // erreur inter spline
            //*itVecVarianceG_YDec +=  (gy - prevMeanG_YDec)*(gy - *itVecG_YDec);

            // Version numériquement plus stable
            delta = gy - vecG_YDec[idx_t]; // = (g - prevMeanG_ZF)
            vecG_YDec[idx_t] += delta / n;
            delta2 = gy - vecG_YDec[idx_t];

            vecVarG_YDec[idx_t] = vecVarG_YDec[idx_t] * (n-1) + delta * delta2;

        }

        // -- Calcul map on YDec

        {
            if (ymin_YDec <= gy && gy <= ymax_YDec) {
                int idx_Y = floor((gy - ymin_YDec)/stepY_YDec + 0.5);
                if (idx_Y < 0) {
                    idx_Y = 0;
                } else if (idx_Y > nbPtsY_YDec - 1) {
                    idx_Y = nbPtsY_YDec - 1;

                }
                ++curveMap_YDec(idx_t, idx_Y);
                curveMap_YDec.setMaxValue(std::max(curveMap_YDec.maxValue(), curveMap_YDec.at(idx_t, idx_Y)));
            }
            if (ymin_GP_YDec <= gpy && gpy <= ymax_GP_YDec) {
                int idx_YGP = floor((gpy - ymin_GP_YDec)/step_GP_YDec + 0.5);
                if (idx_YGP < 0) {
                    idx_YGP = 0;
                } else if (idx_YGP > nbPtsY_GP_YDec - 1) {
                    idx_YGP = nbPtsY_GP_YDec - 1;

                }
                ++curveMap_GP_YDec(idx_t, idx_YGP);
                curveMap_GP_YDec.setMaxValue(std::max(curveMap_GP_YDec.maxValue(), curveMap_GP_YDec.at(idx_t, idx_YGP)));
            }

        }


        if (compute_XYZ) {

            // -- Calcul Mean on ZF
            vecGP_ZF[idx_t] +=  (gpz - vecGP_ZF[idx_t])/n;
            vecGS_ZF[idx_t] +=  (gsz - vecGS_ZF[idx_t])/n;

            // Version numériquement plus stable
            double delta = gz - vecG_ZF[idx_t]; // = (g - prevMeanG_ZF)
            vecG_ZF[idx_t] += delta / n;
            double delta2 = gz - vecG_ZF[idx_t];

            vecVarG_ZF[idx_t] = vecVarG_ZF[idx_t] * (n-1) + delta * delta2;

            // -- Calcul map on ZF

            if (ymin_ZF <= gz && gz <= ymax_ZF) {
                //const int idx_Y = std::clamp(int((gz - ymin_ZF) / stepY_ZF), 0, nbPtsY_ZF - 1);
                int idx_Y = floor((gz - ymin_ZF)/stepY_ZF + 0.5);
                if (idx_Y < 0) {
                    idx_Y = 0;
                } else if (idx_Y > nbPtsY_ZF - 1) {
                    idx_Y = nbPtsY_ZF - 1;

                }
                ++curveMap_ZF(idx_t, idx_Y);
                curveMap_ZF.setMaxValue(std::max(curveMap_ZF.maxValue(), curveMap_ZF.at(idx_t, idx_Y)));
            }
            if (ymin_GP_ZF <= gpz && gpz <= ymax_GP_ZF) {
                //const int idx_YGP = std::clamp(int((gpz - ymin_GP_ZF) / step_GP_ZF), 0, nbPtsY_GP_ZF - 1);
                int idx_YGP = floor((gpz - ymin_GP_ZF)/step_GP_ZF + 0.5);
                if (idx_YGP < 0) {
                    idx_YGP = 0;
                } else if (idx_YGP > nbPtsY_GP_ZF - 1) {
                    idx_YGP = nbPtsY_GP_ZF - 1;

                }
                ++curveMap_GP_ZF(idx_t, idx_YGP);
                curveMap_GP_ZF.setMaxValue(std::max(curveMap_GP_ZF.maxValue(), curveMap_GP_ZF.at(idx_t, idx_YGP)));
            }

        }


    }

    for (int i = 0; i < nbPtsX; ++i) {
        vecVarG_XInc[i] = vecVarG_XInc[i] / n;
        vecVarG_YDec[i] = vecVarG_YDec[i] / n;
        vecVarG_ZF[i] = vecVarG_ZF[i] / n;
    }

}

void ModelCurve::memo_PosteriorG(PosteriorMeanGComposante& postGCompo, const MCMCSplineComposante& splineComposante, const int realyAccepted)
{
    CurveMap& curveMap = postGCompo.mapG;
    const int nbPtsX = curveMap.column();
    const int nbPtsY = curveMap.row();

    const double ymin = curveMap.minY();
    const double ymax = curveMap.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    CurveMap& curveMapGP = postGCompo.mapGP;
    const double yminGP = curveMapGP.minY();
    const double ymaxGP = curveMapGP.maxY();
    const double stepYGP = (ymaxGP - yminGP) / (curveMapGP.row() - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double>& vecVarG = postGCompo.vecVarG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG = postGCompo.vecG.begin();
    std::vector<double>::iterator itVecGP = postGCompo.vecGP.begin();
    std::vector<double>::iterator itVecGS = postGCompo.vecGS.begin();

    std::vector<double>::iterator itVecVarG = vecVarG.begin();

    double t, g, gp, gs;
    g = 0.0;
    gp = 0.0;

    gs = 0.0;

    const double n = realyAccepted;

    // 3 - calcul pour la composante
    unsigned i0 = 0; // idx_t étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idx_X = 0; idx_X < nbPtsX ; ++idx_X) {
        t = idx_X * stepT + mSettings.mTmin ;

        valeurs_G_GP_GS(t, splineComposante, g, gp, gs, i0, mSettings.mTmin, mSettings.mTmax);

        *itVecGP +=  (gp - *itVecGP) / n;
        *itVecGS +=  (gs - *itVecGS) / n;
        //if (idx_t == 100)
          //  std::cout << " g =" << g;
        // Version numériquement plus stable
        double delta = g - *itVecG; // = (g - prevMeanG)
        *itVecG += delta / n;
        double delta2 = g - *itVecG;
        *itVecVarG = *itVecVarG * (n-1) + delta * delta2;

        ++itVecG;
        ++itVecGP;
        ++itVecGS;
        ++itVecVarG;


        // -- calcul map

        if (ymin <= g && g <= ymax) {
            //const int idx_Y = std::clamp(int((g - ymin) / stepY), 0, nbPtsY-1);
            int idx_Y = floor((g - ymin)/stepY + 0.5);
            if (idx_Y < 0) {
                idx_Y = 0;
            } else if (idx_Y > nbPtsY-1) {
                    idx_Y = nbPtsY-1;

            }
            ++curveMap(idx_X, idx_Y);
            curveMap.setMaxValue(std::max(curveMap.maxValue(), curveMap.at(idx_X, idx_Y)));
        }

        if (yminGP <= gp && gp <= ymaxGP) {
            //const int idx_YGP = std::clamp(int((gp - yminGP) / stepYGP), 0, nbPtsY-1);
            int idx_YGP = floor((gp - yminGP)/stepYGP + 0.5);
            if (idx_YGP < 0) {
                idx_YGP = 0;
            } else if (idx_YGP > nbPtsY - 1) {
                idx_YGP = nbPtsY-1;

            }
            ++curveMapGP(idx_X, idx_YGP);
            curveMapGP.setMaxValue(std::max(curveMapGP.maxValue(), curveMapGP.at(idx_X, idx_YGP)));
        }

    }

    std::transform(vecVarG.begin(), vecVarG.end(), vecVarG.begin(),
                   [n](double var) { return var / n; });
    //std::cout << " memo G Gmean =" << postGCompo.vecG.at(100) << " var = " << vecVarG.at(100) << std::endl;
}

#else
/**
 * @brief ModelCurve::memo_PosteriorG update the value of the mean and the variance with the new sampling. This is not a true memo(), but an update().
 * @param postGCompo
 * @param splineComposante the value of the last sampling
 * @param realyAccepted
 */
void ModelCurve::memo_PosteriorG(PosteriorMeanGComposante& postGCompo, const MCMCSplineComposante& splineComposante, const int realyAccepted)
{
    CurveMap& curveMap = postGCompo.mapG;
    const int nbPtsX = curveMap.column();
    const int nbPtsY = curveMap.row();

    const double ymin = curveMap.minY();
    const double ymax = curveMap.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    CurveMap& curveMapGP = postGCompo.mapGP;
    const double yminGP = curveMapGP.minY();
    const double ymaxGP = curveMapGP.maxY();
    const double stepYGP = (ymaxGP - yminGP) / (curveMapGP.row() - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double>& vecVarG = postGCompo.vecVarG; // Correspond à l'enveloppe d'erreur

    // Variables temporaires
    // erreur inter spline
    std::vector<double>& vecVarianceG = postGCompo.vecVarianceG; // tableau intermediare pour stocker le calcul de la variance
    // erreur intra spline
    std::vector<double>& vecVarErrG = postGCompo.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG = postGCompo.vecG.begin();
    std::vector<double>::iterator itVecGP = postGCompo.vecGP.begin();
    std::vector<double>::iterator itVecGS = postGCompo.vecGS.begin();

    //std::vector<double>::iterator itVecVarG = postGCompo.vecVarG.begin(); // pour test
    // Variables temporaires
    // erreur inter spline
    // tableau intermediare pour stocker le calcul de la variance, car il n'est pas possible de retrouver la valeur lors de n-1.
    // Il faudrait connaitre la valeur varG retournée lors de n-1
    std::vector<double>::iterator itVecVarianceG = postGCompo.vecVarianceG.begin();

    // erreur intra spline
    std::vector<double>::iterator itVecVarErrG = postGCompo.vecVarErrG.begin();

    // inter derivate variance
    //std::vector<double>::iterator itVecVarianceGP = postGCompo.vecVarGP.begin();

    double t, g, gp, gs, varG, stdG;
    g = 0.0;
    gp = 0.0;
    varG = 0.0;
    gs = 0.0;

    const double n = realyAccepted;

    constexpr double k = 3.0; // Le nombre de fois sigma G, pour le calcul de la densité
    //double a, b, surfG;

    int  idxYErrMin, idxYErrMax;

    // 3 - calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
        t = (double)idxT * stepT + mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0, mSettings.mTmin, mSettings.mTmax);

        // -- calcul Mean

        *itVecGP += (gp - *itVecGP)/n;
        *itVecGS += (gs - *itVecGS)/n;

        // erreur inter spline
        // Version numériquement plus stable
        double delta = g - *itVecG; // = (g - prevMeanG)

        *itVecG += delta / n;
        double delta2 = g - *itVecG;

        *itVecVarianceG += delta * delta2;

        // erreur intra spline
        *itVecVarErrG += (varG - *itVecVarErrG) / n  ;

        // inter derivate variance
        //*itVecVarianceGP +=  (gp - prevMeanGP)*(gp - *itVecGP);

        ++itVecG;
        ++itVecGP;
        ++itVecGS;
        ++itVecVarianceG;
        ++itVecVarErrG;


        // -- calcul map

        stdG = sqrt(varG);

        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        //idxYErrMin = std::clamp( int((g - k*stdG - ymin) / stepY), 0, nbPtsY-1);
        idxYErrMin = floor((g - k*stdG - ymin)/stepY + 0.5);
        if (idxYErrMin < 0) {
            idxYErrMin = 0;
        } else if (idxYErrMin > nbPtsY - 1) {
            idxYErrMin = nbPtsY-1;

        }
        //idxYErrMax = std::clamp( int((g + k*stdG - ymin) / stepY), 0, nbPtsY-1);
        idxYErrMax = floor((g + k*stdG - ymin)/stepY + 0.5);
        if (idxYErrMax < 0) {
            idxYErrMax = 0;
        } else if (idxYErrMax > nbPtsY - 1) {
            idxYErrMax = nbPtsY-1;
        }

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY-1) {

            ++curveMap(idxT, idxYErrMin); // correction à faire dans finalize/nbIter ;

            curveMap.max_value = std::max(curveMap.max_value, curveMap.at(idxT, idxYErrMin));

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY) {
            double* ptr_Ymin = curveMap.dataPtr(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap.dataPtr(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5)*stepY + ymin;
                double b = (idErr + 0.5)*stepY + ymin;
                double surfG = diff_erf(a, b, g, stdG );// correction à faire dans finalyze /nbIter;
                *ptr_idErr = (*ptr_idErr) + surfG;

                curveMap.max_value = std::max(curveMap.max_value, *ptr_idErr);

                idErr++;
            }
        }

        // Memo mapGP

        if (yminGP <= gp && gp <= ymaxGP) {
            //const int idxYGP = std::clamp( int((gp - yminGP) / stepYGP), 0, nbPtsY-1);
            int idxYGP = floor((gp - yminGP)/stepYGP + 0.5);
            if (idxYGP < 0) {
                idxYGP = 0;
            } else if (idxYGP > nbPtsY - 1) {
                idxYGP = nbPtsY-1;

            }
            ++curveMapGP(idxT, idxYGP);
            curveMapGP.max_value = std::max(curveMapGP.max_value, curveMapGP.at(idxT, idxYGP));
        }

    }
    int tIdx = 0;
    for (auto& vVarG : vecVarG) {
        vVarG = vecVarianceG.at(tIdx)/ n + vecVarErrG.at(tIdx);
        ++tIdx;
    }


}
#endif


void ModelCurve::memo_PosteriorG_filtering(PosteriorMeanGComposante &postGCompo, const MCMCSplineComposante &splineComposante, int &realyAccepted, const std::pair<double, double> GPfilter)
{
    const bool ok_accept = is_accepted_by_filter(splineComposante, GPfilter);

    if (!ok_accept) {
        realyAccepted = realyAccepted - 1;
        return;
    }
    memo_PosteriorG(postGCompo, splineComposante, realyAccepted);

    return;

}



/**
* @brief ModelCurve::is_accepted_by_filter
* @param splineComposante
* @param GPfilter, min and max value in Y unit per time unit ex meter/Year
* @return
 */
bool ModelCurve::is_accepted_by_filter(const MCMCSplineComposante& splineComposante, const std::pair<double, double> GPfilter)
{
    const double dYmin =  GPfilter.first * (mSettings.mTmax - mSettings.mTmin); // On doit passer en temps réduit
    const double dYmax =  GPfilter.second * (mSettings.mTmax - mSettings.mTmin);

    decltype(splineComposante.vecThetaReduced)::const_iterator iVecThetaRed = splineComposante.vecThetaReduced.cbegin();
    decltype(splineComposante.vecGamma)::const_iterator iVecGamma = splineComposante.vecGamma.cbegin();
    decltype(splineComposante.vecG)::const_iterator iVecG = splineComposante.vecG.cbegin();

    // Calcul dérivé avant les thetas
    const t_reduceTime t0 = splineComposante.vecThetaReduced[0];
    const t_reduceTime t1 = splineComposante.vecThetaReduced[1];
    double gPrime = (splineComposante.vecG[1] - splineComposante.vecG.at(0)) / (t1 - t0);
    gPrime -= (t1 - t0) * splineComposante.vecGamma[1] / 6.;

    if (gPrime < dYmin || dYmax < gPrime)
        return false;

    // Calcul dérivé avant les thetas

    const size_t n = splineComposante.vecThetaReduced.size();
    const t_reduceTime tn = splineComposante.vecThetaReduced[n-1];
    const t_reduceTime tn_1 = splineComposante.vecThetaReduced[n-2];
    gPrime = (splineComposante.vecG.at(n-1) - splineComposante.vecG[n-2]) / (tn - tn_1);
    gPrime += (tn - tn_1) * splineComposante.vecGamma[n-2] / 6.0;

    if (gPrime < dYmin || dYmax < gPrime)
        return false;

    //
    bool sup_min_OK = true, inf_max_OK = true; // Il suffit d'une valeur fausse pour annuler la courbe

    for (unsigned long i= 0; i< splineComposante.vecThetaReduced.size()-1; i++) {

        const t_reduceTime t_i = *iVecThetaRed;
        ++iVecThetaRed;
        const t_reduceTime t_i1 = *iVecThetaRed;

        const t_reduceTime hi = t_i1 - t_i;

        const double gamma_i = *iVecGamma;
        ++iVecGamma;
        const double gamma_i1 = *iVecGamma;

        const double g_i = *iVecG;
        ++iVecG;
        const double g_i1 = *iVecG;

        const double a = (g_i1 - g_i) /hi;
        const double b = (gamma_i1 - gamma_i) /(6.0 * hi);
        const double s = t_i + t_i1;
        const double p = t_i * t_i1;
        const double d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i) * gamma_i ) / (6.0 * hi);

        // résolution équation, la dérivée est du deuxième degrée

        const double aDelta = 3* b;
        const double bDelta = 2*d - 2*s*b;
        const double cDelta = p*b - s*d + a;

        double delta_min = bDelta * bDelta - 4*aDelta*(cDelta - dYmin); // on cherche les solutions ax^2 + bx + c - dmin > 0


        if (delta_min <= 0) {
            if (aDelta < 0) { // concave // j'inverse ici !!!
                //sup_min_OK = false;
                return false;

            } else           // convexe
                sup_min_OK = true; // c'est toujours true

        } else {

            double t1_res = (-bDelta - sqrt(delta_min)) / (2.*aDelta);
            double t2_res = (-bDelta + sqrt(delta_min)) / (2.*aDelta);

            if (t1_res > t2_res)
                std::swap(t1_res, t2_res);

            if (aDelta > 0) { //C'est un minimum entre les solutions
                if (t_i1<t1_res || t2_res<=t_i ) {
                        sup_min_OK = true;

                    } else {
                        //sup_min_OK = false;
                        return false;
                    }


            } else { //C'est un maximum entre les solutions
                if ( t1_res <= t_i && t_i1 <= t2_res ) {
                    sup_min_OK = true;

                } else {
                    //sup_min_OK = false;
                    return false;
                }
            }

        }
        // controle valeur max
        double delta_max = bDelta * bDelta - 4*aDelta*(cDelta - dYmax); // On cherche les solutions ax^2 + bx + c - dmax < 0


        if (delta_max <= 0) {
            if (aDelta < 0) {// concave
                inf_max_OK = true;

            } else {          // convexe
                //inf_max_OK = false;
                return false;
            }

        } else {

            double t1_res = (-bDelta - sqrt(delta_max)) / (2.*aDelta);
            double t2_res = (-bDelta + sqrt(delta_max)) / (2.*aDelta);

            if (t1_res > t2_res)
                std::swap(t1_res, t2_res);

            if (aDelta > 0) { //C'est un minimum entre les solutions
                if (t1_res<=t_i && t_i1<t2_res) {
                    inf_max_OK = true;

                } else {
                    //inf_max_OK = false;
                    return false;
                }

            } else { //C'est un maximum entre les solutions
                if ( t_i1< t1_res ||  t2_res< t_i ) {
                    inf_max_OK = true;

                } else {
                    //inf_max_OK = false;
                    return false;
                }

            }
        }



    }

    return (sup_min_OK && inf_max_OK);
}


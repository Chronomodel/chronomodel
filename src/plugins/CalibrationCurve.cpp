#include "CalibrationCurve.h"
#include "PluginManager.h"

CalibrationCurve::CalibrationCurve():
mName(QString("unkown")),
mDescription(QString("undefined"))
{
    mPluginId = "";
    mPlugin = nullptr;
    mRepartition = QVector< double>();
    mCurve = QVector< double>();
    mTmin = -INFINITY;
    mTmax = +INFINITY;
    mStep = 1.;
}
CalibrationCurve::CalibrationCurve(const CalibrationCurve& other)
{
    mName = other.mName;
    mPluginId = other.mPluginId;
    mPlugin = other.mPlugin;

    mDescription = other.mDescription;
    mRepartition.resize(other.mRepartition.size());
    std::copy(other.mRepartition.begin(), other.mRepartition.end(), mRepartition.begin());
    mCurve .resize(other.mCurve.size());
    std::copy(other.mCurve.begin(),other.mCurve.end(), mCurve.begin());
    mTmin = other.mTmin;
    mTmax = other.mTmax;
    mStep = other.mStep;

}
CalibrationCurve::~CalibrationCurve() noexcept
{
    mRepartition.clear();
    mCurve.clear();
    mPluginId.clear();
    mPlugin = nullptr;
}


QDataStream &operator << (QDataStream &stream, const CalibrationCurve &data)
{
    stream << data.mName;
    stream << data.mDescription;
    stream << data.mRepartition;
    stream << data.mCurve;
    stream << data.mTmin;
    stream << data.mTmax;
    stream << data.mStep;
    stream << data.mPluginId;

    return stream;

}

QDataStream &operator >> (QDataStream &stream, CalibrationCurve &data)
{
    stream >> data.mName;
    stream >> data.mDescription;
    stream >> data.mRepartition;
    stream >> data.mCurve;
    stream >> data.mTmin;
    stream >> data.mTmax;
    stream >> data.mStep;
    stream >> data.mPluginId;

    data.mPlugin = PluginManager::getPluginFromId(data.mPluginId);
    if (data.mPlugin == 0)
        throw QObject::tr("Calibration plugin could not be loaded : invalid plugin : ") + data.mPluginId;

    return stream;

}


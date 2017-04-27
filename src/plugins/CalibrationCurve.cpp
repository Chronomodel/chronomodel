#include "CalibrationCurve.h"
#include "PluginManager.h"

CalibrationCurve::CalibrationCurve():
mName(QString("unkown")),
mDescription(QString("undefined")),
mMethod(CalibrationCurve::Method::eFromRef)
{
    // Parameter refere to the Method
    mMCMCSetting = MCMCSettings();
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
    mMCMCSetting =other.mMCMCSetting;
    mPluginId = other.mPluginId;
    mPlugin = other.mPlugin;

    mDescription = other.mDescription;
    mMethod = other.mMethod;
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


QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data )
{
    stream << data.mName;
    stream << data.mDescription;

    switch (data.mMethod) {
       case CalibrationCurve::eFromRef : stream << (quint8)(0);
        break;
       case CalibrationCurve::eFromMCMC : stream << (quint8)(1);
          break;
    }

    stream << data.mRepartition;
    stream << data.mCurve;
    stream << data.mTmin;
    stream << data.mTmax;
    stream << data.mStep;
    stream << data.mMCMCSetting;
    stream << data.mPluginId;

    return stream;

}

QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data )
{
    stream >> data.mName;
    stream >> data.mDescription;

    quint8 tmp8;
    stream >> tmp8;
    switch ((int) tmp8) {
      case 0 : data.mMethod = CalibrationCurve::eFromRef;
       break;
      case 1 : data.mMethod = CalibrationCurve::eFromMCMC;
         break;
    }

    stream >> data.mRepartition;
    stream >> data.mCurve;
    stream >> data.mTmin;
    stream >> data.mTmax;
    stream >> data.mStep;
    stream >> data.mMCMCSetting;

    stream >> data.mPluginId;

    data.mPlugin = PluginManager::getPluginFromId(data.mPluginId);
    if (data.mPlugin == nullptr)
        throw QObject::tr("Calibration plugin could not be loaded : invalid plugin : ") + data.mPluginId;

    return stream;

}


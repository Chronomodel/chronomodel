#ifndef PluginAMSETTINGSVIEW_H
#define PluginAMSETTINGSVIEW_H

#if USE_PLUGIN_AM

#include "../PluginSettingsViewAbstract.h"
#include <QMap>

class PluginAM;
class PluginRefCurveSettingsView;
class QLabel;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;


class PluginAMSettingsView: public PluginSettingsViewAbstract
{
    Q_OBJECT
public:
    PluginAMSettingsView(PluginAM* plugin, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PluginAMSettingsView();
    
private slots:
    void updateSettings();
    
private:
    PluginRefCurveSettingsView* mRefView;
    
    QLabel* mNumChainsLab;
    QLabel* mBurnIterationsLab;
    QLabel* mBatchIterationsLab;
    QLabel* mNumBatchsLab;
    QLabel* mRunIterationsLab;
    QLabel* mThinningLab;
    QLabel* mSeedsLab;
    QLabel* mMixingLab;
    
    QSpinBox* mNumChainsSpin;
    QSpinBox* mBurnIterationsSpin;
    QSpinBox* mBatchIterationsSpin;
    QSpinBox* mNumBatchsSpin;
    QSpinBox* mRunIterationsSpin;
    QSpinBox* mThinningSpin;
    QLineEdit* mSeedsEdit;
    QDoubleSpinBox* mMixingSpin;
};

#endif
#endif


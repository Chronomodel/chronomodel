#include "PluginAMSettingsView.h"
#if USE_PLUGIN_AM

#include "PluginAM.h"
#include "PluginRefCurveSettingsView.h"
#include "QtUtilities.h"
#include <QtWidgets>


PluginAMSettingsView::PluginAMSettingsView(PluginAM* plugin, QWidget* parent, Qt::WindowFlags flags):PluginSettingsViewAbstract(plugin, parent, flags)
{
    mRefView = new PluginRefCurveSettingsView(plugin);
    
    mNumChainsLab = new QLabel(tr("Num chains"), this);
    mNumChainsSpin = new QSpinBox(this);
    mNumChainsSpin->setRange(0, 100);
    mNumChainsSpin->setSingleStep(1);
    
    mBurnIterationsLab = new QLabel(tr("Burn iterations"), this);
    mBurnIterationsSpin = new QSpinBox(this);
    mBurnIterationsSpin->setRange(10, 10000000);
    mBurnIterationsSpin->setSingleStep(1);
    
    mNumBatchsLab = new QLabel(tr("Number of batchs"), this);
    mNumBatchsSpin = new QSpinBox(this);
    mNumBatchsSpin->setRange(1, 10000000);
    mNumBatchsSpin->setSingleStep(1);
    
    mBatchIterationsLab = new QLabel(tr("Iterations per batch"), this);
    mBatchIterationsSpin = new QSpinBox(this);
    mBatchIterationsSpin->setRange(10, 10000000);
    mBatchIterationsSpin->setSingleStep(1);
    
    mRunIterationsLab = new QLabel(tr("Run iterations"), this);
    mRunIterationsSpin = new QSpinBox(this);
    mRunIterationsSpin->setRange(10, 10000000);
    mRunIterationsSpin->setSingleStep(1);
    
    mThinningLab = new QLabel(tr("Thinning interval"), this);
    mThinningSpin = new QSpinBox(this);
    mThinningSpin->setRange(0, 100000);
    mThinningSpin->setSingleStep(1);
    
    mMixingLab = new QLabel(tr("Mixing level"), this);
    mMixingSpin = new QDoubleSpinBox(this);
    mMixingSpin->setSingleStep(0.01);
    mMixingSpin->setRange(0, 1);
    
    mSeedsLab = new QLabel(tr("Seeds"), this);
    mSeedsEdit = new QLineEdit(this);
    
    QGridLayout* gridLayout = new QGridLayout();
    int row = 0;
    
    gridLayout->addWidget(mNumChainsLab, row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mNumChainsSpin, row, 1);
    
    gridLayout->addWidget(mBurnIterationsLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mBurnIterationsSpin, row, 1);
    
    gridLayout->addWidget(mNumBatchsLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mNumBatchsSpin, row, 1);
    
    gridLayout->addWidget(mBatchIterationsLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mBatchIterationsSpin, row, 1);
    
    gridLayout->addWidget(mRunIterationsLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mRunIterationsSpin, row, 1);
    
    gridLayout->addWidget(mThinningLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mThinningSpin, row, 1);
    
    gridLayout->addWidget(mMixingLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mMixingSpin, row, 1);
    
    gridLayout->addWidget(mSeedsLab, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(mSeedsEdit, row, 1);
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(gridLayout);
    layout->addWidget(mRefView);
    setLayout(layout);
    
    
    mNumChainsSpin->setValue(plugin->mMCMCSettings.mNumChains);
    mBurnIterationsSpin->setValue(plugin->mMCMCSettings.mNumBurnIter);
    mNumBatchsSpin->setValue(plugin->mMCMCSettings.mMaxBatches);
    mBatchIterationsSpin->setValue(plugin->mMCMCSettings.mNumBatchIter);
    mRunIterationsSpin->setValue(plugin->mMCMCSettings.mNumRunIter);
    mThinningSpin->setValue(plugin->mMCMCSettings.mThinningInterval);
    mMixingSpin->setValue(plugin->mMCMCSettings.mMixingLevel);
    mSeedsEdit->setText(intListToString(plugin->mMCMCSettings.mSeeds, ";"));
    
    connect(mNumChainsSpin, SIGNAL(valueChanged(int)), this, SLOT(updateSettings()));
    connect(mBurnIterationsSpin, SIGNAL(valueChanged(int)), this, SLOT(updateSettings()));
    connect(mNumBatchsSpin, SIGNAL(valueChanged(int)), this, SLOT(updateSettings()));
    connect(mBatchIterationsSpin, SIGNAL(valueChanged(int)), this, SLOT(updateSettings()));
    connect(mRunIterationsSpin, SIGNAL(valueChanged(int)), this, SLOT(updateSettings()));
    connect(mThinningSpin, SIGNAL(valueChanged(int)), this, SLOT(updateSettings()));
    connect(mMixingSpin, SIGNAL(valueChanged(double)), this, SLOT(updateSettings()));
    connect(mSeedsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateSettings()));
}

PluginAMSettingsView::~PluginAMSettingsView()
{
    
}

void PluginAMSettingsView::updateSettings()
{
    PluginAM* plugin = (PluginAM*) mPlugin;
    
    plugin->mMCMCSettings.mNumChains = mNumChainsSpin->value();
    plugin->mMCMCSettings.mNumBurnIter = mBurnIterationsSpin->value();
    plugin->mMCMCSettings.mMaxBatches = mNumBatchsSpin->value();
    plugin->mMCMCSettings.mNumBatchIter = mBatchIterationsSpin->value();
    plugin->mMCMCSettings.mNumRunIter = mRunIterationsSpin->value();
    plugin->mMCMCSettings.mThinningInterval = mThinningSpin->value();
    plugin->mMCMCSettings.mMixingLevel = mMixingSpin->value();
    plugin->mMCMCSettings.mSeeds = stringListToIntList(mSeedsEdit->text(), ";");
}

#endif

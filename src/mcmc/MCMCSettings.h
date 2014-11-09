#ifndef MCMCSettings_H
#define MCMCSettings_H

#include <QJsonObject>
#include <QList>


class MCMCSettings
{
public:
    MCMCSettings();
    MCMCSettings(const MCMCSettings& s);
    MCMCSettings& operator=(const MCMCSettings& s);
    void copyFrom(const MCMCSettings& s);
    ~MCMCSettings();
    
    static MCMCSettings fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
    
    unsigned long long mNumChains;
    unsigned long long mNumRunIter;
    unsigned long long mNumBurnIter;
    unsigned long long mMaxBatches;
    unsigned long long mIterPerBatch;
    unsigned int mThinningInterval;
    QList<int> mSeeds;
    
    unsigned long mFinalBatchIndex;
};

#endif

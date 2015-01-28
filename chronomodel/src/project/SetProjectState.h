#ifndef SetProjectState_H
#define SetProjectState_H

#include <QUndoCommand>
#include <QJsonObject>
#include <QString>

class Project;


class SetProjectState: public QUndoCommand
{
public:
    SetProjectState(Project* project,
                    const QJsonObject& prevState,
                    const QJsonObject& nextState,
                    const QString& reason,
                    bool notify);
    
    virtual void undo();
    virtual void redo();
    
private:
    Project* mProject;
    QJsonObject mPrevState;
    QJsonObject mNextState;
    QString mReason;
    bool mNotify;
};


#endif
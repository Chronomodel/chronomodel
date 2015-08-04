#include "SetProjectState.h"
#include "Project.h"

SetProjectState::SetProjectState(Project* project,
                                 const QJsonObject& prevState,
                                 const QJsonObject& nextState,
                                 const QString& reason,
                                 bool notify):
mProject(project),
mPrevState(prevState),
mNextState(nextState),
mReason(reason),
mNotify(notify)
{
    // Activate this to show undo/redo actions title in the UndoStack list view
    // However, this will also display action names in the main toolbar undo/redo buttons,
    // Resizing them every time... (actually, this is the only reason to disable the line below!)
    setText(mReason);
}

void SetProjectState::undo()
{
    mProject->sendUpdateState(mPrevState, mReason, mNotify);
}

void SetProjectState::redo()
{
    mProject->sendUpdateState(mNextState, mReason, mNotify);
}

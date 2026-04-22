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

#include "SetProjectState.h"
#include "Project.h"

SetProjectState::SetProjectState(Project* project,
                                 const QJsonObject& prevState,
                                 const QJsonObject& nextState,
                                 const Project::ReasonId id):
    mProject(project),
    mPrevState(prevState),
    mNextState(nextState),
    mReason(id)
{
    // Activate this to show undo/redo actions title in the UndoStack list view in MainWindows::mUndoDock
    // However, this will also display action names in the main toolbar undo/redo buttons,
    // Resizing them every time... (actually, this is the only reason to disable the line below!)
    // Le texte affiché dans la pile d'undo/redo
    if (mProject) {
        setText(Project::reasonToString(mReason));
    } else {
        setText(QStringLiteral("Invalid command"));
    }
}


void SetProjectState::undo()
{
    if (!mProject) {
        qWarning() << "[SetProjectState::undo] Project no longer exists – command ignored";
        return;
    }
 //   qDebug() << "[SetProjectState::undo] reason = " << Project::reasonToString(mReason);
    mProject->mState = mPrevState;

    emit mProject->projectStateChanged();
    emit mProject->currentEventChanged();

}


// come from Project::pushProjectState() with the command MainWindow::getInstance()->getUndoStack()->push(command);
void SetProjectState::redo()
{
    if (!mProject) {
        qWarning() << "[SetProjectState::redo] Project no longer exists – command ignored";
        return;
    }
  //  qDebug() << "[SetProjectState::redo] reason = " << Project::reasonToString(mReason);
    mProject->mState = mNextState;
    emit mProject->currentEventChanged();

    emit mProject->projectStateChanged();

}

/*
bool SetProjectState::mergeWith(const QUndoCommand *other)
{
    // On ne fusionne que les commandes du même type et du même projet
    const SetProjectState *o = static_cast<const SetProjectState*>(other);
    if (o->mProject != mProject)
        return false;
    // On garde le premier état (mPrevState) et on remplace le second état
    mNextState = o->mNextState;
    mReason    = o->mReason;               // on met à jour le texte affiché
    if (mProject)
        setText(Project::reasonToString(mReason));
    return true;   // la commande précédente devient obsolète
}
*/
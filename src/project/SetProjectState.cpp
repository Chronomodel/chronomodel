/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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
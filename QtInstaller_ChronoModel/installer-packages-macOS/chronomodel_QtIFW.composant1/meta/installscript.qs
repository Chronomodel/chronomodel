/**************************************************************************
**
** ChronoModel - installscript.qs
** version du 2026-09-25
** Sur Windows comme sur macOS, le contenu de "data" (chronomodel.exe + DLL,
** ou chronomodel.app) est copie tel quel par le comportement par defaut de
** QtIFW : rien de particulier a faire ici pour le deploiement lui-meme.
**
**************************************************************************/

function Component()
{
}

Component.prototype.createOperations = function()
{
    // Operations standard : copie du contenu de "data" vers TargetDir,
    // creation du desinstalleur, du Menu Demarrer sur Windows, etc.
    component.createOperations();

    if (installer.value("os") === "win") {
        // Raccourci dans le Menu Demarrer.
        component.addOperation("CreateShortcut",
            "@TargetDir@/chronomodel.exe", "@StartMenuDir@/ChronoModel.lnk");
    }
}


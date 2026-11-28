#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-unix.h>
#include "indicator.h"

static gboolean onQuit (gpointer pData)
{
    GMainLoop *pLoop = (GMainLoop*) pData;
    g_main_loop_quit (pLoop);

    return G_SOURCE_REMOVE;
}

int main (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    GApplication *pApplication = g_application_new ("org.ayatana.appindicator.display", G_APPLICATION_DEFAULT_FLAGS);
    GError *pError = NULL;
    gboolean bRegistered = g_application_register (pApplication, NULL, &pError);

    if (!bRegistered)
    {
        gchar *sError = "Unknown error";

        if (pError != NULL)
        {
            sError = pError->message;
        }

        g_warning ("Unable to register application: %s", sError);
        g_clear_error (&pError);
        g_object_unref (pApplication);

        return 1;
    }

    gboolean bRemote = g_application_get_is_remote (pApplication);

    if (bRemote)
    {
        return 0;
    }

    setlocale(LC_ALL, "");
    setlocale (LC_NUMERIC, "POSIX");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    textdomain (GETTEXT_PACKAGE);
    IndicatorDisplay *pService = indicator_display_new ();
    GMainLoop *pLoop = g_main_loop_new (NULL, FALSE);
    g_unix_signal_add (SIGINT, onQuit, pLoop);
    g_unix_signal_add (SIGTERM, onQuit, pLoop);
    g_main_loop_run (pLoop);
    g_clear_object (&pService);
    g_main_loop_unref (pLoop);
    g_object_unref (pApplication);

    return 0;
}

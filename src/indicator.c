#include <glib/gi18n.h>
#include <geoclue.h>
#include <ayatana/common/utils.h>
#include "indicator.h"
#include "elevation.h"

G_DEFINE_TYPE_WITH_PRIVATE (IndicatorDisplay, indicator_display, G_TYPE_OBJECT)

typedef struct
{
    guint nTempLow;
    guint nTempHigh;
    const gchar *sName;
} TempProfile;

TempProfile m_lTempProfiles[] =
{
    {0, 0, N_("Manual")},
    {4500, 6500, N_("Adaptive (Colder)")},
    {3627, 4913, N_("Adaptive")},
    {3058, 4913, N_("Adaptive (Warmer)")},
    {0, 0, NULL}
};

static gboolean updateColor (gpointer pData)
{
    IndicatorDisplay *self = INDICATOR_DISPLAY (pData);
    guint nProfile = 0;
    gdouble fBrightness = 0.0;
    g_settings_get (self->pPrivate->pSettings, "color-temp-profile", "q", &nProfile);
    fBrightness = g_settings_get_double (self->pPrivate->pSettings, "brightness");
    gchar *sThemeProfile = NULL;
    gboolean bThemeAdaptive = FALSE;
    sThemeProfile = g_settings_get_string (self->pPrivate->pSettings, "theme-profile");
    bThemeAdaptive = g_str_equal (sThemeProfile, "adaptive");

    guint nTemperature = 0;
    const gchar *sColorScheme = NULL;
    gint64 nNow = g_get_real_time ();
    gdouble fElevation = solarElevation ((gdouble) nNow / 1000000.0, self->pPrivate->fLatitude, self->pPrivate->fLongitude);

    if (nProfile == 0)
    {
        g_settings_get (self->pPrivate->pSettings, "color-temp", "q", &nTemperature);
    }
    else
    {
        gdouble fShifting = 0.0;

        if (fElevation < -6.0)
        {
            fShifting = 1.0;
        }
        else if (fElevation < 3.0)
        {
            fShifting = 1.0 - ((-6.0 - fElevation) / -9.0);
        }

        nTemperature = m_lTempProfiles[nProfile].nTempHigh - (m_lTempProfiles[nProfile].nTempHigh - m_lTempProfiles[nProfile].nTempLow) * fShifting;
        self->pPrivate->bAutoSliderUpdate = TRUE;
    }

    if (!bThemeAdaptive)
    {
        gboolean bLightTheme = g_str_equal (sThemeProfile, "light");

        if (bLightTheme)
        {
            sColorScheme = "prefer-light";
        }
        else
        {
            sColorScheme = "prefer-dark";
        }
    }
    else
    {
        if (fElevation < -6.0)
        {
            sColorScheme = "prefer-dark";
        }
        else
        {
            sColorScheme = "prefer-light";
        }
    }

    if (self->pPrivate->fLastBrightness != fBrightness || self->pPrivate->nLastColorTemp != nTemperature)
    {
        g_debug ("Calling xsct with %u %f", nTemperature, fBrightness);

        GAction *pAction = g_action_map_lookup_action (G_ACTION_MAP (self->pPrivate->pActionGroup), "color-temp");
        GVariant *pTemperature = g_variant_new_double (nTemperature);
        g_action_change_state (pAction, pTemperature);

        GError *pError = NULL;
        gchar *sCommand = g_strdup_printf ("xsct %u %f", nTemperature, fBrightness);
        gboolean bSuccess = g_spawn_command_line_sync (sCommand, NULL, NULL, NULL, &pError);

        if (!bSuccess)
        {
            g_warning ("Panic: The call to '%s' failed: %s", sCommand, pError->message);
            g_error_free (pError);
        }
        else
        {
            self->pPrivate->fLastBrightness = fBrightness;
            self->pPrivate->nLastColorTemp = nTemperature;
        }

        g_free (sCommand);
    }

    gboolean bSameColorScheme = g_str_equal (sColorScheme, self->pPrivate->sLastColorScheme);

    if (self->pPrivate->pColorSchemeSettings && !bSameColorScheme)
    {
        g_debug ("Changing color scheme to %s", sColorScheme);

        g_settings_set_string (self->pPrivate->pColorSchemeSettings, "color-scheme", sColorScheme);
        self->pPrivate->sLastColorScheme = sColorScheme;
    }

    return G_SOURCE_CONTINUE;
}

static void onGeoClueLoaded (GObject *pObject, GAsyncResult *pResult, gpointer pData)
{
    IndicatorDisplay *self = INDICATOR_DISPLAY (pData);
    GError *pError = NULL;
    GClueSimple *pSimple = gclue_simple_new_finish (pResult, &pError);

    if (pError != NULL)
    {
        g_warning ("Failed to connect to GeoClue2 service: %s", pError->message);
    }
    else
    {
        GClueLocation *pLocation = gclue_simple_get_location (pSimple);
        self->pPrivate->fLatitude = gclue_location_get_latitude (pLocation);
        self->pPrivate->fLongitude = gclue_location_get_longitude (pLocation);
        g_settings_set_double (self->pPrivate->pSettings, "latitude", self->pPrivate->fLatitude);
        g_settings_set_double (self->pPrivate->pSettings, "longitude", self->pPrivate->fLongitude);
    }

    updateColor (self);
}

static void onColorTempSettings (GSettings *pSettings, const gchar *sKey, gpointer pData)
{
    GVariant *pProfile = g_variant_new_uint16 (0);
    g_settings_set_value (pSettings, "color-temp-profile", pProfile);
    updateColor (pData);
}

static gboolean settingsIntToActionStateString (GValue *pValue, GVariant *pVariant, gpointer pData)
{
    guint16 nVariant = g_variant_get_uint16 (pVariant);
    gchar *sVariant = g_strdup_printf ("%u", nVariant);
    GVariant *pVariantString = g_variant_new_string (sVariant);
    g_free (sVariant);
    g_value_set_variant (pValue, pVariantString);

    return TRUE;
}

static GVariant* actionStateStringToSettingsInt (const GValue *pValue, const GVariantType *pVariantType, gpointer pData)
{
    GVariant *pVariantString = g_value_get_variant (pValue);
    const gchar *sValue = g_variant_get_string (pVariantString, NULL);
    guint16 nValue = (guint16) g_ascii_strtoull (sValue, NULL, 10);
    GVariant *pVariantInt = g_variant_new_uint16 (nValue);
    GValue cValue = G_VALUE_INIT;
    g_value_init (&cValue, G_TYPE_VARIANT);
    g_value_set_variant (&cValue, pVariantInt);

    return g_value_dup_variant (&cValue);
}

static void onColorTempState (GSimpleAction *pAction, GVariant *pVariant, gpointer pData)
{
    g_simple_action_set_state (pAction, pVariant);
    IndicatorDisplay *self = INDICATOR_DISPLAY (pData);

    if (self->pPrivate->bAutoSliderUpdate)
    {
        self->pPrivate->bAutoSliderUpdate = FALSE;

        return;
    }

    GVariant *pProfile = g_variant_new_uint16 (0);
    g_settings_set_value (self->pPrivate->pSettings, "color-temp-profile", pProfile);

    guint16 nTemperature = (guint16) g_variant_get_double (pVariant);
    GVariant *pTemperature = g_variant_new_uint16 (nTemperature);
    g_settings_set_value (self->pPrivate->pSettings, "color-temp", pTemperature);
}

static gboolean settingsToActionState(GValue *pValue, GVariant *pVariant, gpointer pData)
{
    g_value_set_variant (pValue, pVariant);

    return TRUE;
}

static GVariant* actionStateToSettings(const GValue *pValue, const GVariantType * pType, gpointer pData)
{
    return g_value_dup_variant (pValue);
}

static void onSettings (GSimpleAction *pAction, GVariant *pVariant, gpointer pData)
{
    gboolean bMate = ayatana_common_utils_is_mate ();
    gboolean bXfce = ayatana_common_utils_is_xfce ();

    if (bMate)
    {
        ayatana_common_utils_execute_command ("mate-display-properties");
    }
    else if (bXfce)
    {
        ayatana_common_utils_execute_command ("xfce4-display-settings");
    }
    else
    {
        ayatana_common_utils_execute_command ("gnome-control-center display");
    }
}

static void onDispose (GObject *pObject)
{
    IndicatorDisplay *self = INDICATOR_DISPLAY (pObject);

    if (self->pPrivate->nCallback)
    {
        g_source_remove (self->pPrivate->nCallback);
    }

    if (self->pPrivate->pSettings != NULL)
    {
        g_signal_handlers_disconnect_by_data (self->pPrivate->pSettings, self);
        g_clear_object (&self->pPrivate->pSettings);
    }

    g_clear_object (&self->pPrivate->pIndicator);
    g_clear_object (&self->pPrivate->pColorSchemeSettings);
    g_clear_object (&self->pPrivate->pActionGroup);
    g_clear_object (&self->pPrivate->pMenu);

    G_OBJECT_CLASS (indicator_display_parent_class)->dispose (pObject);
}

static void indicator_display_init (IndicatorDisplay *self)
{
    self->pPrivate = indicator_display_get_instance_private (self);
    self->pPrivate->nCallback = 0;
    self->pPrivate->pColorSchemeSettings = NULL;
    self->pPrivate->bAutoSliderUpdate = FALSE;
    self->pPrivate->fLastBrightness = 0.0;
    self->pPrivate->nLastColorTemp = 0;
    self->pPrivate->pActionGroup = NULL;
    self->pPrivate->sLastColorScheme = "default";

    const gchar *sWayland = g_getenv ("WAYLAND_DISPLAY");

    if (sWayland != NULL)
    {
        g_warning ("Panic: XSCT does not work on Wayland.");
    }

    self->pPrivate->pSettings = g_settings_new ("org.ayatana.appindicator.display");
    self->pPrivate->pColorSchemeSettings = g_settings_new ("org.gnome.desktop.interface");
    self->pPrivate->pActionGroup = g_simple_action_group_new ();

    GVariantType *pVariantType = g_variant_type_new ("d");
    guint nTemperature = 0;
    g_settings_get (self->pPrivate->pSettings, "color-temp", "q", &nTemperature);
    GVariant *pTemperature = g_variant_new_double (nTemperature);
    GSimpleAction *pAction = g_simple_action_new_stateful ("color-temp", pVariantType, pTemperature);
    g_variant_type_free (pVariantType);
    g_action_map_add_action (G_ACTION_MAP (self->pPrivate->pActionGroup), G_ACTION (pAction));
    g_signal_connect (self->pPrivate->pSettings, "changed::color-temp", G_CALLBACK (onColorTempSettings), self);
    g_signal_connect (pAction, "change-state", G_CALLBACK (onColorTempState), self);
    g_object_unref (G_OBJECT (pAction));

    pVariantType = g_variant_type_new ("s");
    guint nProfile = 0;
    g_settings_get (self->pPrivate->pSettings, "color-temp-profile", "q", &nProfile);
    gchar *sProfile = g_strdup_printf ("%i", nProfile);
    GVariant *pProfile = g_variant_new_string (sProfile);
    pAction = g_simple_action_new_stateful ("profile", pVariantType, pProfile);
    g_free (sProfile);
    g_variant_type_free (pVariantType);
    g_settings_bind_with_mapping (self->pPrivate->pSettings, "color-temp-profile", pAction, "state", G_SETTINGS_BIND_DEFAULT, settingsIntToActionStateString, actionStateStringToSettingsInt, NULL, NULL);
    g_action_map_add_action (G_ACTION_MAP (self->pPrivate->pActionGroup), G_ACTION (pAction));
    g_object_unref (G_OBJECT (pAction));
    g_signal_connect_swapped (self->pPrivate->pSettings, "changed::color-temp-profile", G_CALLBACK (updateColor), self);

    pVariantType = g_variant_type_new ("d");
    gdouble fBrightness = g_settings_get_double (self->pPrivate->pSettings, "brightness");
    GVariant *pBrightness = g_variant_new_double (fBrightness);
    pAction = g_simple_action_new_stateful ("brightness", pVariantType, pBrightness);
    g_variant_type_free (pVariantType);
    g_settings_bind_with_mapping (self->pPrivate->pSettings, "brightness", pAction, "state", G_SETTINGS_BIND_DEFAULT, settingsToActionState, actionStateToSettings, NULL, NULL);
    g_action_map_add_action (G_ACTION_MAP (self->pPrivate->pActionGroup), G_ACTION (pAction));
    g_object_unref (G_OBJECT (pAction));
    g_signal_connect_swapped (self->pPrivate->pSettings, "changed::brightness", G_CALLBACK (updateColor), self);

    pVariantType = g_variant_type_new ("s");
    GVariant *pLight = g_variant_new_string ("light");
    pAction = g_simple_action_new_stateful ("theme", pVariantType, pLight);
    g_variant_type_free (pVariantType);
    g_settings_bind_with_mapping (self->pPrivate->pSettings, "theme-profile", pAction, "state", G_SETTINGS_BIND_DEFAULT, settingsToActionState, actionStateToSettings, NULL, NULL);
    g_action_map_add_action (G_ACTION_MAP (self->pPrivate->pActionGroup), G_ACTION (pAction));
    g_object_unref (G_OBJECT (pAction));
    g_signal_connect_swapped (self->pPrivate->pSettings, "changed::theme-profile", G_CALLBACK (updateColor), self);

    pAction = g_simple_action_new ("settings", NULL);
    g_action_map_add_action (G_ACTION_MAP (self->pPrivate->pActionGroup), G_ACTION (pAction));
    g_signal_connect (pAction, "activate", G_CALLBACK (onSettings), self);
    g_object_unref (G_OBJECT (pAction));

    self->pPrivate->pMenu = g_menu_new ();
    GMenu *pSection = g_menu_new ();
    GMenuItem *pItem;

    GIcon *pIconMin = g_themed_icon_new_with_default_fallbacks ("ayatana-appindicator-display-brightness-low");
    GIcon *pIconMax = g_themed_icon_new_with_default_fallbacks ("ayatana-appindicator-display-brightness-high");
    GVariant *pIconMinSerialised = g_icon_serialize (pIconMin);
    GVariant *pIconMaxSerialised = g_icon_serialize (pIconMax);
    pItem = g_menu_item_new (_("Brightness"), "indicator.brightness");
    g_menu_item_set_attribute (pItem, "x-ayatana-type", "s", "org.ayatana.indicator.slider");
    g_menu_item_set_attribute (pItem, "x-ayatana-type", "s", "org.ayatana.indicator.slider");
    g_menu_item_set_attribute_value (pItem, "min-icon", pIconMinSerialised);
    g_menu_item_set_attribute_value (pItem, "max-icon", pIconMaxSerialised);
    g_menu_item_set_attribute (pItem, "min-value", "d", 0.5);
    g_menu_item_set_attribute (pItem, "max-value", "d", 1.0);
    g_menu_item_set_attribute (pItem, "step", "d", 0.01);
    g_menu_item_set_attribute (pItem, "digits", "y", 2);
    g_menu_append_item (pSection, pItem);

    pIconMin = g_themed_icon_new_with_default_fallbacks ("ayatana-appindicator-display-colortemp-on");
    pIconMax = g_themed_icon_new_with_default_fallbacks ("ayatana-appindicator-display-colortemp-off");
    pIconMinSerialised = g_icon_serialize (pIconMin);
    pIconMaxSerialised = g_icon_serialize (pIconMax);
    pItem = g_menu_item_new (_("Color temperature"), "indicator.color-temp");
    g_menu_item_set_attribute (pItem, "x-ayatana-type", "s", "org.ayatana.indicator.slider");
    g_menu_item_set_attribute_value (pItem, "min-icon", pIconMinSerialised);
    g_menu_item_set_attribute_value (pItem, "max-icon", pIconMaxSerialised);
    g_menu_item_set_attribute (pItem, "min-value", "d", 3000.0);
    g_menu_item_set_attribute (pItem, "max-value", "d", 6500.0);
    g_menu_item_set_attribute (pItem, "step", "d", 100.0);
    g_menu_item_set_attribute (pItem, "digits", "y", 0);
    g_menu_append_item (pSection, pItem);

    GMenu *pMenuProfiles = g_menu_new ();
    GMenuItem *pItemProfiles = g_menu_item_new_submenu (_("Color temperature profile"), G_MENU_MODEL (pMenuProfiles));
    nProfile = 0;

    while (m_lTempProfiles[nProfile].sName != NULL)
    {
        gchar *sAction = g_strdup_printf ("indicator.profile::%u", nProfile);
        gchar *sName = gettext (m_lTempProfiles[nProfile].sName);
        GMenuItem *pItemProfile = g_menu_item_new (sName, sAction);
        g_free (sAction);
        g_menu_append_item (pMenuProfiles, pItemProfile);
        g_object_unref (pItemProfile);
        nProfile++;
    }

    g_menu_append_item (pSection, pItemProfiles);
    g_object_unref (pItemProfiles);
    g_object_unref (pMenuProfiles);

    g_menu_append_section (self->pPrivate->pMenu, NULL, G_MENU_MODEL (pSection));
    g_object_unref (pIconMin);
    g_object_unref (pIconMax);
    g_variant_unref (pIconMinSerialised);
    g_variant_unref (pIconMaxSerialised);
    g_object_unref (pSection);
    g_object_unref (pItem);

    pSection = g_menu_new ();
    pMenuProfiles = g_menu_new ();
    pItemProfiles = g_menu_item_new_submenu (_("Theme profile"), G_MENU_MODEL (pMenuProfiles));
    GMenuItem *pItemProfile = g_menu_item_new (_("Light"), "indicator.theme::light");
    g_menu_append_item (pMenuProfiles, pItemProfile);
    g_object_unref (pItemProfile);
    pItemProfile = g_menu_item_new (_("Dark"), "indicator.theme::dark");
    g_menu_append_item (pMenuProfiles, pItemProfile);
    g_object_unref (pItemProfile);
    pItemProfile = g_menu_item_new (_("Adaptive"), "indicator.theme::adaptive");
    g_menu_append_item (pMenuProfiles, pItemProfile);
    g_object_unref (pItemProfile);
    g_menu_append_item (pSection, pItemProfiles);
    g_object_unref (pItemProfiles);
    g_object_unref (pMenuProfiles);
    g_menu_append_section (self->pPrivate->pMenu, NULL, G_MENU_MODEL (pSection));
    g_object_unref (pSection);

    pSection = g_menu_new ();
    pItem = g_menu_item_new (_("Display settings..."), "indicator.settings");
    g_menu_append_item (pSection, pItem);
    g_menu_append_section (self->pPrivate->pMenu, NULL, G_MENU_MODEL (pSection));
    g_object_unref (pSection);
    g_object_unref (pItem);

    self->pPrivate->fLatitude = g_settings_get_double (self->pPrivate->pSettings, "latitude");
    self->pPrivate->fLongitude = g_settings_get_double (self->pPrivate->pSettings, "longitude");
    gclue_simple_new ("ayatana-appindicator-display", GCLUE_ACCURACY_LEVEL_CITY, NULL, onGeoClueLoaded, self);
    self->pPrivate->nCallback = g_timeout_add_seconds (60, updateColor, self);
    updateColor (self);

    self->pPrivate->pIndicator = app_indicator_new ("ayatana-appindicator-display", "video-display-panel", APP_INDICATOR_CATEGORY_HARDWARE);
    app_indicator_set_icon (self->pPrivate->pIndicator, "video-display-panel", _("Display settings and features"));
    app_indicator_set_menu (self->pPrivate->pIndicator, self->pPrivate->pMenu);
    app_indicator_set_actions (self->pPrivate->pIndicator, self->pPrivate->pActionGroup);
    app_indicator_set_tooltip (self->pPrivate->pIndicator, NULL, NULL, _("Display settings and features"));
    app_indicator_set_status (self->pPrivate->pIndicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_title (self->pPrivate->pIndicator, _("Display"));
}

static void indicator_display_class_init (IndicatorDisplayClass *klass)
{
    GObjectClass *pClass = G_OBJECT_CLASS (klass);
    pClass->dispose = onDispose;
}

IndicatorDisplay *indicator_display_new ()
{
    GObject *pObject = g_object_new (INDICATOR_DISPLAY_TYPE, NULL);

    return INDICATOR_DISPLAY (pObject);
}

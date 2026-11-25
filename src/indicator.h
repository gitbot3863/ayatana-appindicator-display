#ifndef __INDICATOR_DISPLAY_H__
#define __INDICATOR_DISPLAY_H__

#include <ayatana-appindicator.h>

G_BEGIN_DECLS

#define INDICATOR_DISPLAY(pObject) (G_TYPE_CHECK_INSTANCE_CAST((pObject), INDICATOR_DISPLAY_TYPE, IndicatorDisplay))
#define INDICATOR_DISPLAY_TYPE (indicator_display_get_type ())

typedef struct
{
    AppIndicator *pIndicator;
    GMenu *pMenu;
    GSimpleActionGroup* pActionGroup;
    GSettings *pSettings;
    GSettings *pColorSchemeSettings;
    guint nCallback;
    gdouble fLatitude;
    gdouble fLongitude;
    gboolean bAutoSliderUpdate;
    gdouble fLastBrightness;
    guint nLastColorTemp;
    const gchar *sLastColorScheme;
} IndicatorDisplayPrivate;

typedef struct
{
    GObject pParent;
    IndicatorDisplayPrivate *pPrivate;
} IndicatorDisplay;

typedef struct
{
    GObjectClass pParentClass;
} IndicatorDisplayClass;

GType indicator_display_get_type ();
IndicatorDisplay* indicator_display_new ();

G_END_DECLS

#endif

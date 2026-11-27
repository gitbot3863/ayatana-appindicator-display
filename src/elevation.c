#include <math.h>
#include "elevation.h"

#define DEGREES_TO_RADIANS(fValue) ((fValue) * (M_PI / 180.0))
#define RADIANS_TO_DEGREES(fValue) ((fValue) * (180.0 / M_PI))

static gdouble sunGeometricMeanAnomaly (gdouble fJulianCentury)
{
    return DEGREES_TO_RADIANS (357.52911 + fJulianCentury * (35999.05029 - fJulianCentury * 0.0001537));
}

static gdouble sunGeometricMeanLongitude (gdouble fJulianCentury)
{
    return DEGREES_TO_RADIANS (fmod (280.46646 + fJulianCentury * (36000.76983 + fJulianCentury * 0.0003032), 360.0));
}

static gdouble sunCorrectedObliquity (gdouble fJulianCentury)
{
    gdouble fObliquityArcSeconds = 21.448 - fJulianCentury * (46.815 + fJulianCentury * (0.00059 - fJulianCentury * 0.001813));
    gdouble fMeanObliquity = DEGREES_TO_RADIANS (23.0 + (26.0 + fObliquityArcSeconds / 60.0) / 60.0);
    gdouble fOmega = 125.04 - fJulianCentury * 1934.136;

    return fMeanObliquity + DEGREES_TO_RADIANS (0.00256 * cos (DEGREES_TO_RADIANS (fOmega)));
}

gdouble solarElevation (gdouble fUnixTimestamp, gdouble fLatitude, gdouble fLongitude)
{
    gdouble fJulianDay = fUnixTimestamp / 86400.0 + 2440587.5;
    gdouble fJulianCentury = (fJulianDay - 2451545.0) / 36525.0;
    gdouble fMinutesOfDay = fmod (fUnixTimestamp, 86400.0) / 60.0;
    gdouble fObliquity = sunCorrectedObliquity (fJulianCentury);
    gdouble fMeanLongitude = sunGeometricMeanLongitude (fJulianCentury);
    gdouble fOrbitalEccentricity = 0.016708634 - fJulianCentury * (0.000042037 + fJulianCentury * 0.0000001267);
    gdouble fMeanAnomaly = sunGeometricMeanAnomaly (fJulianCentury);
    gdouble fY = pow (tan (fObliquity / 2.0), 2.0);
    gdouble fEquationOfTime = fY * sin (2.0 * fMeanLongitude) - 2.0 * fOrbitalEccentricity * sin (fMeanAnomaly) + 4.0 * fOrbitalEccentricity * fY * sin (fMeanAnomaly) * cos (2.0 * fMeanLongitude) - 0.5 * fY * fY * sin (4.0 * fMeanLongitude) - 1.25 * fOrbitalEccentricity * fOrbitalEccentricity * sin (2.0 * fMeanAnomaly);
    gdouble fEquationOfTimeMinutes = 4.0 * RADIANS_TO_DEGREES (fEquationOfTime);
    gdouble fEquationOfCenter = sin (fMeanAnomaly) * (1.914602 - fJulianCentury * (0.004817 + 0.000014 * fJulianCentury)) + sin (2.0 * fMeanAnomaly) * (0.019993 - 0.000101 * fJulianCentury) + sin (3.0 * fMeanAnomaly) * 0.000289;
    gdouble fTrueLongitude = fMeanLongitude + DEGREES_TO_RADIANS (fEquationOfCenter);
    gdouble fApparentLongitude = fTrueLongitude - DEGREES_TO_RADIANS (0.00569 + 0.00478 * sin (DEGREES_TO_RADIANS (125.04 - 1934.136 * fJulianCentury)));
    gdouble fDeclination = asin (sin (fObliquity) * sin (fApparentLongitude));
    gdouble fLatitudeRadians = DEGREES_TO_RADIANS (fLatitude);

    if (fMinutesOfDay < 0.0)
    {
        fMinutesOfDay += 1440.0;
    }

    gdouble fHourAngle = DEGREES_TO_RADIANS ((720.0 - fMinutesOfDay - fEquationOfTimeMinutes) / 4.0 - fLongitude);
    gdouble fSolarElevation = asin (CLAMP (cos (fHourAngle) * cos (fLatitudeRadians) * cos (fDeclination) + sin (fLatitudeRadians) * sin (fDeclination), -1.0, 1.0));

    return RADIANS_TO_DEGREES (fSolarElevation);
}

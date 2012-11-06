#ifndef SOLARREDIATION_H
#define SOLARREDIATION_H

class SolarRediation
{
public:
    /** @brief Calculate solar radiatio altitude and alpha in rad */
    static double BeamRadiation(unsigned int dayOftheYear, double elevation, double altitude, double alpha);

};

#endif // SOLARREDIATION_H

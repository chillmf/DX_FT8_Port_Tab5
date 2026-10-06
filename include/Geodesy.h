#pragma once

typedef struct
{
    double latitude;
    double longitude;
    bool isValid;
} LatLong;

typedef struct
{
    double distance_m;  // meters
    double fwd_azimuth; // degrees
    double rev_azimuth; // degrees
    bool isValid;
} VincentyResult;

VincentyResult VincentyDistance(double lat1, double long1, double lat2, double long2);
double RadioHorizon(double height);
bool IsValidLocator(const char *locator);
LatLong QRAtoLatLong(const char *locator);

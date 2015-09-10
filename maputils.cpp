#include "maputils.h"

MapUtils::MapUtils(QObject *parent) :
    QObject(parent)
{
}

double MapUtils::min(double a, double b){
    if(a < b){
        return a;
    }
    return b;
}

double MapUtils::max(double a, double b){
    if(a < b){
        return b;
    }
    return a;
}

double MapUtils::Clip(double n, double minValue, double maxValue){
    return min(max(n, minValue), maxValue);
}

uint MapUtils::MapSize(int levelOfDetail){
    return (uint) 256 << levelOfDetail;
}

double MapUtils::GroundResolution(double latitude, int levelOfDetail){
    latitude = Clip(latitude, MinLatitude, MaxLatitude);
    return cos(latitude * pi / 180) * 2 * pi * EarthRadius / MapSize(levelOfDetail);
}

double MapUtils::MapScale(double latitude, int levelOfDetail, int screenDpi){
    return GroundResolution(latitude, levelOfDetail) * screenDpi / 0.0254;
}

void MapUtils::LatLongToPixelXY(double latitude, double longitude, int levelOfDetail, int &pixelX, int &pixelY){
    latitude = Clip(latitude, MinLatitude, MaxLatitude);
    longitude = Clip(longitude, MinLongitude, MaxLongitude);

    double x = (longitude + 180) / 360;
    double sinLatitude = sin(latitude * pi / 180);
    double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * pi);

    uint mapSize = MapSize(levelOfDetail);
    pixelX = (int) Clip(x * mapSize + 0.5, 0, mapSize - 1);
    pixelY = (int) Clip(y * mapSize + 0.5, 0, mapSize - 1);
}

void MapUtils::PixelXYToLatLong(int pixelX, int pixelY, int levelOfDetail, double &latitude, double &longitude){
    double mapSize = MapSize(levelOfDetail);
    double x = (Clip(pixelX, 0, mapSize - 1) / mapSize) - 0.5;
    double y = 0.5 - (Clip(pixelY, 0, mapSize - 1) / mapSize);

    latitude = 90 - 360 * atan(exp(-y * 2 * pi)) / pi;
    longitude = 360 * x;
}

void MapUtils::PixelXYToTileXY(int pixelX, int pixelY, int &tileX, int &tileY){
    tileX = pixelX / 256;
    tileY = pixelY / 256;
}

void MapUtils::TileXYToPixelXY(int tileX, int tileY, int &pixelX, int &pixelY){
    pixelX = tileX * 256;
    pixelY = tileY * 256;
}

QString MapUtils::TileXYToQuadKey(int tileX, int tileY, int levelOfDetail){
    QString quadKey;
    for (int i = levelOfDetail; i > 0; i--)
    {
        char digit = '0';
        int mask = 1 << (i - 1);
        if ((tileX & mask) != 0)
        {
            digit++;
        }
        if ((tileY & mask) != 0)
        {
            digit++;
            digit++;
        }
        quadKey.append(digit);
    }
    return quadKey;
}

void MapUtils::QuadKeyToTileXY(QString quadKey, int &tileX, int &tileY, int &levelOfDetail){
    tileX = tileY = 0;
    levelOfDetail = quadKey.length();
    for (int i = levelOfDetail; i > 0; i--)
    {
        int mask = 1 << (i - 1);
        switch (quadKey[levelOfDetail - i].toLatin1())
        {
        case '0':
            break;

        case '1':
            tileX |= mask;
            break;

        case '2':
            tileY |= mask;
            break;

        case '3':
            tileX |= mask;
            tileY |= mask;
            break;

        default:
            tileX = -1;
            tileY = -1;
            levelOfDetail = -1;
            return;
        }
    }
}

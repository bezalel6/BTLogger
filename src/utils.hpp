#pragma once
#include "Arduino.h"

struct Point {
    int x;
    int y;
};

struct TouchCoordinates {
    int x;
    int y;
    bool isTouched;

    TouchCoordinates() : x(0), y(0), isTouched(false) {}
    TouchCoordinates(int x, int y, bool isTouched) : x(x), y(y), isTouched(isTouched) {}

    Point toPoint() const {
        return {x, y};
    }
};

using f_void = void (*)();

class Rectangle {
   public:
    Point origin;
    Point topRight;
    int w;
    int h;
    Rectangle() = default;
    Rectangle(int x, int y, int w, int h) : Rectangle({x, y}, w, h) {}
    Rectangle(Point origin, int w, int h) {
        this->origin = origin;
        this->w = w;
        this->h = h;
        this->topRight = {origin.x + w, origin.y + h};
    }
    Point getMiddle() {
        return {origin.x + (w / 2), origin.y + (h / 2)};
    }
    bool checkInside(Point p) {
        // Add bounds validation to prevent potential issues
        if (w <= 0 || h <= 0) {
            return false;
        }

        bool result = (p.x >= origin.x && p.x <= topRight.x && p.y >= origin.y && p.y <= topRight.y);
        return result;
    }
};

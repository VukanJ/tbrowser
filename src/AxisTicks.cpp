#include "AxisTicks.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

#include <ncurses.h>
#include "TAxis.h"

#include "definitions.h"

AxisTicks::AxisTicks(double min, double max, int napprox, bool logarithmic) 
    : vmin(min), vmax(max) 
{
    if (min > max) {
        throw std::runtime_error("max > min is required");
    }

    if (logarithmic) {
        init_logarithmic();
    }
    else {
        init_linear(napprox);
    }

    assert(static_cast<int>(values_d.size()) == nticks);
    assert(static_cast<int>(values_i.size()) == nticks);
    assert(static_cast<int>(values_str.size()) == nticks);
}

void AxisTicks::init_logarithmic() {
    logarithmic = true;
    double minMag = floor(log10(std::max<double>(vmin, 0.5)));
    double maxMag = ceil(log10(vmax));

    if (minMag == maxMag) {
        maxMag++;
    }

    vmin = log10(std::max<double>(vmin, 0.5));
    vmax = log10(std::max<double>(vmax, 0.5));
    vmin_adjust = minMag;
    vmax_adjust = maxMag;

    values_d.reserve(maxMag - minMag);
    values_i.reserve(maxMag - minMag);
    values_str.reserve(maxMag - minMag);
    for (int mag = minMag; mag <= maxMag; ++mag) {
        values_d.push_back(mag);
        values_i.push_back(mag);
        values_str.push_back(std::to_string(values_i.back()));
    }

    // Keep it simple
    E = 0;
    nticks = values_d.size();
    integer = true;
}

void AxisTicks::init_linear(int napprox) {
    {
        double range = vmax - vmin;
        double rawStep = range / napprox;
        double mag = floor(log10(range / napprox));

        rawStep = round(rawStep / pow(10, mag)) * pow(10, mag);
        vmin_adjust = floor(vmin / rawStep) * rawStep;
        vmax_adjust = ceil(vmax / rawStep) * rawStep;
        nticks = round((vmax_adjust - vmin_adjust) / rawStep);
    }

    TAxis xaxis;
    xaxis.Set(nticks, vmin_adjust, vmax_adjust);
    int nBinsX = xaxis.GetNbins();

    // Collect ticks
    for (int i = 0; i <= nBinsX; ++i) {
        double tickPos = xaxis.GetBinLowEdge(i + 1);
        values_d.push_back(tickPos);
        values_i.push_back(lround(tickPos));
    }

    // Check if all are integer
    nticks = values_i.size();
    integer = std::all_of(values_d.begin(), values_d.end(), 
            [](double val){ return trunc(val) == val; });

    auto count_0 = [](long I){
        auto c = 0;
        while (I % 10 == 0 && I != 0) { I /= 10; c++; }
        return c;
    };

    if (integer) {
        // Compute axis exponent
        int minNumZero = 10000;
        for (auto v : values_i) {
            int n0 = count_0(v);
            if (v != 0 && n0 < minNumZero) {
                minNumZero = n0;
            }
        }
        // Compact the number, divide by common magnitude
        if (minNumZero >= 3) {
            E = minNumZero;
            for (auto& v : values_i) { 
                v /= std::pow<long>(10, E); 
            }
            for (int j = 0; auto& v : values_d) { 
                v = values_i[j++]; 
            }
        }
        else {
            E = 0;
        }
    }
    else {
        E = -1000;
        for (auto v : values_d) {
            auto mag = trunc(log10(std::abs(v)));
            if (mag > E) {
                E = mag;
            }
        }
        if (std::abs(E) >= 2) {
            for (auto& v : values_d) { 
                v /= std::pow(10.0, E); 
            }
        }
        else {
            E = 0;
        }
    }

    values_str.reserve(values_d.size());
    if (integer) {
        for (auto v : values_i) {
            values_str.push_back(std::to_string(v));
        }
    }
    else {
        for (auto v : values_d) {
            values_str.push_back(fmtstring("{:.7g}", v));
        }
    }
}

double AxisTicks::tickPosition(int i) const {
    return values_d[i] * std::pow(10.0, E);
}

double AxisTicks::min() const {
    return vmin;
}

double AxisTicks::max() const {
    return vmax;
}

double AxisTicks::minAdjusted() const {
    return vmin_adjust;
}

double AxisTicks::maxAdjusted() const {
    return vmax_adjust;
}

int AxisTicks::maxLabelWidth() const {
    auto maxlabel = std::max_element(values_str.begin(), values_str.end(), 
            [](const auto& str1, const auto& str2){ return str1.size() < str2.size(); });
    if (maxlabel != values_str.end()) {
        return maxlabel->size();
    }
    return 0;
}

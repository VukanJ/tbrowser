#include "AxisTicks.h"
#include "TAxis.h"
#include <cmath>
#include <ncurses.h>
#include <cassert>

AxisTicks::AxisTicks(double min, double max) 
    : vmin(min), vmax(max) {
    // Compute range of histogram x axis

    // mvprintw(18, 20, "xmin %f", vmin);
    // mvprintw(19, 20, "xmax %f", vmax);
    {
        int nbins_approx = 10;
        double range = (vmax - vmin);
        double rawStep = (vmax - vmin) / nbins_approx;
        double mag = floor(log10(range / nbins_approx));
        /* attron(COLOR_PAIR(white_on_blue)); */
        // mvprintw(20, 20, "range %f", range);
        // mvprintw(21, 20, "rawStep %f", rawStep);
        // mvprintw(22, 20, "mag %f", mag);

        rawStep = round(rawStep / pow(10, mag)) * pow(10, mag);
        // mvprintw(23, 20, "rawStep %f", rawStep);
        vmin_adjust = floor(vmin / rawStep) * rawStep;
        vmax_adjust = ceil(vmax / rawStep) * rawStep;
        // mvprintw(24, 20, "xmin corrected %f", xmin);
        // mvprintw(25, 20, "xmax corrected %f", xmax);
        nticks = round((vmax_adjust - vmin_adjust) / rawStep);
        // mvprintw(26, 20, "nbins %i", nbins);
        /* attron(COLOR_PAIR(white_on_blue)); */
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
            values_str.push_back(std::format("{:.7g}", v));
        }
    }

    assert(static_cast<int>(values_d.size()) == nticks);
    assert(static_cast<int>(values_i.size()) == nticks);
    assert(static_cast<int>(values_str.size()) == nticks);
}

double AxisTicks::min() const {
    return vmin;
}

double AxisTicks::max() const {
    return vmax;
}

double AxisTicks::min_adjusted() const {
    return vmin_adjust;
}

double AxisTicks::max_adjusted() const {
    return vmax_adjust;
}

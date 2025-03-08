#include "AxisTicks.h"
#include <algorithm>
#include <cmath>
#include <cassert>

#include <string>
#include "TAxis.h"

#include "definitions.h"

AxisTicks::AxisTicks(double min, double max, int napprox, bool logarithmic) 
    : vmin(min), vmax(max) 
{
    // vmin and vmax are lower and upper bounds of the plot data
    // vmin_adjust and vmax_adjust are the adjusted bounds

    if (min > max) {
        throw std::runtime_error(fmtstring("max > min is required, got {} {}", min, max));
    }
    if (min == max) {
        vmin--;
        vmax++;
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
    assert(vmax > vmin);
    assert(vmax_adjust > vmin_adjust);
}

void AxisTicks::init_logarithmic() {
    logarithmic = true;
    double minMag = floor(log10(std::max<double>(vmin, minimum_log_bin)));
    double maxMag = ceil(log10(vmax));

    if (minMag == maxMag) {
        maxMag++;
    }

    vmin = log10(std::max<double>(vmin, minimum_log_bin));
    vmax = log10(std::max<double>(vmax, minimum_log_bin));
    vmin_adjust = minMag;
    vmax_adjust = maxMag;

    int span = maxMag - minMag;
    if (span <= 0) {
        maxMag++;
        span = 1;
    }
    values_d.reserve(span);
    values_i.reserve(span);
    values_str.reserve(span);
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
        int minNumZero = plus_intfinity;
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
        E = minus_intfinity;
        for (auto v : values_d) {
            auto mag = trunc(log10(std::abs(v)));
            E = std::max<double>(mag, E);
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

    // Catch uninitialized exponent just in case
    if (E == plus_intfinity || E == minus_intfinity) {
        E = 0;
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

void AxisTicks::setAxisPixels(int npix) {
    npixel = npix;
}

AxisTicks::Tick AxisTicks::getTick(int i, bool adjusted_range) const {
    if (i < 0 || i >= nticks) { 
        return {}; 
    }
    
    Tick tick;
    if (logarithmic) {
        tick.char_position = (values_i[i] - vmin_adjust) / (vmax_adjust - vmin_adjust) * npixel;
    }
    else {
        if (adjusted_range) {
            tick.char_position = (values_d[i] * std::pow(10.0, E) - vmin_adjust) / (vmax_adjust - vmin_adjust) * npixel;
        }
        else {
            tick.char_position = (values_d[i] * std::pow(10.0, E) - vmin) / (vmax - vmin) * npixel;
        }
    }


    if (tick.char_position < 0 || tick.char_position >= npixel) {
        tick.char_position = -999; // Just to be sure
    }

    if (logarithmic) {
        if (values_i[i] <= 2) {
            tick.tickstr = std::to_string(static_cast<int>(std::pow<int>(10, values_i[i])));
            tick.tickstr_length = tick.tickstr.size();
        }
        else {
            tick.tickstr = fmtstring("10{}", make_superscript(values_i[i]));
            tick.tickstr_length = 2 + values_str[i].size();
        }
    }
    else {
        tick.tickstr = values_str[i];
        tick.tickstr_length = tick.tickstr.size();
    }

    return tick;
}

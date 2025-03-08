#ifndef AXISTICKS_H
#define AXISTICKS_H

#include <vector>
#include <string>
#include <limits>

class AxisTicks {
public:
    AxisTicks() = default;
    AxisTicks(double vmin, double vmax, int napprox=10, bool logarithmic=false);

    struct Tick {
        int char_position = -999;
        std::string tickstr;
        int tickstr_length; // Needed because of unicode
    };

    double tickPosition(int i) const;
    double min() const;
    double max() const;

    double minAdjusted() const;
    double maxAdjusted() const;

    int maxLabelWidth() const;
    void setAxisPixels(int);

    Tick getTick(int i, bool adjusted_range) const;

    std::vector<double> values_d;
    std::vector<long long> values_i;
    std::vector<std::string> values_str;

    bool integer;
    bool logarithmic = false;
    long E = 0; // Exponent
    int nticks = 1;

private:
    constexpr static int plus_intfinity = std::numeric_limits<int>::max();
    constexpr static int minus_intfinity = std::numeric_limits<int>::min();
    void init_linear(int napprox);
    void init_logarithmic();

    int npixel = 0;

    // data minima and maxima
    double vmin = 0;
    double vmax = 1;
    // plot minima and maxima
    double vmin_adjust = 0;
    double vmax_adjust = 1;

};

#endif // AXISTICKS_H

#ifndef AXISTICKS_H
#define AXISTICKS_H

#include <vector>
#include <string>

class AxisTicks {
public:
    AxisTicks(double vmin, double vmax, int napprox=10, bool logarithmic=false);

    struct Tick {
        int char_position = -999;
        std::string tickstr;
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
    void init_linear(int napprox);
    void init_logarithmic();

    int npixel = 0;

    // data minima and maxima
    double vmin;
    double vmax;
    // plot minima and maxima
    double vmin_adjust;
    double vmax_adjust;

};

#endif // AXISTICKS_H

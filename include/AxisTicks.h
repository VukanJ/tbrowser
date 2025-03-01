#ifndef AXISTICKS_H
#define AXISTICKS_H

#include <vector>
#include <string>

class AxisTicks {
public:
    AxisTicks(double vmin, double vmax, int napprox=10);

    double tickPosition(int i) const;
    double min() const;
    double max() const;

    double minAdjusted() const;
    double maxAdjusted() const;

    int maxLabelWidth() const;

    std::vector<double> values_d;
    std::vector<long> values_i;
    std::vector<std::string> values_str;
    bool integer;
    long E = 0; // Exponent
    int nticks = 1;

private:
    // data minima and maxima
    double vmin;
    double vmax;
    // plot minima and maxima
    double vmin_adjust;
    double vmax_adjust;

};

#endif // AXISTICKS_H

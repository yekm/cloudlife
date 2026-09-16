#pragma once

#include <cmath>
#include <limits>
#include <tuple>
#include <variant>
#include <vector>

struct AttractorParameter {
    const char* label;
    std::variant<double*, int*> value;
    double minimum;
    double maximum;
    double step = 0.0001;
};

template <typename F = double>
class StrangeAttractor // : public PointGenerator?
{
public:
    StrangeAttractor() {
        aa = 1, bb = 2, cc = 3, dd = 4;
        reset();
    }
    virtual ~StrangeAttractor() {};
    typedef F float_t;
    typedef std::tuple<F, F> tuple_t;
    virtual std::tuple<F, F> get_point() = 0;


    virtual void reset() {
        //a=b=c=d=e=f=0;
        a = aa; b = bb; c = cc; d = dd; e = ee; f = ff;
        inc = iinc;
    }

    virtual void init_random() {}
    virtual std::vector<AttractorParameter> parameters() = 0;
    virtual const char* validation_error() const { return nullptr; }

    double aa = 0, bb = 0, cc = 0, dd = 0, ee = 0, ff = 0, iinc = 1;

protected:
    // Coefficients are shared storage, but each map chooses which fields to expose.
    std::vector<AttractorParameter> coefficient_parameters(int count, bool increment = false)
    {
        std::vector<AttractorParameter> result;
        const AttractorParameter coefficients[] = {
            {"a", &aa, -20, 20}, {"b", &bb, -20, 20},
            {"c", &cc, -20, 20}, {"d", &dd, -20, 20},
        };
        for (int index = 0; index < count; ++index) {
            result.push_back(coefficients[index]);
        }
        if (increment) {
            result.push_back({"Orbit offset", &iinc, -20, 20});
        }
        result.push_back({"Initial x", &ee, -20, 20});
        result.push_back({"Initial y", &ff, -20, 20});
        return result;
    }

    void set_defaults(double a_value, double b_value, double c_value, double d_value,
                      double x_value = 0.1, double y_value = 0.1, double offset = 0.0)
    {
        aa = a_value;
        bb = b_value;
        cc = c_value;
        dd = d_value;
        ee = x_value;
        ff = y_value;
        iinc = offset;
        reset();
    }

    F a, b, c, d, inc;
    union {
        F e;
        F i;
        F x;
    };
    union {
        F f;
        F j;
        F y;
    };
};

class AMartin: public StrangeAttractor<> {
public:
    AMartin()
    {
        set_defaults(.4, 1, 0, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto t = std::sqrt(std::fabs(b * oldi - c));
        i = oldj + (i < 0 ? t : -t);
        return {i + j, i - j};
    };
};

class AEJK1: public StrangeAttractor<> {
public:
    AEJK1()
    {
        set_defaults(1, .5, 1, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto t = b * oldi - c;
        i = oldj - (i > 0 ? t : -t);
        return {i + j, i - j};
    };
};

class AEJK2: public StrangeAttractor<> {
public:
    const char* validation_error() const override
    {
        if (bb == 0 && cc == 0) {
            return "EJK2 requires a nonzero logarithm argument (b or c must be nonzero).";
        }
        if (bb * (ee + iinc) - cc == 0) {
            return "EJK2 starts at log(0); change a coefficient, initial x, or orbit offset.";
        }
        return nullptr;
    }

    AEJK2()
    {
        set_defaults(1, 1, 1, 0, .1, .1, .1);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto argument = std::fabs(b * oldi - c);
        if (argument == 0) {
            i = j = std::numeric_limits<double>::quiet_NaN();
            return {i, j};
        }
        const auto t = std::log(argument);
        i = oldj - (i < 0 ? t : -t);
        return {i + j, i - j};
    };
};

class AEJK3: public StrangeAttractor<> {
public:
    AEJK3()
    {
        set_defaults(1, 1, 1, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto t = std::sin(b * oldi) - c;
        i = oldj - (i < 0 ? t : -t);
        return {i + j, i - j};
    };
};

class AEJK4: public StrangeAttractor<> {
public:
    AEJK4()
    {
        set_defaults(1, 1, 1, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        i = oldj - (i > 0
                    ? std::sin(b * oldi) - c
                    : - std::sqrt(std::fabs(b * oldi - c)) );
        return {i + j, i - j};
    };
};

class AEJK5: public StrangeAttractor<> {
public:
    AEJK5()
    {
        set_defaults(1, .5, 1, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        i = oldj - (i > 0
                    ? std::sin(b * oldi) - c
                    : -(b * oldi - c) );
        return {i + j, i - j};
    };
};

class AEJK6: public StrangeAttractor<> {
public:
    AEJK6()
    {
        set_defaults(1, 1.1, 0, 0, .1, .1, .1);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(2, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto product = b * oldi;
        i = oldj - std::asin(product - std::trunc(product));
        return {i + j, i - j};
    };
};

class ARR: public StrangeAttractor<> {
public:
    const char* validation_error() const override
    {
        if (dd < 0 && bb * (ee + iinc) - cc == 0) {
            return "RR starts at zero raised to a negative power; change its seed or coefficients.";
        }
        return nullptr;
    }

    ARR()
    {
        set_defaults(.4, 1, 0, .5);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(4, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto base = std::fabs(b * oldi - c);
        if (base == 0 && d < 0) {
            i = j = std::numeric_limits<double>::quiet_NaN();
            return {i, j};
        }
        const auto t = std::pow(base, d);
        i = oldj - (i < 0 ? -t : t );
        return {i + j, i - j};
    };
};

class APOPCORN: public StrangeAttractor<> {
public:
    APOPCORN()
    {
        set_defaults(0, 0, 3, 0, 0, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return {
            {"Grid start column", &m_grid_start_column, 0, GRID_LIMIT, 1},
            {"Grid start row", &m_grid_start_row, 0, GRID_LIMIT, 1},
            {"Grid spacing (degrees)", &cc, .01, 20, .01},
        };
    }

    const char* validation_error() const override
    {
        if (m_grid_start_column < 0 || m_grid_start_column > GRID_LIMIT ||
            m_grid_start_row < 0 || m_grid_start_row > GRID_LIMIT) {
            return "Popcorn grid start column and row must be between 0 and 50.";
        }
        if (!std::isfinite(cc) || cc <= 0) {
            return "Popcorn grid spacing must be a positive finite value.";
        }
        return nullptr;
    }

    void reset() override
    {
        StrangeAttractor::reset();
        a = m_grid_start_column;
        b = m_grid_start_row;
        inc = 0;
        seed_grid_point();
    }

    tuple_t get_point() override
    {
        if (inc >= ORBIT_STEPS) {
            inc = 0;
            if (++a > GRID_LIMIT) {
                a = 0;
                if (++b > GRID_LIMIT) {
                    b = 0;
                }
            }
            seed_grid_point();
        }
        const auto next_i = i - STEP_SIZE * std::sin(j + std::tan(3.0 * j));
        const auto next_j = j - STEP_SIZE * std::sin(i + std::tan(3.0 * i));
        i = next_i;
        j = next_j;
        ++inc;
        return {i, j};
    }

private:
    static constexpr int GRID_LIMIT = 50;
    static constexpr int ORBIT_STEPS = 100;
    static constexpr double STEP_SIZE = .05;

    int m_grid_start_column = 0;
    int m_grid_start_row = 0;

    void seed_grid_point()
    {
        constexpr double degrees_to_radians = 3.14159265358979323846 / 180.0;
        i = (-c * GRID_LIMIT / 2 + c * a) * degrees_to_radians;
        j = (-c * GRID_LIMIT / 2 + c * b) * degrees_to_radians;
    }
};

class AJONG: public StrangeAttractor<> {
public:
    AJONG()
    {
        set_defaults(1.4, -2.3, 2.4, -2.1);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(4, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;

        j = sin(c * i) - cos(d * j);
        i = sin(a * oldj) - cos(b * oldi);

        return {i + j, i - j};
    };
};

class ASINE: public StrangeAttractor<> {
public:
    ASINE()
    {
        set_defaults(0, 0, 0, 0, 1, 1);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(1, true);
    }

    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;

        j = a - i;
        i = oldj - sin(oldi);

        return {i + j, i - j};
    };
};



class AXY : public StrangeAttractor<> {
public:
    using StrangeAttractor::float_t;
    using StrangeAttractor::tuple_t;
    virtual tuple_t get_point() override {
        std::tie(x, y) = gen_point(x, y);
        return {x, y};
    }
    virtual tuple_t gen_point(float_t x, float_t y) = 0;
};

// https://examples.holoviz.org/gallery/attractors/attractors.html

class AClifford : public AXY {
public:
    AClifford()
    {
        set_defaults(-1.4, 1.6, 1, .7);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(4, false);
    }

    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(a * y) + c * cos(a * x),
            sin(b * x) + d * cos(b * y)
        };
    }
};

class ADeJong : public AXY {
public:
    ADeJong()
    {
        set_defaults(1.4, -2.3, 2.4, -2.1);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(4, false);
    }

    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(a * y) - cos(b * x),
            sin(c * x) - cos(d * y)
        };
    }
};

class ABedhead : public AXY {
public:
    const char* validation_error() const override
    {
        return bb == 0 ? "Bedhead requires b to be nonzero." : nullptr;
    }

    ABedhead()
    {
        set_defaults(-.81, -.92, 0, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(2, false);
    }

    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(x*y/b)*y + cos(a*x-y),
            x + sin(y)/b
        };
    }
};

class AFractalDream : public AXY {
public:
    AFractalDream()
    {
        set_defaults(-.966918, 2.879879, .765145, .744728);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(4, false);
    }

    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(y*b)+c*sin(x*b),
            sin(x*a)+d*sin(y*a)
        };
    }
};

class AHopalong1 : public AXY {
public:
    AHopalong1()
    {
        set_defaults(.4, 1, 0, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, false);
    }

    virtual tuple_t gen_point(float_t x, float_t y) override {
        const auto t = sqrt(fabs(b * x - c));
        return {
            y - (x<0 ? -t : t),
            a - x
        };
    }
};

class AHopalong2 : public AXY {
public:
    AHopalong2()
    {
        set_defaults(.4, 1, 0, 0);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return coefficient_parameters(3, false);
    }

    virtual tuple_t gen_point(float_t x, float_t y) override {
        const auto t = sqrt(fabs(b * x - 1.0 - c));
        return {
            y - 1.0 - (x-1<0 ? -t : t),
            a - x - 1.0
        };
    }
};

class AGumowskiMira : public AXY {
    float_t G(float_t x, float_t mu) {
        return mu * x + 2 * (1 - mu) * x*x / (1.0 + x*x);
    }
public:
    AGumowskiMira()
    {
        set_defaults(.008, .05, -.496, 0, 0, 1);
    }

    std::vector<AttractorParameter> parameters() override
    {
        return {
            {"a", &aa, -20, 20},
            {"b", &bb, -20, 20},
            {"mu", &cc, -1, 1},
            {"Initial x", &ee, -20, 20},
            {"Initial y", &ff, -20, 20},
        };
    }
    virtual tuple_t gen_point(float_t x, float_t y) override {

        const auto mu = c;
        const auto xn = y + a*(1 - b*y*y)*y + G(x, mu);
        return {
            xn,
            -x + G(xn, mu)
        };
    }
};


class ASymmetricIcon : public AXY {
public:
    ASymmetricIcon()
    {
        aa = 1.8;
        bb = 0.0;
        cc = 1.0;
        dd = 0.0;
        ee = 0.01;
        ff = 0.01;
        reset();
    }

    std::vector<AttractorParameter> parameters() override
    {
        return {
            {"Alpha", &aa, -20, 20},
            {"Beta", &bb, -20, 20},
            {"Gamma", &cc, -20, 20},
            {"Omega", &dd, -20, 20},
            {"Lambda", &lambda, -20, 20},
            {"Degree", &degree, 2, 12, 1},
            {"Initial x", &ee, -20, 20},
            {"Initial y", &ff, -20, 20},
        };
    }

    const char* validation_error() const override
    {
        return degree < 2 || degree > 12 ? "Symmetric Icon degree must be between 2 and 12." : nullptr;
    }

    // a = alpha, b = beta, c = gamma, d = omega. Keep the seed fields e/f
    // separate from lambda and the integer symmetry degree (at least two).
    double lambda = -1.93;
    int degree = 5;

    tuple_t gen_point(float_t x, float_t y) override
    {
        auto zreal = x;
        auto zimag = y;
        const auto symmetry_degree = degree < 2 ? 2 : degree;
        // zreal + i*zimag = z^(degree-1), while zn below is Re(z^degree).
        for (int power = 1; power < symmetry_degree - 1; ++power) {
            const auto next_real = zreal * x - zimag * y;
            const auto next_imag = zimag * x + zreal * y;
            zreal = next_real;
            zimag = next_imag;
        }

        const auto zn = x * zreal - y * zimag;
        const auto p = a * (x * x + y * y) + lambda + b * zn;
        return {
            p * x + c * zreal - d * y,
            p * y - c * zimag + d * x
        };
    }
};

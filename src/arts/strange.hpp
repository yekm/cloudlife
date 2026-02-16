#include <cmath>
#include <tuple>

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


    void reset() {
        //a=b=c=d=e=f=0;
        a = aa; b = bb; c = cc; d = dd; e = ee; f = ff;
        inc = iinc;
    }

    virtual void init_random() {}

    double aa = 0, bb = 0, cc = 0, dd = 0, ee = 0, ff = 0, iinc = 1;

protected:
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
    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto t = std::log(std::fabs(b * oldi - c));
        i = oldj - (i < 0 ? t : -t);
        return {i + j, i - j};
    };
};

class AEJK3: public StrangeAttractor<> {
public:
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
    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        i = oldj - std::asin((b * oldi) - (long) (b * oldi));
        return {i + j, i - j};
    };
};

class ARR: public StrangeAttractor<> {
public:
    virtual StrangeAttractor::tuple_t get_point() override {
        auto oldj = j;
        auto oldi = i + inc;
        j = a - i;
        const auto t = std::pow(std::fabs(b * oldi - c), d);
        i = oldj - (i < 0 ? -t : t );
        return {i + j, i - j};
    };
};

class APOPCORN: public StrangeAttractor<> {
public:
    virtual StrangeAttractor::tuple_t get_point() override {
#define HVAL 0.05
#define INCVAL 50

        if (inc >= 100)
            inc = 0;
        if (inc == 0) {
            if (a++ >= INCVAL) {
                a = 0;
                if (b++ >= INCVAL)
                    b = 0;
            }
            i = (-c * INCVAL / 2 + c * a) * M_PI / 180.0;
            j = (-c * INCVAL / 2 + c * b) * M_PI / 180.0;
        }
        auto tempi = i - HVAL * sin(j + tan(3.0 * j));
        auto tempj = j - HVAL * sin(i + tan(3.0 * i));
        //xp->x = centerx + (int) (w / 40 * tempi);
        //xp->y = centery + (int) (h / 40 * tempj);
        i = tempi;
        j = tempj;

        return {i, j};
    };
};

class AJONG: public StrangeAttractor<> {
public:
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
    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(a * y) + c * cos(a * x),
            sin(b * x) + d * cos(b * y)
        };
    }
};

class ADeJong : public AXY {
public:
    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(a * y) - cos(b * x),
            sin(c * x) - cos(d * y)
        };
    }
};

class ABedhead : public AXY {
public:
    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(x*y/b)*y + cos(a*x-y),
            x + sin(y)/b
        };
    }
};

class AFractalDream : public AXY {
public:
    virtual tuple_t gen_point(float_t x, float_t y) override {
        return {
            sin(y*b)+c*sin(x*b),
            sin(x*a)+d*sin(y*a)
        };
    }
};

class AHopalong1 : public AXY {
public:
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
    AGumowskiMira(){
        ee=0, ff=1, aa=0.008, bb=0.05, cc=-0.496;
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


/*

def Symmetric_Icon(x, y, a, b, g, om, l, d, *o):
    zzbar = x*x + y*y
    p = a*zzbar + l
    zreal, zimag = x, y

    for i in range(1, d-1):
        za, zb = zreal * x - zimag * y, zimag * x + zreal * y
        zreal, zimag = za, zb

    zn = x*zreal - y*zimag
    p += b*zn

    return p*x + g*zreal - om*y, p*y - g*zimag + om*x

class ASymmetricIcon : public AXY {
public:
    ASymmetricIcon() {
        ee=0, ff=1, aa=0.008, bb=0.05, cc=-0.496;
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
*/

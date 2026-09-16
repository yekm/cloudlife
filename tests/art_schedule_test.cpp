#include "art.hpp"

#include <cstdio>

class ScheduledArt : public Art {
public:
    explicit ScheduledArt(double period)
        : Art("Schedule test")
    {
        shuffle_period = period;
    }

    unsigned shuffles = 0;

    std::string about() const override { return "Scheduling test fixture."; }

private:
    bool render(uint32_t*) override { return false; }
    void shuffle() override { ++shuffles; }
};

int main()
{
    // No GUI or graphics context: scheduling must work independently of both.
    ScheduledArt art(42);
    art.check_shuffle(1000);
    art.check_shuffle(1041.99);
    if (art.shuffles != 0)
        return 1;
    art.check_shuffle(1042);
    art.check_shuffle(1042);
    if (art.shuffles != 1)
        return 2;
    art.check_shuffle(2000);
    if (art.shuffles != 2)
        return 3;
    art.check_shuffle(0);
    art.check_shuffle(41);
    if (art.shuffles != 2)
        return 4;
    art.check_shuffle(42);
    if (art.shuffles != 3)
        return 5;

    ScheduledArt disabled(0);
    disabled.check_shuffle(0);
    disabled.check_shuffle(10000);
    if (disabled.shuffles != 0)
        return 6;

    std::puts("Art scheduling passes without GUI or graphics context.");
}

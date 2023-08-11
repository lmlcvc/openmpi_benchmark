#ifndef SCANBENCHMARK_H
#define SCANBENCHMARK_H

#include "benchmark.h"

class ScanBenchmark : public Benchmark
{
public:
    ScanBenchmark(int argc, char **argv, int rank);
    virtual ~ScanBenchmark() override {}

    void setup(int rank) override;
    void run() override;

private:
    void performWarmup() override;

    const std::size_t maxPower = 22;
};

#endif // SCANBENCHMARK_H
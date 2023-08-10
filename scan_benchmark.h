#ifndef SCANBENCHMARK_H
#define SCANBENCHMARK_H

#include "benchmark.h"

class ScanBenchmark : public Benchmark
{
public:
    ScanBenchmark(int argc, char **argv);
    virtual ~ScanBenchmark() override {}
    void setup() override;
    void run() override;

private:
    void allocateMemory() override;

    const std::size_t maxPower = 22;
};

#endif // SCANBENCHMARK_H
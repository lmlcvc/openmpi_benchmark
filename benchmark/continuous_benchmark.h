#ifndef CONTINUOUSBENCHMARK_H
#define CONTINUOUSBENCHMARK_H

#include "benchmark.h"

class ContinuousBenchmark : public Benchmark
{
public:
protected:
    void allocateMemory() override;

    const std::size_t minMessageSize = 1e4;

private:
};

#endif // CONTINUOUSBENCHMARK_H
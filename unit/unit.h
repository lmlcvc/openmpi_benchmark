#ifndef UNIT_H
#define UNIT_H

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <time.h>
#include <functional>
#include <iostream>
#include <unistd.h>
#include <numeric>
#include <mpi.h>

enum UnitType
{
    UNDEFINED,
    RU,
    BU
};

class Unit
{
public:
    Unit();
    ~Unit() {}
    void allocateMemory();

    const int getRank() { return m_rank; }

    const std::string getId() { return m_id; }
    void setId(std::string id) { m_id = id; }

    const std::size_t getBufferBytes() const { return m_bufferBytes; }
    void setBufferBytes(std::size_t bytes) { m_bufferBytes = bytes; }

    int8_t *getBuffer() const { return m_buffer; }

    const UnitType getUnitType() const { return m_type; }
    void setUnitType(UnitType type) { m_type = type; }

    void ruShift(int idx);
    void buShift(int idx);
    int getPair(int phase) { return (*m_shift)[phase]; }

protected:
    int m_rank;
    std::string m_id;

    typedef std::unique_ptr<void, std::function<void(void *)>> buffer_t;

    buffer_t m_memBufferPtr;
    std::size_t m_bufferBytes = 1e7;
    int8_t *m_buffer;

    std::unique_ptr<std::vector<int>> m_shift;
    UnitType m_type = UNDEFINED;
};

#endif // UNIT_H
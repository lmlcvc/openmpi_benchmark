#include "readout_unit.h"

ReadoutUnit::ReadoutUnit(int rank)
{
    m_rank = rank;

    allocateMemory();
}

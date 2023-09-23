#include "unit.h"

std::string sanitizeHostname(const std::string &input)
{
    std::string sanitized;
    for (char c : input)
    {
        if (std::isalnum(c) || c == '-')
            sanitized += c;
    }
    return sanitized;
}

Unit::Unit()
{
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // m_shift = std::make_unique<std::vector<int>>;//(size / 2);
    // std::iota(m_shift->begin(), m_shift->end(), 0);
}

void Unit::allocateMemory()
{
    const long pageSize = sysconf(_SC_PAGESIZE);

    void *mem = nullptr;

    if (posix_memalign(&mem, pageSize, m_bufferBytes) != 0)
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    m_buffer = static_cast<int8_t *>(mem);
    std::fill(m_buffer, m_buffer + m_bufferBytes, 0);

    m_memBufferPtr = buffer_t(mem, free);
}

void Unit::parseConfig()
{
    // TODO: should init shift pattern from config file
    std::ifstream file(m_configPath);
    if (!file)
    {
        std::cerr << "Failed to open JSON file: " << m_configPath << std::endl;
        return;
    }

    std::string json;
    std::string line;
    while (std::getline(file, line))
    {
        json += line;
    }

    size_t index = 0, nodeInd = 0;
    while (index < json.size())
    {
        size_t hostnameStart = json.find("\"hostname\"", index);
        if (hostnameStart == std::string::npos)
            break; // No more hostnames found

        size_t valueStart = json.find(":", hostnameStart);
        if (valueStart == std::string::npos)
            break;

        size_t valueEnd = json.find_first_of(",", valueStart + 1);
        if (valueEnd == std::string::npos)
            break;

        // get host name
        std::string hostname = json.substr(valueStart, valueEnd - valueStart - 1);
        hostname = sanitizeHostname(hostname);
        index = valueEnd + 1;

        // get index
        size_t rankidStart = json.find("\"rankid\"", index);
        int rankId;
        if (rankidStart != std::string::npos)
        {
            size_t rankidValueStart = json.find(":", rankidStart);
            if (rankidValueStart != std::string::npos)
            {
                size_t rankidValueEnd = json.find_first_of(",", rankidValueStart + 1);
                if (rankidValueEnd == std::string::npos)
                {
                    rankidValueEnd = json.find("}", rankidValueStart + 1);
                }
                if (rankidValueEnd != std::string::npos)
                {
                    std::string rankidStr = json.substr(rankidValueStart + 1, rankidValueEnd - rankidValueStart);
                    rankId = std::stoi(rankidStr);
                }
            }
        }

        if (((m_type == BU) && (nodeInd % 2 == 0))     // append RUs to BU shift vector
            || ((m_type == RU) && (nodeInd % 2 == 1))) // append BUs to RU shift vector
        {
            if (hostname == "-1")   // mark dummies
            {
                m_shift.push_back(-1);
            }
            else
            {
                m_shift.push_back(rankId); 
            }
        }
        nodeInd++;
    }
}

void Unit::ruShift(int idx)
{
    parseConfig();
    std::rotate(m_shift.begin(), m_shift.begin() + idx, m_shift.end());
}

void Unit::buShift(int idx)
{
    parseConfig();

    std::vector<int> vec_r(m_shift.size());
    std::copy(m_shift.rbegin(), m_shift.rend(), vec_r.begin());

    std::copy(vec_r.begin(), vec_r.end(), m_shift.begin());
    std::rotate(m_shift.begin(), m_shift.end() - 1 - idx, m_shift.end());
}
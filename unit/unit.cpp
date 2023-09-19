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

    m_shift = std::make_unique<std::vector<int>>(size / 2);
    std::iota(m_shift->begin(), m_shift->end(), 0);

    parseConfig("host.json"); // TODO: pass as parameter
    for (int pos : m_dummies)
    {
        if (pos >= 0 && pos < m_shift->size())
            m_shift->at(pos) = -1;
    }
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

void Unit::parseConfig(const std::string &jsonFile)
{
    std::ifstream file(jsonFile);
    if (!file)
    {
        std::cerr << "Failed to open JSON file: " << jsonFile << std::endl;
        return;
    }

    std::string json;
    std::string line;
    while (std::getline(file, line))
    {
        json += line;
    }

    size_t index = 0;
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

        std::string hostname = json.substr(valueStart, valueEnd - valueStart - 1);
        hostname = sanitizeHostname(hostname);
        index = valueEnd + 1;

        if (hostname == "-1")
        {
            // get index of dummy
            size_t rankidStart = json.find("\"rankid\"", index);
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
                        int rankid = std::stoi(rankidStr);
                        m_dummies.push_back(rankid);
                    }
                }
            }
        }
    }
}

void Unit::ruShift(int idx)
{
    std::rotate(m_shift->begin(), m_shift->begin() + idx, m_shift->end());
}

void Unit::buShift(int idx)
{
    std::vector<int> vec_r(m_shift->size());
    std::copy(m_shift->rbegin(), m_shift->rend(), vec_r.begin());

    std::copy(vec_r.begin(), vec_r.end(), m_shift->begin());
    std::rotate(m_shift->begin(), m_shift->end() - 1 - idx, m_shift->end());
}
#include "fixed_message.h"

BenchmarkFixedMessage::BenchmarkFixedMessage(std::vector<ArgumentEntry> args, int rank, int size, CommunicationType commType)
{
    m_rank = rank;
    m_nodesCount = size;
    m_commType = commType;
    m_messageSize = 1e5;

    m_unit = std::make_unique<Unit>();

    parseArguments(args);

    if (m_rank == 0 && m_messageSize > m_ruBufferBytes)
    {
        std::cerr << "Message cannot exceed buffer. Exiting." << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    initUnitLists();
    m_unit->allocateMemory();

    m_syncIterations = m_iterations / 1e4;

    if (m_rank == 0)
    {
        std::cout << std::endl
                  << "Performing fixed size benchmark. "
                  << std::endl;

        if (commType == COMM_FIXED_BLOCKING)
        {
            std::cout << "Blocking communication." << std::endl
                      << std::endl;
        }
        else if (commType == COMM_FIXED_NONBLOCKING)
        {
            std::cout << "Non-blocking communication." << std::endl
                      << std::endl;
        }

        std::cout << std::left << std::setw(20) << "Message size:"
                  << std::right << std::setw(10) << m_messageSize << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "RU buffer size:"
                  << std::right << std::setw(10) << m_ruBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "RU buffer size:"
                  << std::right << std::setw(10) << m_buBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Number of iterations:"
                  << std::right << std::setw(9) << m_iterations << std::endl;
    }

    clock_gettime(CLOCK_MONOTONIC, &m_lastAvgCalculationTime);
}

void BenchmarkFixedMessage::parseArguments(std::vector<ArgumentEntry> args)
{
    std::size_t tmp;
    for (const auto &entry : args)
    {
        switch (entry.option)
        {
        case 'r':
            tmp = std::stoul(entry.value);
            if (tmp > 0)
                m_ruBufferBytes = tmp;
            break;
        case 'b':
            tmp = std::stoul(entry.value);
            if (tmp > 0)
                m_buBufferBytes = tmp;
            break;
        case 'm':
            tmp = std::stoul(entry.value);
            m_messageSize = (tmp > 0) ? tmp : m_messageSize;
            break;
        case 'p':
            tmp = std::stoul(entry.value);
            m_messagesPerPhase = (tmp > 0) ? tmp : m_messagesPerPhase;
            break;
        case 'i':
            tmp = std::stoul(entry.value);
            m_iterations = tmp;
            break;
        case 'w':
            tmp = std::stoul(entry.value);
            m_warmupIterations = (tmp > 0) ? tmp : m_warmupIterations;
            break;
        case 'l':
            tmp = std::stoul(entry.value);
            m_lastAvgCalculationInterval = (tmp > 0) ? tmp : m_lastAvgCalculationInterval;
            break;
        case 'c':
            m_unit->setConfigPath(entry.value);
            break;
        default:
            if (m_rank == 0)
            {
                std::cerr << "Invalid argument: " << entry.option << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
        }
    }
}

#include "WatchdogModule.h"

WatchdogModule::WatchdogModule(unsigned uReportTime_s, std::vector<uint8_t> &vu8MACAddress) :
    BaseModule(1),
    m_uReportTime_s(uReportTime_s)
{
     m_vu8MACAddress = vu8MACAddress;
}

void WatchdogModule::ContinuouslyTryProcess()
{
    std::unique_lock<std::mutex> ProcessLock(m_ProcessStateMutex);

    while (!m_bShutDown)
    {
        ProcessLock.unlock();
        auto pBaseChunk = std::make_shared<BaseChunk>();
        Process(pBaseChunk);

        ProcessLock.lock();
    }
}

void WatchdogModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
    std::cout << std::string(__PRETTY_FUNCTION__) + ": Send Watchdog \n";
    
    // Passing data on
    auto pWatchdogChunk = std::make_shared<WatchdogChunk>(m_vu8MACAddress);
    if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pWatchdogChunk)))
    {
        std::cout << std::string(__PRETTY_FUNCTION__) + ": Next buffer full, dropping current chunk and passing \n";
    }

    // Now that report is sent, sleep
    std::this_thread::sleep_for(std::chrono::milliseconds((unsigned)((1000*m_uReportTime_s))));
}
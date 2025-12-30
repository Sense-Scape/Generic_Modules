#include "SimulatorModule.h"
#include "plog/Log.h"
#include <cstdint>
#include <numbers>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

SimulatorModule::SimulatorModule(unsigned uBufferSize,
                                 nlohmann::json_abi_v3_11_2::json jsonConfig)
    : BaseModule(uBufferSize), m_dChunkSize(512),
      m_vu8SourceIdentifier(CheckAndThrowJSON<std::vector<uint8_t>>(
          jsonConfig, "SourceIdentifier")),
      m_dSampleRate(CheckAndThrowJSON<double>(jsonConfig, "SampleRate_Hz")),
      m_fSNR_db(CheckAndThrowJSON<float>(jsonConfig, "SNR_db")),
      m_strADCMode(CheckAndThrowJSON<std::string>(jsonConfig, "ADCMode")),
      m_strClockMode(CheckAndThrowJSON<std::string>(jsonConfig, "ClockMode")),
      m_fSignalPower_dBm(
          CheckAndThrowJSON<float>(jsonConfig, "SignalPower_dBm")),
      m_uSimulatedFrequency(
          CheckAndThrowJSON<uint32_t>(jsonConfig, "SimulatedFrequency_Hz")),
      m_uNumChannels(
          CheckAndThrowJSON<uint16_t>(jsonConfig, "NumberOfChannels")),
      m_i64StartUpDelay_us(
          CheckAndThrowJSON<int64_t>(jsonConfig, "StartupDelay_us")),
      m_u64CurrentTimeStamp_us(
          1767112434 +
          CheckAndThrowJSON<int64_t>(jsonConfig, "StartupDelay_us")) {
  ConfigureModule(jsonConfig);
  PrintConfiguration();
}

void SimulatorModule::PrintConfiguration() {
  std::string strSourceIdentifier = std::accumulate(
      m_vu8SourceIdentifier.begin(), m_vu8SourceIdentifier.end(),
      std::string(""), [](const std::string &str, int element) {
        return str + std::to_string(element) + " ";
      });

  std::string strInfo =
      std::string(__FUNCTION__) + " Simulator Module create:\n" +
      "=========\n" + +"General Config: \n" + "SourceIdentifier [ " +
      strSourceIdentifier + "] \n" + "SampleRate [ " +
      std::to_string(m_dSampleRate) + " ] Hz \n" + "ChunkSize [ " +
      std::to_string(m_dChunkSize) + " ]\n" + "Start Up Delay (us) [ " +
      std::to_string(m_i64StartUpDelay_us) + " ]\n" + "Number of Channels [ " +
      std::to_string(m_uNumChannels) + " ]\n" + "SNR [ " +
      std::to_string(m_fSNR_db) + " ]\n" + "ADC Mode [ " + m_strADCMode +
      " ]\n" + "Clock Mode [ " + m_strClockMode + " ]\n" +
      "Sinusoid Config: \n" + "SignalPower_dBm [ " +
      std::to_string(m_fSignalPower_dBm) + " ]\n" + "=========";

  PLOG_INFO << strInfo;
}

void SimulatorModule::ConfigureModule(
    nlohmann::json_abi_v3_11_2::json jsonConfig) {

  // Convert phases to radians
  m_vfChannelPhases_rad =
      CheckAndThrowJSON<std::vector<float>>(jsonConfig, "ChannelPhases_deg");
  std::transform(m_vfChannelPhases_rad.begin(), m_vfChannelPhases_rad.end(),
                 m_vfChannelPhases_rad.begin(),
                 [](float f) { return f * M_PI / 180; });

  // Used only in gaussian mode
  if (m_strADCMode == "Gaussian") {

    float stdDev = ConvertPowerToStdDev(m_fSignalPower_dBm);
    assert(stdDev >= 0 && "std dev of gaussian cannot be negative! ");
    std::normal_distribution<double> dist(0, stdDev);
    m_dist = dist;

  } else if (m_strADCMode == "Sinusoid") {

    auto fSignalPower_lin = std::pow(10, m_fSignalPower_dBm / 10);
    m_fAmplitudeScalingFactor = std::pow(fSignalPower_lin * 2, 0.5);

  } else {
    std::runtime_error(m_strADCMode + " must be either Gaussian or Sinusoid");
  }
}

void SimulatorModule::ContinuouslyTryProcess() {
  WaitForStartUpDelay();

  while (!m_bShutDown) {
    auto pBaseChunk = std::make_shared<BaseChunk>();
    Process(pBaseChunk);
  }
}

void SimulatorModule::Process(std::shared_ptr<BaseChunk> pBaseChunk) {
  // Creating simulated data
  ReinitializeTimeChunk();
  SimulateADCSamples();
  SimulateNoise();
  SimulateTimeStampMetaData();

  // Passing data on
  std::shared_ptr<TimeChunk> pTimeChunk = std::move(m_pTimeChunk);
  if (!TryPassChunk(std::static_pointer_cast<BaseChunk>(pTimeChunk))) {
    std::string strWarning =
        std::string(__FUNCTION__) +
        ": Next buffer full, dropping current chunk and passing \n";
    PLOG_WARNING << strWarning;
  }

  // Sleeping for time equivalent to chunk period
  std::this_thread::sleep_for(std::chrono::nanoseconds(
      (unsigned)((1e9 * m_dChunkSize) / m_dSampleRate)));
}

void SimulatorModule::ReinitializeTimeChunk() {
  m_pTimeChunk = std::make_shared<TimeChunk>(
      m_dChunkSize, m_dSampleRate, 0, sizeof(int16_t) * 8, sizeof(int16_t), 1);
  m_pTimeChunk->m_vvi16TimeChunks.resize(m_uNumChannels);

  // Current implementation simulated N channels on a single ADC
  for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++) {
    // Initialising channel data vector for each ADC
    m_pTimeChunk->m_vvi16TimeChunks[uChannel].resize(m_dChunkSize);
  }

  m_pTimeChunk->m_uNumChannels = m_uNumChannels;
}

void SimulatorModule::GenerateSinsoid() {

  // Iterate through each channel vector and sample index
  for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize;
       uCurrentSampleIndex++) {
    for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++) {
      double dSinArgument = (double)2.0 * M_PI *
                            ((double)m_uSimulatedFrequency *
                             (double)m_u64SampleCount / (double)m_dSampleRate);
      int16_t datum =
          m_fAmplitudeScalingFactor *
          sin(dSinArgument + (double)m_vfChannelPhases_rad[uChannel]);
      m_pTimeChunk->m_vvi16TimeChunks[uChannel][uCurrentSampleIndex] = datum;
    }
    m_u64SampleCount += 1;
  }
}

void SimulatorModule::GenerateGaussianNoise() {
  for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize;
       uCurrentSampleIndex++) {
    for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++) {
      // Generate a random sample using the gaussian distribution
      m_pTimeChunk->m_vvi16TimeChunks[uChannel][uCurrentSampleIndex] =
          m_dist(m_generator);
      m_count++;
    }
  }
}

void SimulatorModule::SimulateADCSamples() {
  if (m_strADCMode == "Sinusoid")
    GenerateSinsoid();
  else if (m_strADCMode == "Gaussian")
    GenerateGaussianNoise();
  else
    throw std::runtime_error("ADC Mode incorrectly specified as: " +
                             m_strADCMode);
}

void SimulatorModule::GenerateClockTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto i64CurrentTime_us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          now.time_since_epoch())
          .count();
  m_u64CurrentTimeStamp_us = i64CurrentTime_us;
}

void SimulatorModule::GenerateCountTimestamp() {
  uint64_t u64ChunkPeriod_us =
      uint64_t(m_dChunkSize * (1 / m_dSampleRate) * 1e6);
  m_u64CurrentTimeStamp_us = m_u64CurrentTimeStamp_us + u64ChunkPeriod_us;
}

void SimulatorModule::SimulateTimeStampMetaData() {
  // First set
  m_pTimeChunk->m_i64TimeStamp = m_u64CurrentTimeStamp_us;
  m_pTimeChunk->SetSourceIdentifier(m_vu8SourceIdentifier);

  if (m_strClockMode == "Clock")
    GenerateClockTimestamp();
  else if (m_strClockMode == "Counter")
    GenerateCountTimestamp();
  else
    throw std::runtime_error("Clock Mode incorrectly specified as: " +
                             m_strClockMode);
}

float SimulatorModule::ConvertPowerToStdDev(float fSignalPower_dBm) {
  PLOG_FATAL << std::to_string(m_fSignalPower_dBm * std::pow(10, -1 / 10));
  return std::pow(m_fSignalPower_dBm * std::pow(10, -1 / 10), 0.5);
}

void SimulatorModule::SimulateNoise() {
  // Noise signal is norm dist with power = stddev as defined above where we
  // back calcualte using snr and signal level
  float fSignalPower_mw = std::pow(10, m_fSignalPower_dBm / 10) / 1000;
  auto stdDevNoise =
      std::pow(fSignalPower_mw / std::pow(10, m_fSNR_db / 10), 0.5);

  // Now generate normal distribution wiht power to create noise
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::normal_distribution<double> dist(0, stdDevNoise);

  for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize;
       uCurrentSampleIndex++) {
    for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++) {
      double dNoiseValue = dist(generator);
      m_pTimeChunk->m_vvi16TimeChunks[uChannel][uCurrentSampleIndex] =
          m_pTimeChunk->m_vvi16TimeChunks[uChannel][uCurrentSampleIndex] +
          dNoiseValue;
    }
    m_u64SampleCount += 1;
  }
}

void SimulatorModule::WaitForStartUpDelay() {
  PLOG_INFO << "Sleeping on startup for us: " << m_i64StartUpDelay_us;

  if (!m_i64StartUpDelay_us)
    return;

  std::this_thread::sleep_for(
      std::chrono::nanoseconds(m_i64StartUpDelay_us * (uint32_t)1e3));
}

void SimulatorModule::StartReportingLoop() {
  while (!m_bShutDown) {

    // Lets start by generating Queue stat
    std::unique_lock<std::mutex> BufferAccessLock(m_BufferStateMutex);
    uint16_t u16CurrentBufferSize = m_cbBaseChunkBuffer.size();
    auto strModuleName = GetModuleType();
    BufferAccessLock.unlock();

    nlohmann::json j = {
        {m_strReportingJsonRoot,
         {{strModuleName + "_" + m_strReportingJsonModuleAddition,
           {// Extra `{}` around key-value pairs
            {"QueueLength", std::to_string(u16CurrentBufferSize)},
            {"ChannelCount", std::to_string(m_uNumChannels)},
            {"SampleRate_Hz", std::to_string(int(m_dSampleRate))},
            {"ChunkSize", std::to_string(int(m_dChunkSize))}}}}}};

    // Then transmit
    auto pJSONChunk = std::make_shared<JSONChunk>();
    pJSONChunk->m_JSONDocument = j;
    CallChunkCallbackFunction(pJSONChunk);

    // And sleep as not to send too many
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

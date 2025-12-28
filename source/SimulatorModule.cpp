#include "SimulatorModule.h"
#include <numbers>
#include <random>

SimulatorModule::SimulatorModule(unsigned uBufferSize,
                                 nlohmann::json_abi_v3_11_2::json jsonConfig)
    : BaseModule(uBufferSize) {
  ConfigureModuleJSON(jsonConfig);
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

void SimulatorModule::ConfigureModuleJSON(
    nlohmann::json_abi_v3_11_2::json jsonConfig) {
  m_dChunkSize = 512;

  CheckAndThrowJSON(jsonConfig, "SourceIdentifier");
  m_vu8SourceIdentifier =
      jsonConfig["SourceIdentifier"].get<std::vector<uint8_t>>();

  CheckAndThrowJSON(jsonConfig, "SampleRate_Hz");
  m_dSampleRate = jsonConfig["SampleRate_Hz"];

  CheckAndThrowJSON(jsonConfig, "SimulatedFrequency_Hz");
  m_uSimulatedFrequency = jsonConfig["SimulatedFrequency_Hz"];

  CheckAndThrowJSON(jsonConfig, "NumberOfChannels");
  m_uNumChannels = jsonConfig["NumberOfChannels"];

  CheckAndThrowJSON(jsonConfig, "StartupDelay_us");
  m_i64StartUpDelay_us = jsonConfig["StartupDelay_us"];
  m_u64CurrentTimeStamp_us = m_i64StartUpDelay_us;

  CheckAndThrowJSON(jsonConfig, "SNR_db");
  m_fSNR_db = jsonConfig["SNR_db"];

  CheckAndThrowJSON(jsonConfig, "ADCMode");
  m_strADCMode = jsonConfig["ADCMode"];

  CheckAndThrowJSON(jsonConfig, "ClockMode");
  m_strClockMode = jsonConfig["ClockMode"];

  CheckAndThrowJSON(jsonConfig, "SignalPower_dBm");
  m_fSignalPower_dBm = jsonConfig["SignalPower_dBm"];

  CheckAndThrowJSON(jsonConfig, "ChannelPhases_deg");
  m_vfChannelPhases_rad =
      jsonConfig["ChannelPhases_deg"].get<std::vector<float>>();
  std::transform(m_vfChannelPhases_rad.begin(), m_vfChannelPhases_rad.end(),
                 m_vfChannelPhases_rad.begin(),
                 [](float f) { return f * M_PI / 180; });

  if (m_vfChannelPhases_rad.size() != m_uNumChannels) {
    std::string strFatal = std::string(__FUNCTION__) +
                           "Channel phase count does not equal channel count";
    PLOG_FATAL << strFatal;
    throw std::runtime_error(strFatal);
  }

  m_vfChannelPhases_rad.resize(m_uNumChannels);

  // Used only in gaussian mode
  auto stdDev = ConvertPowerToStdDev(m_fSignalPower_dBm);
  std::normal_distribution<double> dist(0, stdDev);
  m_dist = dist;
}

void SimulatorModule::ContinuouslyTryProcess() {
  WaitForStartUpDelay();

  while (!m_bShutDown) {
    auto pBaseChunk = std::make_shared<BaseChunk>();
    Process(pBaseChunk);
  }
}

void SimulatorModule::SetChannelPhases(std::vector<float> vfChannelPhases_deg) {
  assert(vfChannelPhases_deg.size() == m_uNumChannels);

  std::transform(vfChannelPhases_deg.begin(), vfChannelPhases_deg.end(),
                 vfChannelPhases_deg.begin(),
                 [](double d) { return d * std::numbers::pi / 180; });

  m_vfChannelPhases_rad = vfChannelPhases_deg;
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
  auto m_fSignalPower_lin = std::pow(10, m_fSignalPower_dBm / 10);
  float fAmplitudeScalingFactor = std::pow(m_fSignalPower_lin * 2, 0.5);

  // Iterate through each channel vector and sample index
  for (unsigned uCurrentSampleIndex = 0; uCurrentSampleIndex < m_dChunkSize;
       uCurrentSampleIndex++) {
    for (unsigned uChannel = 0; uChannel < m_uNumChannels; uChannel++) {
      double dSinArgument = (double)2.0 * M_PI *
                            ((double)m_uSimulatedFrequency *
                             (double)m_u64SampleCount / (double)m_dSampleRate);
      int16_t datum =
          fAmplitudeScalingFactor *
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
  return std::pow(m_fSignalPower_dBm * std::pow(10, -1 / 10), 0.5);
}

void SimulatorModule::SimulateNoise() {
  // Noise signal is norm dist with power = stddev as defined above where we
  // back calcualte using snr and signal level
  auto stdDevNoise =
      std::pow(m_fSignalPower_dBm * std::pow(10, -m_fSNR_db / 10), 0.5);

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

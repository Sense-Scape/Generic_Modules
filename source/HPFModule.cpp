#include "HPFModule.h"

HPFModule::HPFModule(unsigned uBufferSize, double dCutOffFrequency, double dSamplingRate, unsigned uFilterOrder) :
	BaseModule(uBufferSize),
	m_vdInputCoefficients(),
	m_vdOuputCoefficients(),
	m_dCutOffFrequency(dCutOffFrequency),
	m_dSamplingRate(dSamplingRate),
	m_uFilterOrder(uFilterOrder)
{
	CalculateButterWorthCoefficients();
}

void HPFModule::CalculateButterWorthCoefficients()
{
	// TODO: create generic filter order
	double dPi = 3.14159265358979323846;

	m_uFilterOrder = 1;

	m_vdInputCoefficients.resize(m_uFilterOrder+1);
	m_vdOuputCoefficients.resize(m_uFilterOrder+1);

	m_vdInputCoefficients[0] = 1;
	m_vdInputCoefficients[1] = -1;
	m_vdOuputCoefficients[0] = 0.995;
	m_vdOuputCoefficients[1] = 0;

}

void HPFModule::ApplyFilter(std::vector<uint16_t>& vfData)
{
	// Creating arrays to store previosuly processed values
	std::vector<uint16_t> vfOutputData;
	vfOutputData.resize(vfData.size());

	// Applying Filter
	for (int uSampleIndex = 0; uSampleIndex < vfData.size(); uSampleIndex++)
	{
		double y = 0;
		for (int j = 0; j <= m_uFilterOrder; j++) {
			if ((uSampleIndex - j) >= 0) {
				y += m_vdInputCoefficients[j] * vfData[uSampleIndex - j] + m_vdOuputCoefficients[j] * vfOutputData[uSampleIndex - j];
			}
		}
		vfOutputData[uSampleIndex] = y;
	}

	vfData = vfOutputData;
}

void HPFModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	//auto pWAVChunk = std::static_pointer_cast<WAVChunk>(pBaseChunk);

	// Inplace operation of chunk data
	// ApplyFilter(pWAVChunk->m_vfData);

	// Try pass on
	//TryPassChunk(pWAVChunk);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout << "HPFModule remains unimplemented" << std::endl;
}
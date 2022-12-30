#ifndef HIGH_PASS_FILTER_MODULE
#define HIGH_PASS_FILTER_MODULE

#include "BaseModule.h"
#include <cmath>

/*
* @brief Module process that will take data and low pass filter it
*/
class HPFModule :
    public BaseModule
{
public:
    /**
     * @brief Construct a new Router Module forward data into different pipeline streams.
     * @param uBufferSize size of processing input buffer
     */
    HPFModule(unsigned uBufferSize, double dCutOffFrequency, double dSamplingRate, unsigned uFilterOrder);
    ~HPFModule() {};

    ///**
    // * @brief Returns module type
    // * @return ModuleType of processing module
    // */
    ModuleType GetModuleType() override { return ModuleType::HPFModule; };

private:
    std::vector<double> m_vdInputCoefficients;  ///< Difference equatio input coefficient values (b[n])
    std::vector<double> m_vdOuputCoefficients;  ///< Difference equatio output coefficient values (a[n])
    double m_dCutOffFrequency;                  ///< Desired filter HPFF cure off frequency
    double m_dSamplingRate;                     ///< Data sampling rate
    double m_uFilterOrder;                      ///< Desired order of filter

    /*
     * @brief Module process to collect and format UDP data
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

    /*
     * @brief Creates DC blocking filter
     */
    void CalculateButterWorthCoefficients();

    /*
    * @brief Applies the filter using difference equation onto data vector
    */
    void ApplyFilter(std::vector<float> &vfData);

};

#endif

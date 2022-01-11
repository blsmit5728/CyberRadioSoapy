#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Logger.hpp>
#include <LibCyberRadio/Driver/RadioHandler.h>
#include <LibCyberRadio/Driver/Driver.h>
#include <iostream>


struct cyberradio_devinfo {
    std::string host;
    std::string radio;
    bool verbose;
};

/***********************************************************************
 * Device interface
 **********************************************************************/
class CyberRadioSoapy : public SoapySDR::Device
{
public:
    //Implement constructor with device specific arguments...
    CyberRadioSoapy( const std::string host, const std::string radio, bool verbose=false );
    //Implement all applicable virtual methods from SoapySDR::Device
    std::string getDriverKey(void) const
    {
        return "cyberradio";
    }
    /*******************************************************************
    * Identification API
    ******************************************************************/
    std::string getHardwareKey(void) const;
    SoapySDR::Kwargs getHardwareInfo(void) const;
    /*******************************************************************
    * Channels API
    ******************************************************************/
    size_t getNumChannels(const int) const;
    bool getFullDuplex(const int direction, const size_t channel) const;
    /*******************************************************************
    * Freq API
    ******************************************************************/
    void setFrequency(const int direction, const size_t channel, 
                      const std::string &name, const double frequency, 
                      const SoapySDR::Kwargs &);
    double getFrequency(const int direction, 
                        const size_t channel, const std::string &name) const;
    SoapySDR::RangeList getFrequencyRange(const int direction, 
                                          const size_t channel, 
                                          const std::string &name) const;

    /*******************************************************************
    * Sample Rate API
    ******************************************************************/
    void setSampleRate(const int direction, const size_t channel, const double rate);
    double getSampleRate(const int direction, const size_t channel) const;
    SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const;

private:
    std::shared_ptr<LibCyberRadio::Driver::RadioHandler> _handler;
    LibCyberRadio::Driver::ConfigurationDict _cfgDict;
};
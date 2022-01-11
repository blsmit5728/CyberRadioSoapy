#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <LibCyberRadio/Driver/RadioHandler.h>
#include <LibCyberRadio/Driver/Driver.h>
#include <iostream>


struct cyberradio_devinfo {
    std::string host;
    std::string radio;
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
                        const size_t channel, const std::string &name);
    SoapySDR::RangeList getFrequencyRange(const int direction, 
                                          const size_t channel, 
                                          const std::string &name);

private:
    std::shared_ptr<LibCyberRadio::Driver::RadioHandler> _handler;
    LibCyberRadio::Driver::ConfigurationDict _cfgDict;
};
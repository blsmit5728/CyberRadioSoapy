#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Logger.hpp>
#include <LibCyberRadio/Driver/RadioHandler.h>
#include <LibCyberRadio/Driver/Driver.h>
#include <iostream>
#include <tuple>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netpacket/packet.h>


struct cyberradio_devinfo {
    std::string host;
    std::string radio;
    bool verbose;
    std::vector<std::string> streamInterfaces;
};

struct cyberradio_channel_info {
    double rate;
    int rateIndex;
    int linkIndex;
    int destIndex;
    int nbddcIndex;
    int wbddcIndex;
    int tunerIndex;
    std::string hostStreamInterface;
    std::string hostStreamIp;
    std::string sourceStreamIp;
    std::string hostStreamMac;
};

/***********************************************************************
 * Device interface
 **********************************************************************/
class CyberRadioSoapy : public SoapySDR::Device
{
public:
    //Implement constructor with device specific arguments...
    CyberRadioSoapy( const std::string host, const std::string radio, 
                     std::vector<std::string>& interfaces, bool verbose=false );
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
    SoapySDR::Stream* setupStream ( const int direction, 
                        const std::string & format, 
                        const std::vector< size_t >& channels,
                        const SoapySDR::Kwargs & args) override;
private:
    std::shared_ptr<LibCyberRadio::Driver::RadioHandler> _handler;
    LibCyberRadio::Driver::ConfigurationDict _cfgDict;
    std::map<int, double> _globalRates;
    void dumpConfig(const LibCyberRadio::Driver::ConfigurationDict& cfg);
    std::vector<LibCyberRadio::Driver::ConfigurationDict> tunerCfgVector;
    std::vector<LibCyberRadio::Driver::ConfigurationDict> wbddcCfgVector;
    std::vector<LibCyberRadio::Driver::ConfigurationDict> nbddcCfgVector;
    std::vector<cyberradio_channel_info> _channelInfoVector;
    void collectGlobalRates();
    std::vector<std::string> _streamingInterfaces;
    std::string _host;
    std::string _radio;
    bool _verbose;
    std::tuple<std::string,std::string,std::string> getIPAddress(std::string interface);
    std::string createSourceIPAddress( std::string input );
    std::vector<std::tuple<std::string,std::string,std::string>> _streamingMap;
};
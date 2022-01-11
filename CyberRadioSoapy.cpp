#include "CyberRadioSoapy.hpp"

#define UNUSED(x) (void)(x)

static cyberradio_devinfo kwargs_to_devinfo(const SoapySDR::Kwargs &args)
{
    std::cout << "-- kwargs_to_devinfo" << std::endl;
    cyberradio_devinfo info;
    info.host = args.at("host");
    info.radio = args.at("radio");
    std::cout << " -- " << info.host << " : " << info.radio << std::endl;
    return info;
}

/***********************************************************************
 * Find available devices
 **********************************************************************/
SoapySDR::KwargsList findCyberRadioSoapy(const SoapySDR::Kwargs &args)
{
    std::cout << "-- findCyberRadioSoapy" << std::endl;
    //locate the device on the system...
    //return a list of 0, 1, or more argument maps that each identify a device
    SoapySDR::KwargsList results;
    const cyberradio_devinfo info = kwargs_to_devinfo( args );
    SoapySDR::Kwargs A;
    A.insert(std::pair<std::string, std::string>("host","192.168.0.10"));
    results.push_back(A);
    return results;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeCyberRadioSoapy(const SoapySDR::Kwargs &args)
{
    //create an instance of the device object given the args
    //here we will translate args into something used in the constructor
    std::cout << "-- makeCyberRadioSoapy" << std::endl;
    cyberradio_devinfo info = kwargs_to_devinfo( args );
    return new CyberRadioSoapy( info.host, info.radio );
}

CyberRadioSoapy::CyberRadioSoapy( const std::string host, const std::string radio, bool verbose)
{
    _handler = LibCyberRadio::Driver::getRadioObject( radio, host, -1, verbose );
    if ( (_handler != NULL) && _handler->isConnected() ) {
        std::cout << "-- Connect SUCCESS" << std::endl;
    } else {
        _handler = NULL;
    }
    LibCyberRadio::BasicStringStringDict versionInfo = _handler->getVersionInfo();
    std::cout << " -- Serial Number : " << versionInfo["serialNumber"] << std::endl;
    std::cout << " -- Model         : " << versionInfo["model"] << std::endl;
    std::cout << " -- Software Ver  : " << versionInfo["softwareVersion"] << std::endl;
    std::cout << " -- Firmware Ver  : " << versionInfo["firmwareVersion"] << std::endl;
    std::cout << " -- Unit Revision : " << versionInfo["unitRevision"] << std::endl;
    std::cout << " -- HW Revision   : " << versionInfo["hardwareVersion"] << std::endl;
}

/*******************************************************************
 * Identification API
 ******************************************************************/
std::string CyberRadioSoapy::getHardwareKey(void) const
{
    return "NDR358";
}

SoapySDR::Kwargs CyberRadioSoapy::getHardwareInfo(void) const
{
    SoapySDR::Kwargs info;
    LibCyberRadio::BasicStringStringDict versionInfo = this->_handler->getVersionInfo();
    info["serial"] = versionInfo["serialNumber"];
    info["sw_version"] = versionInfo["softwareVersion"];
    return info;
}

/*******************************************************************
 * Channels API
 ******************************************************************/

size_t CyberRadioSoapy::getNumChannels(const int direction) const
{
    size_t channels = 0;
    if ( direction == SOAPY_SDR_RX ) {
        channels = _handler->getNumTuner();
    }
    return channels;
}

bool CyberRadioSoapy::getFullDuplex(const int direction, const size_t channel) const
{
    UNUSED(direction);
    UNUSED(channel);
    return false;
}

/*******************************************************************
 * Frequency API
 ******************************************************************/

void CyberRadioSoapy::setFrequency(const int direction, const size_t channel, 
                                   const std::string &name, const double frequency, 
                                   const SoapySDR::Kwargs &)
{
    UNUSED(name);
    if( direction == SOAPY_SDR_RX ){
        _handler->setTunerFrequency( channel, frequency );
    }
}

double CyberRadioSoapy::getFrequency(const int direction, 
                                      const size_t channel, const std::string &name)
{
    UNUSED(name);
    double freq = 0;
    if( direction == SOAPY_SDR_RX ){
        freq = _handler->getTunerFrequency( channel );
    }
    return freq;
}

SoapySDR::RangeList CyberRadioSoapy::getFrequencyRange(const int direction, 
                                                       const size_t channel, 
                                                       const std::string &name)
{
    UNUSED(channel);
    UNUSED(name);
    if( direction == SOAPY_SDR_RX ){
        LibCyberRadio::BasicDoubleList L = _handler->getTunerFrequencyRange( );
    }
}                                                       



/*******************************************************************
 * Stream API
 ******************************************************************/

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerCyberRadioSoapy("cyberradiosoapy", &findCyberRadioSoapy, &makeCyberRadioSoapy, SOAPY_SDR_ABI_VERSION);

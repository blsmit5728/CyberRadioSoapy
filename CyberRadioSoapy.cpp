#include "CyberRadioSoapy.hpp"

#define UNUSED(x) (void)(x)

static cyberradio_devinfo kwargs_to_devinfo(const SoapySDR::Kwargs &args)
{
    cyberradio_devinfo info;
    info.host = args.at("host");
    info.radio = args.at("radio");
    std::istringstream f(args.at("streamif").c_str());
    std::string s;
    while (getline(f, s, ':')) {
        info.streamInterfaces.push_back(s);
    }
    if ( args.at("verbose") == "true" ) {
        info.verbose = true;
    } else {
        info.verbose = false;
    }
    return info;
}

/***********************************************************************
 * Find available devices
 **********************************************************************/
SoapySDR::KwargsList findCyberRadioSoapy(const SoapySDR::Kwargs &args)
{
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
    cyberradio_devinfo info = kwargs_to_devinfo( args );
    return new CyberRadioSoapy( info.host, info.radio, info.streamInterfaces, info.verbose );
}

CyberRadioSoapy::CyberRadioSoapy( const std::string host, 
                const std::string radio, std::vector<std::string>& interfaces,
                bool verbose) :
    _streamingInterfaces( interfaces ),
    _host( host ),
    _radio( radio ),
    _verbose( verbose)
{
    _handler = LibCyberRadio::Driver::getRadioObject( _radio, _host, -1, _verbose );
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
    _cfgDict = _handler->getConfiguration();
    //this->dumpConfig(_cfgDict);
    for (int i = 0; i < _handler->getNumTuner(); i++)
    {
        tunerCfgVector.push_back(_handler->getTunerConfiguration(i));
    }
    //this->dumpConfig(tunerCfgVector.at(0));
    for (int i = 0; i < _handler->getNumWbddc(); i++)
    {
        wbddcCfgVector.push_back(_handler->getWbddcConfiguration(i));
    }
    //this->dumpConfig(wbddcCfgVector.at(0));
    for (int i = 0; i < _handler->getNumNbddc(); i++)
    {
        nbddcCfgVector.push_back(_handler->getNbddcConfiguration(i));
    }
    //this->dumpConfig(nbddcCfgVector.at(0));

    for( auto & element : _streamingInterfaces )
    {
        std::tuple<std::string,std::string,std::string> ip = this->getIPAddress( element );
        _streamingMap.push_back(ip);
        //std::cout << element << " ---> " << 
        //             std::get<0>(ip) << ":" << 
        //             std::get<1>(ip) << ":" << 
        //             std::get<2>(ip) << std::endl;   
    }
    for( int i = 0; i < _handler->getNumTuner(); i++)
    {
        struct cyberradio_channel_info info;
        info.destIndex = 0;
        info.linkIndex = i % 4;
        info.nbddcIndex = i * 16;
        info.wbddcIndex = i;
        info.tunerIndex = i;
        info.rate = 128e6;
        info.rateIndex = 40;
        info.hostStreamInterface = _streamingInterfaces.at(i%4);
        info.hostStreamIp = std::get<1>(_streamingMap.at(i%4));
        info.hostStreamMac = std::get<2>(_streamingMap.at(i%4));
        info.sourceStreamIp = this->createSourceIPAddress(info.hostStreamIp);
        _channelInfoVector.push_back(info);
        _handler->setWbddcSource( info.wbddcIndex, info.tunerIndex );
        std::cout << info.tunerIndex << " : " 
                  << info.hostStreamInterface << "," 
                  << info.hostStreamIp << "," 
                  << info.hostStreamMac << ","
                  << info.sourceStreamIp << std::endl;
        _handler->enableTuner( i, true);
    } 
    // Setup the CFGE10G's
    int udpPortBase = 10000;
    for (int i = 0; i < _handler->getNumDataPorts(); i++)
    {
        int currPort = udpPortBase;
        _handler->setDataPortSourceIP(i, _channelInfoVector.at(i).sourceStreamIp);
        int dests = _handler->getNumDataPortDipEntries();
        for (int j = 0;  j < 10; j++)
        {
            currPort = udpPortBase + (1000 * i) + j; // 10000,10001..11000,11001 etc.
            _handler->setDataPortDestInfo(i, j,
                    _channelInfoVector.at(i).hostStreamIp,
                    _channelInfoVector.at(i).hostStreamMac,
                    currPort,
                    currPort);
        }

    }
    this->collectGlobalRates();
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
                                      const size_t channel, const std::string &name) const
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
                                                       const std::string &name) const
{
    SoapySDR::RangeList R;
    std::cout << "getFrequencyRange" <<std::endl;
    UNUSED(channel);
    UNUSED(name);
    LibCyberRadio::BasicDoubleList L;
    if( direction == SOAPY_SDR_RX ){
        L = _handler->getTunerFrequencyRange( );
        for( size_t i = 0; i < L.size(); i++ ){
            std::cout << L.at(i) << std::endl;
            SoapySDR::Range e( L.at(i), L.at(i) );
            R.push_back( e );
        }
    }
    for(double n : L)
    {
        std::cout << n << "\n";
    }
    return R;
}                                                       

/*******************************************************************
* Stream API
******************************************************************/

void CyberRadioSoapy::collectGlobalRates( void )
{
    std::map<int, double>::iterator it;
    std::map<int, double> w = _handler->getWbddcRateSet();
    for( it = w.begin(); it != w.end(); it++)
    {
        _globalRates.insert(std::pair<int, double>(it->first, it->second));
    }
    std::map<int, double> n = _handler->getNbddcRateSet();
    for( it = n.begin(); it != n.end(); it++)
    {
        _globalRates.insert(std::pair<int, double>(it->first, it->second));
    }
}

void CyberRadioSoapy::setSampleRate(const int direction, const size_t channel, const double rate)
{
    int rateIndex = -1;
    std::map<int, double>::iterator it;
    for( it = _globalRates.begin(); it != _globalRates.end(); it++)
    {
        if( it->second == rate )
        {
            rateIndex = it->first;
            break;
        }
    }
    if( rateIndex != -1 )
    {
        if( rateIndex <= 15 ){
            _handler->setNbddcRateIndex( channel, rateIndex );
        } else {
            _handler->setWbddcRateIndex( channel, rateIndex );
        }
        _channelInfoVector.at(channel).rate = rate;
        _channelInfoVector.at(channel).rateIndex = rateIndex;
    }
}

double CyberRadioSoapy::getSampleRate(const int direction, const size_t channel) const
{
    double S = 0;
    int rInd = _channelInfoVector.at(channel).rateIndex;
    if( rInd <= 15 ){
        S = _globalRates.at(_handler->getNbddcRateIndex(channel));
    } else {
        S = _globalRates.at(_handler->getWbddcRateIndex(channel));
    }
    //r = r + w;
    //r = r + n;
    return S;
}
SoapySDR::RangeList CyberRadioSoapy::getSampleRateRange(const int direction, const size_t channel) const
{
    UNUSED(channel);
    SoapySDR::RangeList R;
    
    LibCyberRadio::BasicDoubleList L;
    if( direction == SOAPY_SDR_RX ){
        L = _handler->getNbddcRateList();
        for( size_t i = 0; i < L.size(); i++ ){
            //std::cout << L.at(i) << std::endl;
            SoapySDR::Range e( L.at(i), L.at(i) );
            R.push_back( e );
        }
        L = _handler->getWbddcRateList();
        for( size_t i = 0; i < L.size(); i++ ){
            //std::cout << L.at(i) << std::endl;
            SoapySDR::Range e( L.at(i), L.at(i) );
            R.push_back( e );
        }
        
    }
    return R;
}

SoapySDR::Stream* CyberRadioSoapy::setupStream ( const int direction, 
                                       const std::string & format, 
                                       const std::vector< size_t >& channels,
                                       const SoapySDR::Kwargs& args)
{

    SoapySDR::Stream * h;
    return h;
}

/**
 * \brief Dumps a configuration dictionary to standard output.
 * \param cfg Configuration dictionary.
 */
void CyberRadioSoapy::dumpConfig(const LibCyberRadio::Driver::ConfigurationDict& cfg)
{
    std::cout << "    Configuration(" << std::endl;
    LibCyberRadio::Driver::ConfigurationDict::const_iterator it;
    for ( it = cfg.begin(); it != cfg.end(); it++)
    {
        std::cout << "        " << it->first << " = " << it->second << std::endl;
    }
    std::cout << "    )" << std::endl;
}

std::tuple<std::string,std::string,std::string> 
    CyberRadioSoapy::getIPAddress(std::string interface)
{
    std::string ipAddress="Unable to get IP Address";
    std::string macAddress="Unable to get Mac Address";
//    std::pair<std::string, std::string> ip_pair("none", "none");
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *ifa = NULL;
    int success = 0;
    char buffer[20];
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        for ( ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next)
        {
            if( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_INET) ) {
                if( strstr(ifa->ifa_name, interface.c_str()) != NULL ){
                    ipAddress=inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr);
                }
            }
            if( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET) ) {
                if( strstr(ifa->ifa_name, interface.c_str()) != NULL ){
                    struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
                    snprintf(&buffer[0], 20, "%02X:%02X:%02X:%02X:%02X:%02X",
                             s->sll_addr[0],
                             s->sll_addr[1],
                             s->sll_addr[2],
                             s->sll_addr[3],
                             s->sll_addr[4],
                             s->sll_addr[5]);
                    macAddress=buffer;
                    //for (i=0; i <s->sll_halen; i++)
                    //{
                    //    printf("%02x%c", (s->sll_addr[i]), (i+1!=s->sll_halen)?':':'\n');
                    //}
                }
            }
        }
        //temp_addr = interfaces;
        //while(temp_addr != NULL) {
        //    if(temp_addr->ifa_addr->sa_family == AF_INET) {
        //        if( strstr(temp_addr->ifa_name, interface.c_str()) != NULL ){
        //            ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
        //        }
        //    }
        //    
        //    temp_addr = temp_addr->ifa_next;
        //}
    }
    
    // Free memory
    freeifaddrs(interfaces);
    return std::tie(interface, ipAddress, macAddress);
    //return ipAddress;
}

std::string CyberRadioSoapy::createSourceIPAddress( std::string input )
{
    std::vector<std::string> strings;
    std::istringstream inputStream(input);
    std::string ret;
    std::string s;    
    while (getline(inputStream, s, '.')) {
        //std::cout << s << std::endl;
        strings.push_back(s);
    }
    int c = 0;
    for( auto element : strings )
    {
        int octet = std::stoi(element);
        if (c == 3)
        {
            octet = 100;
        }
        ret.append(std::to_string(octet));
        if( c != 3 )
        {
            ret.push_back('.');
        }
        c++;
    }
    return ret;
}

/***********************************************************************
* Registration
**********************************************************************/
static SoapySDR::Registry registerCyberRadioSoapy("cyberradiosoapy", &findCyberRadioSoapy, &makeCyberRadioSoapy, SOAPY_SDR_ABI_VERSION);



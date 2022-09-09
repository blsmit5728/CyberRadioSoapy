#include "CyberRadioSoapy.hpp"

#define UNUSED(x) (void)(x)

static cyberradio_devinfo kwargs_to_devinfo(const SoapySDR::Kwargs &args)
{
    for( auto & it : args)
    {
        SoapySDR::logf(SOAPY_SDR_DEBUG, "[\"%s\"] -> %s",it.first.c_str(), it.second.c_str());
    }
    cyberradio_devinfo info;
    try{
        info.host = args.at("host");
    } catch ( const std::exception &ex ) {
        info.host = "192.168.0.10";
    }
    info.radio = args.at("radio");
    if( info.radio == "ndr324" )
    {
        info.vitaType = 324;
    }
    try
    {
        std::istringstream f(args.at("streamif").c_str());
        std::string s;
        while (getline(f, s, ':')) {
            info.streamInterfaces.push_back(s);
        }
    } catch ( const std::exception &ex ) {
        SoapySDR::log(SOAPY_SDR_NOTICE, "[CyberRadioSoapy] streamif not provided, defaulting");
        // max of 4 to satisfy all radios.
        info.streamInterfaces.push_back("ethNull");
        info.streamInterfaces.push_back("ethNull");
        info.streamInterfaces.push_back("ethNull");
        info.streamInterfaces.push_back("ethNull");
    }
    try {
        if ( args.at("verbose") == "true" ) {
            info.verbose = true;
        } else {
            info.verbose = false;
        }
    } catch ( const std::exception &ex ) {
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
    std::vector<SoapySDR::Kwargs> results;
    std::vector<std::string> crdResults = LibCyberRadio::Driver::getSupportedDevices();
    if( args.empty() ) {
        for( auto & it : crdResults ){
            SoapySDR::Kwargs devInfo;
            devInfo["label"] = it.c_str();
            devInfo["radio"] = it.c_str();
            //devInfo["streamif"] = "eth10:eth11:eth12:eth13";
            //devInfo["verbose"] = "false";
            results.push_back(devInfo);
        }
    } else {
        results.push_back(args);
    }
    return results;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeCyberRadioSoapy(const SoapySDR::Kwargs &args)
{
    SoapySDR::log(SOAPY_SDR_DEBUG, "Making CyberRadio");
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
    SoapySDR::logf(SOAPY_SDR_NOTICE, "[CyberRadioSoapy] Constructor (%s, %s, -1)",_radio.c_str(), _host.c_str());
    _handler = LibCyberRadio::Driver::getRadioObject( _radio, _host, 19091, true );
    //_handler->connect()
    if ( (_handler != NULL) && _handler->isConnected() ) {
        SoapySDR::log(SOAPY_SDR_NOTICE, "[CyberRadioSoapy] LibCyberRadio Connect SUCCESS");
    } else if ( (_handler != NULL) && !_handler->isConnected() ) {
        SoapySDR::log(SOAPY_SDR_ERROR, "[CyberRadioSoapy] LibCyberRadio Connect Error!");
        _handler = NULL;
    } else {
        SoapySDR::log(SOAPY_SDR_ERROR, "[CyberRadioSoapy] LibCyberRadio Connect Error!");
        _handler = NULL;
    }

    if (_handler != NULL )
    {

        LibCyberRadio::BasicStringStringDict versionInfo = _handler->getVersionInfo();
        SoapySDR::logf(SOAPY_SDR_DEBUG, " -- Serial Number : %s", versionInfo["serialNumber"].c_str());
        SoapySDR::logf(SOAPY_SDR_DEBUG, " -- Model         : %s", versionInfo["model"].c_str());
        SoapySDR::logf(SOAPY_SDR_DEBUG, " -- Software Ver  : %s", versionInfo["softwareVersion"].c_str());
        SoapySDR::logf(SOAPY_SDR_DEBUG, " -- Firmware Ver  : %s", versionInfo["firmwareVersion"].c_str());
        SoapySDR::logf(SOAPY_SDR_DEBUG, " -- Unit Revision : %s", versionInfo["unitRevision"].c_str());
        SoapySDR::logf(SOAPY_SDR_DEBUG, " -- HW Revision   : %s", versionInfo["hardwareVersion"].c_str());
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
            if( element != "ethNull") {
                std::tuple<std::string,std::string,std::string> ip = this->getIPAddress( element );
                _streamingMap.push_back(ip);
            } else {
                _streamingMap.push_back(std::tuple<std::string,std::string,std::string>("ethNull","0.0.0.0","00:00:00:00:00:00"));
            }
            //std::cout << element << " ---> " << 
            //             std::get<0>(ip) << ":" << 
            //             std::get<1>(ip) << ":" << 
            //             std::get<2>(ip) << std::endl;   
        }
        int dataPorts = _handler->getNumDataPorts();
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
            info.hostStreamInterface = _streamingInterfaces.at( i % dataPorts);
            try {
                info.hostStreamIp = std::get<1>(_streamingMap.at( i % dataPorts));
                info.hostStreamMac = std::get<2>(_streamingMap.at( i % dataPorts));
                info.sourceStreamIp = this->createSourceIPAddress(info.hostStreamIp);
            } catch ( const std::exception &ex ) {
                info.hostStreamIp = "0.0.0.0";
                info.hostStreamMac = "00:00:00:00:00:00";
                info.sourceStreamIp = "1.0.0.0";
            }
            _channelInfoVector.push_back(info);
            _handler->setWbddcSource( info.wbddcIndex, info.tunerIndex );
            SoapySDR::logf(SOAPY_SDR_NOTICE, "[CyberRadioSoapy] %d %s %s %s %s", 
                            info.tunerIndex,
                            info.hostStreamInterface.c_str(),
                            info.hostStreamIp.c_str(),
                            info.hostStreamMac.c_str(),
                            info.sourceStreamIp.c_str());
            _handler->enableTuner( i, true);
        } 
        // Setup the CFGE10G's
        int udpPortBase = 10000;
        for (int i = 0; i < _handler->getNumDataPorts(); i++)
        {
            int currPort = udpPortBase;
            if( _channelInfoVector.at(i).hostStreamInterface != "ethNull" )
            {
                _handler->setDataPortSourceIP(i, _channelInfoVector.at(i).sourceStreamIp);
                int dests = _handler->getNumDataPortDipEntries();
                for (int j = 0;  j < dests; j++)
                {
                    currPort = udpPortBase + (1000 * i) + j; // 10000,10001..11000,11001 etc.
                    _handler->setDataPortDestInfo(i, j,
                            _channelInfoVector.at(i).hostStreamIp,
                            _channelInfoVector.at(i).hostStreamMac,
                            currPort,
                            currPort);
                    _channelInfoVector.at(i).destPorts.push_back(currPort);
                }
            }
        }
        this->collectGlobalRates();
    }
}

CyberRadioSoapy::~CyberRadioSoapy()
{
    SoapySDR::log(SOAPY_SDR_NOTICE, "Destructor Called");
    for( int i = 0; i < _handler->getNumWbddc(); i++)
    {
        if ( _handler->isWbddcEnabled(i) )
        {
            _handler->enableWbddc(i, false);
        }
    }
}

/*******************************************************************
 * Identification API
 ******************************************************************/
std::string CyberRadioSoapy::getHardwareKey(void) const
{
    LibCyberRadio::BasicStringStringDict versionInfo = this->_handler->getVersionInfo();
    return versionInfo["model"];
}

SoapySDR::Kwargs CyberRadioSoapy::getHardwareInfo(void) const
{
    SoapySDR::Kwargs info;
    LibCyberRadio::BasicStringStringDict versionInfo = this->_handler->getVersionInfo();
    info["serial"] = versionInfo["serialNumber"];
    info["sw_version"] = versionInfo["softwareVersion"];
    info["model"] = versionInfo["model"];
    return info;
}

std::string SoapySDR::Device::getDriverKey(void) const
{
    return "CyberRadio";
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
                                                       const size_t channel ) const
{
    SoapySDR::RangeList R;
    std::cout << "getFrequencyRange" <<std::endl;
    UNUSED(channel);
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
    UNUSED(direction);
    int rateIndex = -1;
    std::map<int, double>::iterator it;
    for( it = _globalRates.begin(); it != _globalRates.end(); it++)
    {
        SoapySDR::logf(SOAPY_SDR_NOTICE, "Rate: %f <-> %f", it->second, rate );
        if( it->second == rate )
        {
            rateIndex = it->first;
            SoapySDR::logf(SOAPY_SDR_NOTICE, "RateIndex = %d", rateIndex);
            break;
        }
    }
    if( rateIndex != -1 )
    {
        if( _radio == "ndr358" || _radio == "ndr551" ){
            if( rateIndex <= 15 ){
                _handler->setNbddcRateIndex( channel, rateIndex );
            } else {
                _handler->setWbddcRateIndex( channel, rateIndex );
            }
        }
        else {
            _handler->setWbddcRateIndex( channel, rateIndex );
        }
        _channelInfoVector.at(channel).rate = rate;
        _channelInfoVector.at(channel).rateIndex = rateIndex;
    }
}

double CyberRadioSoapy::getSampleRate(const int direction, const size_t channel) const
{
    UNUSED(direction);
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

std::vector<double> CyberRadioSoapy::listSampleRates(const int direction, const size_t channel) const
{
    UNUSED(channel);
    std::vector<double> rates;
    LibCyberRadio::BasicDoubleList L;
    if( direction == SOAPY_SDR_RX ){
        L = _handler->getNbddcRateList();
        for( size_t i = 0; i < L.size(); i++ ){
            rates.push_back( L.at(i) );
        }
        L = _handler->getWbddcRateList();
        for( size_t i = 0; i < L.size(); i++ ){
            rates.push_back( L.at(i) );
        }
        
    }
    return rates;
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
    UNUSED(format);
    LibCyberRadio::VitaIqSource * stream;
    if( direction == SOAPY_SDR_RX ) {
        // only process first channel.
        int dest_port = _channelInfoVector.at(0).destPorts.at(0);
        stream = new LibCyberRadio::VitaIqSource("NDRIQ", 
                    324, 
                    _handler->getVitaPayloadSize(), 
                    _handler->getVitaHeaderSize(),
                    _handler->getVitaTailSize(), 
                    true, false, "0.0.0.0", dest_port, _verbose);
    } else {
        return reinterpret_cast<SoapySDR::Stream *>(NULL);
    }
    return reinterpret_cast<SoapySDR::Stream *>(stream);
}

int CyberRadioSoapy::activateStream(SoapySDR::Stream *handle, 
                                    const int flags, 
                                    const long long timeNs, 
                                    const size_t numElems)
{
    SoapySDR::logf(SOAPY_SDR_NOTICE, "Stream Activated");
    _handler->enableWbddc(0, true);
}

int CyberRadioSoapy::readStream(SoapySDR::Stream *handle, void * const *buffs, 
                           const size_t numElems, 
                           int &flags, 
                           long long &timeNs, 
                           const long timeoutUs)
{
    SoapySDR::logf(SOAPY_SDR_NOTICE, "Read Stream called, requesting: %d elements", numElems);
    LibCyberRadio::VitaIqSource *stream = reinterpret_cast<LibCyberRadio::VitaIqSource *>(handle);
    int n_recv = stream->getPacketsPayloadData(numElems, (void *)buffs);
    //std::memcpy( (void *)buffs, packets.data(), packets.size() * sizeof(packets) );
    return n_recv;
}

size_t CyberRadioSoapy::getStreamMTU(SoapySDR::Stream *handle) const
{
    LibCyberRadio::VitaIqSource *stream = reinterpret_cast<LibCyberRadio::VitaIqSource *>(handle);
    return (long unsigned int)(stream->getPacketSize());
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



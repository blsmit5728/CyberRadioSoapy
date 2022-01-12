#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/ConverterRegistry.hpp>
#include <algorithm> //sort, min, max
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <csignal>
#include <chrono>
#include <thread>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

int main( )
{
    std::string argStr("driver=cyberradiosoapy,host=192.168.0.10,streamif=eth10:eth11:eth12:eth13,radio=ndr358,verbose=true");
    std::cout << "Probe device " << argStr << std::endl;
    auto device = SoapySDR::Device::make(argStr);
    int channels = device->getNumChannels(SOAPY_SDR_RX);
    std::cout << "Channels : " << channels << std::endl;
    device->getFrequencyRange( SOAPY_SDR_RX, 0);
    std::cout << "Tuner: " << 0 << " Freq : " << device->getFrequency( SOAPY_SDR_RX, 0, "_") << std::endl;
    device->setFrequency( SOAPY_SDR_RX, 0, "", 1005e6);
    /*
    ** Sample Rates
    */
    /*
    SoapySDR::RangeList r = device->getSampleRateRange( SOAPY_SDR_RX, 0);
    std::vector<SoapySDR::Range>::iterator it;
    for( it = r.begin(); it != r.end(); it++)
    {
        std::cout << "Start: " << it->minimum() << 
                " Stop: " << it->maximum() << " Step: " << it->step() << std::endl;
    }
    */
    /**********************************************************************/
    //for( int i = 0; i < channels; i++){
    //    std::cout << "Channel: " << i << " Sample Rate: " << device->getSampleRate( SOAPY_SDR_RX, i) << std::endl;
    //    device->setSampleRate(SOAPY_SDR_RX, i, 32e6);
    //    std::cout << "Channel: " << i << " Sample Rate: " << device->getSampleRate( SOAPY_SDR_RX, i) << std::endl;
    //}
    std::vector<size_t> c = {0};
    SoapySDR::Stream * stream = device->setupStream(SOAPY_SDR_RX, "test", c );

    char *dataBuffer = new char[device->getStreamMTU(stream) * 1000];
    int j = 0;
    long long int k = 0;
    const long t = 0;
    std::cout << "activate" << std::endl;
    device->activateStream(stream, 0, 0, 0);        
    int packets = device->readStream(stream, (void * const*)dataBuffer, 1000, j, k, t);
    std::cout << "RX: " << packets << std::endl;
    SoapySDR::Device::unmake(device);
    return EXIT_SUCCESS;
}


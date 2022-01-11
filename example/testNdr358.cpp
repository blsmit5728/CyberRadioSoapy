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
    std::string argStr("driver=cyberradiosoapy,host=192.168.0.10,radio=ndr358,verbose=false");
    std::cout << "Probe device " << argStr << std::endl;
        auto device = SoapySDR::Device::make(argStr);
        std::cout << "Channels :" << device->getNumChannels(SOAPY_SDR_RX) << std::endl;
        device->getFrequencyRange( SOAPY_SDR_RX, 0);
        std::cout << "Freq: " << device->getFrequency( SOAPY_SDR_RX, 0, "_") << std::endl;
        device->getSampleRateRange( SOAPY_SDR_RX, 0);
        device->setSampleRate( SOAPY_SDR_RX, 0, 128e6);
        //SoapySDR::Device::unmake(device);
    return EXIT_SUCCESS;
}
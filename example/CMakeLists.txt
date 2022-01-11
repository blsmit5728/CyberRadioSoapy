LIST( APPEND exampleSources testNdr358.cpp )
SET( ndr358example NDR358Example )
ADD_EXECUTABLE( ${ndr358example} ${exampleSources} )

TARGET_LINK_LIBRARIES( ${ndr358example} 
                       SoapySDR )
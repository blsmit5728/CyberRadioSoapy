LIST( APPEND exampleSources testNdr358.cpp )
SET( ndr358example NDR358Example )
ADD_EXECUTABLE( ${ndr358example} ${exampleSources} )

TARGET_LINK_LIBRARIES( ${ndr358example} 
                       SoapySDR )

LIST( APPEND exampleSources2 testNdr324.cpp )
SET( ndr324example NDR324Example )
ADD_EXECUTABLE( ${ndr324example} ${exampleSources2} )

TARGET_LINK_LIBRARIES( ${ndr324example} 
                       SoapySDR )
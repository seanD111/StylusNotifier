set(TARGET_NAME tablet_driver)

add_library(${TARGET_NAME} SHARED
  driverlog.cpp
  driverlog.h
  tablet_driver.cpp
)

add_definitions(-DDRIVER_SAMPLE_EXPORTS)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
)

setTargetOutputDirectory(${TARGET_NAME})

install(
  PROGRAMS
  memoir-load
  memoir-lower
  memoir-norm
  memoir-norm-runtime
  memoir-opt
  memoir-prof
  memoir-enable
  
  TYPE BIN
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/memoir-config.in
  ${CMAKE_INSTALL_BINDIR}/memoir-config
  USE_SOURCE_PERMISSIONS
  @ONLY
)

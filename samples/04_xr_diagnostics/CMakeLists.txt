#==============================================================================
# 04_xr_diagnostics -- reference scene for checking scale, tracking, input and frame timing
#==============================================================================

vne_xr_add_sample_library(xr_diagnostics
    REGISTER registerXrDiagnosticsSample
    SOURCES
        xr_diagnostics_sample.cpp
        xr_diagnostics_layer.cpp
    LIBRARIES vne::rhi vne::math
)

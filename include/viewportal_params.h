#ifndef VIEWPORTAL_PARAMS_H
#define VIEWPORTAL_PARAMS_H

#include "viewportal.h"
#include <string>

namespace viewportal {

/**
 * Result of loading parameters from file.
 * Holds ViewPortalParams with window_title pointing into window_title_storage
 * so the pointer remains valid for the lifetime of this struct.
 */
struct LoadedParams {
    ViewPortalParams viewportal;
    std::string window_title_storage;
};

/**
 * Load parameters from a key=value config file.
 * Unknown keys are ignored. Missing keys leave defaults from ViewPortalParams.
 * \param path Path to config file; default is "config/params.cfg".
 * \return Loaded params; on missing file or parse error, returns defaults.
 */
LoadedParams loadParams(const std::string& path = "config/params.cfg");

} // namespace viewportal

#endif // VIEWPORTAL_PARAMS_H

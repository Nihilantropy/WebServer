#include "LocationConfigDefaults.hpp"

void LocationConfigDefaults::setDefaults(LocationConfig& config)
{
    _setDefaultAllowedMethods(config);
    _setDefaultIndex(config);
}

void LocationConfigDefaults::_setDefaultAllowedMethods(LocationConfig& config)
{
    // If no allowed methods are defined, set default to GET only
    if (config.getAllowedMethods().empty()) {
        std::vector<std::string> methods;
        methods.push_back("GET");
        config.setAllowedMethods(methods);
    }
}

void LocationConfigDefaults::_setDefaultIndex(LocationConfig& config)
{
    // If autoindex is off and no index is defined, set default index
    if (!config.getAutoIndex() && config.getIndex().empty()) {
        config.setIndex("index.html");
    }
}
#pragma once

#include "../parser/LocationConfig.hpp"

/**
 * @brief Class to set default values for LocationConfig objects
 */
class LocationConfigDefaults
{
public:
    /**
     * @brief Sets default values for a LocationConfig object
     * 
     * @param config LocationConfig object to set defaults for
     */
    static void setDefaults(LocationConfig& config);

private:
    /**
     * @brief Sets default allowed methods if none are defined
     * 
     * @param config LocationConfig object to set defaults for
     */
    static void _setDefaultAllowedMethods(LocationConfig& config);

    
    
    /**
     * @brief Sets default index file if autoindex is off and no index is defined
     * 
     * @param config LocationConfig object to set defaults for
     */
    static void _setDefaultIndex(LocationConfig& config);
};
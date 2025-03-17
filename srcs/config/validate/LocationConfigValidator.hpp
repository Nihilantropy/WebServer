#pragma once

#include "AValidator.hpp"
#include "../parser/LocationConfig.hpp"
#include <iostream>

/**
 * @brief class to validate Location configuration info
 * 
 * @param LocationConfig object at construction
 * 
 * @throw ValidationException subclass of runtime_exception
 */
class LocationConfigValidator : public AValidator
{
public:
	LocationConfigValidator(const LocationConfig& locationConfig);
	~LocationConfigValidator();

private:
	const LocationConfig& _locationConfig;

	void _validate(void) const;
	void _validatePath(void) const;
	void _validateRoot(void) const;
	void _validateAllowedMethods(void) const;
	void _validateIndex(void) const;
	void _validateCgi(void) const;
	void _validateUploadDir(void) const;
	void _validateRedirection(void) const;
};
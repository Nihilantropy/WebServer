#pragma once

#include <stdexcept>

/**
 * @brief abstract class with validate() method
 */
class AValidator
{
public:
	virtual ~AValidator() {}
	virtual void	_validate( void ) const = 0;
};
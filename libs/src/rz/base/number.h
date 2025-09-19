#pragma once


#include <cmath>



namespace rz {

inline bool equal( double a, double b, double epsilon = 1e-3 ) {
	return std::abs( a - b ) < epsilon;
}

inline double toDouble( size_t value, size_t precision = 2 ) {
	// Convert size_t to double
	double result = static_cast<double>(value);

	// Calculate the factor to round to the desired precision
	double factor = std::pow( 10.0, static_cast<double>(precision) );

	// Round the result to the specified precision
	result = std::round( result * factor ) / factor;

	return result;
}

}

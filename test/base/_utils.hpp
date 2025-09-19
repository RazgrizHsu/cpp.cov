#pragma once
#include "doctest.h"

// class BaseTester : public ::testing::Test
// {
// protected:
// 	void SetUp() override
// 	{
// 		//|20210605053156275064|info|2171280|
// 		//lg::set_pattern("|%Y%m%d%H%M%S%f|%l|%t| %v");
//
// 		//|20210611T094650.118696|info|6509611| test
// 		lg::set_pattern("|%Y%m%dT%H%M%S.%f|%l|%t| %v");
//
//
// 		lg::set_error_handler( []( const std::string &msg )
// 		{
// 			lg::get( "console" )->error( "*** LOGGER ERROR ***: {}", msg );
// 		});
//
// //		lg::set_error_handler( []( const std::string &msg )
// //		{
// //			std::cerr << "my err handler: " << msg << std::endl;
// //		});
// 	}
//
// 	void TearDown() override
// 	{
// 	}
//
// public:
// 	static void logBy( const char *msg )
// 	{
// 		lg::info( msg );
// 	}
//
// };

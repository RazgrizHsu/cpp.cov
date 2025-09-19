#include "doctest.h"

#include "../../../libs/src/rz/idx.h"


using namespace rz::dt;
using namespace std::chrono;

TEST_CASE( "idu: base" ) {
    // Test constructors and basic operations
    IDU idu1;
    CHECK_EQ(idu1.ms(), 0);
    IDU idu2(3661000); // 1 hour, 1 minute, 1 second
    CHECK_EQ(idu2.ms(), 3661000);

    // Test string constructor and parse method
    IDU idu3("PT1H1M1S");
    CHECK_EQ(idu3.ms(), 3661000);
    IDU idu4("01:01:01");
    CHECK_EQ(idu4.ms(), 3661000);
    IDU idu5("3661");
    CHECK_EQ(idu5.ms(), 3661000);

    // Test conversions
    CHECK_EQ(idu2.secs(), doctest::Approx(3661.0));
    CHECK_EQ(idu2.mins(), doctest::Approx(61.016666666666666));
    CHECK_EQ(idu2.hours(), doctest::Approx(1.0169444444444444));
    CHECK_EQ(idu2.days(), doctest::Approx(0.042372685185185185));

    // Test basic arithmetic operators
    IDU idu6(1000);
    CHECK_EQ((idu2 + idu6).ms(), 3662000);
    CHECK_EQ((idu2 - idu6).ms(), 3660000);

    // Test factory methods
    auto idu7 = IDU::secs(3661);
    CHECK_EQ(idu7.ms(), 3661000);
    auto idu8 = IDU::mins(61);
    CHECK_EQ(idu8.ms(), 3660000);
    auto idu9 = IDU::hours(1.5);
    CHECK_EQ(idu9.ms(), 5400000);
    auto idu10 = IDU::days(1);
    CHECK_EQ(idu10.ms(), 86400000);

    // Test advanced arithmetic operators
    IDU idu11(1000);
    idu11 += IDU(500);
    CHECK_EQ(idu11.ms(), 1500);
    idu11 -= IDU(200);
    CHECK_EQ(idu11.ms(), 1300);
    auto idu12 = idu11 * 2;
    CHECK_EQ(idu12.ms(), 2600);
    auto idu13 = idu12 / 2;
    CHECK_EQ(idu13.ms(), 1300);

    // Test comparison operators
    CHECK(idu12 > idu13);
    CHECK(idu13 < idu12);
    CHECK(idu13 == idu11);
    CHECK(idu12 != idu13);
    CHECK(idu13 <= idu12);
    CHECK(idu12 >= idu13);

    // Test str method with ISO 8601 format (default)
    IDU idu14(90061500); // 1 day, 1 hour, 1 minute, 1.5 seconds
    CHECK_EQ(idu14.str(), "P1DT1H1M1.500S");
    CHECK_EQ(idu14.human(), "1d 1h 1m 1s");
    CHECK_EQ(idu14.human(true), "1d 1h 1m 1.500s");

    // Test str method with custom formats
    CHECK_EQ(idu14.str("HH:mm:ss"), "25:01:01");
    CHECK_EQ(idu14.str("H:m:s.S"), "25:1:1.5");
    CHECK_EQ(idu14.str("H:m:s.SS"), "25:1:1.50");
    CHECK_EQ(idu14.str("H:m:s.SSS"), "25:1:1.500");

    // Test parse method with various formats
    auto idu15 = IDU::parse("P1DT2H30M45.500S");
    CHECK_EQ(idu15.ms(), 95445500);
    CHECK_EQ(idu15.str(), "P1DT2H30M45.500S");
    CHECK_EQ(idu15.human(), "1d 2h 30m 45s");
    CHECK_EQ(idu15.human(true), "1d 2h 30m 45.500s");

    auto idu16 = IDU::parse("02:30:45.500");
    CHECK_EQ(idu16.ms(), 9045500);
    CHECK_EQ(idu16.str("HH:mm:ss.SSS"), "02:30:45.500");

    auto idu17 = IDU::parse("9045.5");
    CHECK_EQ(idu17.ms(), 9045500);

    // Test edge cases
    IDU idu18(86400000 * 2 + 3661500); // 2 days, 1 hour, 1 minute, 1.5 seconds
    CHECK_EQ(idu18.str("HH:mm:ss"), "49:01:01");
    CHECK_EQ(idu18.str(), "P2DT1H1M1.500S");
    CHECK_EQ(idu18.human(), "2d 1h 1m 1s");
    CHECK_EQ(idu18.human(true), "2d 1h 1m 1.500s");

    // Test human method
    IDU idu19(450321500); // 5 days, 5 hours, 5 minutes, 21.5 seconds
    CHECK_EQ(idu19.human(), "5d 5h 5m 21s");
    CHECK_EQ(idu19.human(true), "5d 5h 5m 21.500s");

    IDU idu20(321500); // 5 minutes, 21.5 seconds
    CHECK_EQ(idu20.human(), "5m 21s");
    CHECK_EQ(idu20.human(true), "5m 21.500s");

    IDU idu21(21500); // 21.5 seconds
    CHECK_EQ(idu21.human(), "21s");
    CHECK_EQ(idu21.human(true), "21.500s");

    // Test zero duration
    IDU idu22(0);
    CHECK_EQ(idu22.human(), "0s");
    CHECK_EQ(idu22.human(true), "0.000s");

    // Test only milliseconds
    IDU idu23(100);
    CHECK_EQ(idu23.human(), "0s");
    CHECK_EQ(idu23.human(true), "0.100s");

    // Test millisecond handling
    IDU idu24(90061510); // 1 day, 1 hour, 1 minute, 1.51 seconds
    CHECK_EQ(idu24.str("HH:mm:ss.S"), "25:01:01.5");
    CHECK_EQ(idu24.str("HH:mm:ss.SS"), "25:01:01.51");
    CHECK_EQ(idu24.str("HH:mm:ss.SSS"), "25:01:01.510");

    IDU idu25(90061501); // 1 day, 1 hour, 1 minute, 1.501 seconds
    CHECK_EQ(idu25.str("HH:mm:ss.S"), "25:01:01.5");
    CHECK_EQ(idu25.str("HH:mm:ss.SS"), "25:01:01.50");
    CHECK_EQ(idu25.str("HH:mm:ss.SSS"), "25:01:01.501");

    // Test boundary cases
    IDU idu26(90061999); // 1 day, 1 hour, 1 minute, 1.999 seconds
    CHECK_EQ(idu26.str("HH:mm:ss.S"), "25:01:01.9");
    CHECK_EQ(idu26.str("HH:mm:ss.SS"), "25:01:01.99");
    CHECK_EQ(idu26.str("HH:mm:ss.SSS"), "25:01:01.999");

    IDU idu27(90061001); // 1 day, 1 hour, 1 minute, 1.001 seconds
    CHECK_EQ(idu27.str("HH:mm:ss.S"), "25:01:01.0");
    CHECK_EQ(idu27.str("HH:mm:ss.SS"), "25:01:01.00");
    CHECK_EQ(idu27.str("HH:mm:ss.SSS"), "25:01:01.001");

    // Test parse with hours > 24
    auto idu28 = IDU::parse("25:00:00");
    CHECK_EQ(idu28.ms(), 90000000);
    CHECK_EQ(idu28.str("HH:mm:ss"), "25:00:00");

    // Test invalid input
    CHECK_THROWS_AS(IDU::parse("invalid"), std::invalid_argument);
	CHECK_NOTHROW( IDU::parse("P1DT25H") );
	CHECK_NOTHROW( IDU::parse("25:00:00") );
}

TEST_CASE( "idt: base" ) {

	CHECK_EQ( IDT::parse("2000-01-01 00:00:00")->isLeapYear(), true );
	CHECK_EQ( IDT::parse("2024-01-01 00:00:00")->isLeapYear(), true );
	CHECK_EQ( IDT::parse("2023-01-01 00:00:00")->isLeapYear(), false );
	CHECK_EQ( IDT::parse("1900-01-01 00:00:00")->isLeapYear(), false );

	CHECK_THROWS_AS( IDT("invalid-date-string"), std::invalid_argument );
	CHECK_THROWS_AS( IDT("2021-13-01T00:00:00XXX"), std::invalid_argument );
	CHECK_THROWS_AS( IDT("2021/01/32T00:00:00Z"), std::invalid_argument );


	long long now = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
	IDT idt1( now, "+00:00" );
	CHECK_EQ( idt1.unixtime(), now );
	CHECK_EQ( idt1.tz(), 0 );
	CHECK_EQ( idt1.zoneStr(), "UTC" );

	idt1.tz( "+08:00" );
	CHECK_EQ( idt1.tz(), 480 );
	CHECK_EQ( idt1.zoneStr(), "UTC+08:00" );

	// Test date and time components
	IDT idt2( 1629234567000, "+00:00" ); //2021-08-17T21:09:27.000Z (ISOString)
	CHECK_EQ( idt2.year(), 2021 );
	CHECK_EQ( idt2.month(), 8 );
	CHECK_EQ( idt2.day(), 17 );
	CHECK_EQ( idt2.hour(), 21 );
	CHECK_EQ( idt2.minute(), 9 );
	CHECK_EQ( idt2.second(), 27 );
	CHECK_EQ( idt2.millisecond(), 0 );
	CHECK_EQ( idt2.tz(), 0 );
	CHECK_EQ( idt2.str(), "2021-08-17 21:09:27.000" );
	CHECK_EQ( idt2.utc(), "2021-08-17 21:09:27.000" );
	CHECK_EQ( idt2.iso(), "2021-08-17T21:09:27.000Z" );
	CHECK_EQ( idt2.dayOfYear(), 229 ); // August 17 is the 229th day of the year
	CHECK_EQ( idt2.dayOfWeek(), 2 );   // August 17, 2021 was a Tuesday (2)



	// Test setters
	idt2.year( 2022 );
	idt2.month( 9 );
	idt2.day( 18 );
	idt2.hour( 20 );
	idt2.minute( 43 );
	idt2.second( 48 );
	idt2.millisecond( 500 );
	CHECK_EQ( idt2.year(), 2022 );
	CHECK_EQ( idt2.month(), 9 );
	CHECK_EQ( idt2.day(), 18 );
	CHECK_EQ( idt2.hour(), 20 );
	CHECK_EQ( idt2.minute(), 43 );
	CHECK_EQ( idt2.second(), 48 );
	CHECK_EQ( idt2.millisecond(), 500 );
	CHECK_EQ( idt2.str(), "2022-09-18 20:43:48.500" );
	CHECK_EQ( idt2.utc(), "2022-09-18 20:43:48.500" );
	CHECK_EQ( idt2.iso(), "2022-09-18T20:43:48.500Z" );

	idt2.tz( "+01:00" );
	CHECK_EQ( idt2.str(), "2022-09-18 21:43:48.500" );
	CHECK_EQ( idt2.iso(), "2022-09-18T21:43:48.500+01:00" );
	CHECK_EQ( idt2.utc(), "2022-09-18 20:43:48.500" );


	// Test operators
	IDT idt3 = idt2 + IDU( 1000 );
	CHECK_EQ( idt3.unixtime(), idt2.unixtime() + 1000 );

	IDU diff = idt3 - idt2;
	CHECK_EQ( diff.ms(), 1000 );

	CHECK( idt2 < idt3 );
	CHECK( idt3 > idt2 );
	CHECK( idt2 <= idt3 );
	CHECK( idt3 >= idt2 );
	CHECK( idt2 != idt3 );


	// Test parse method
	auto idt4 = IDT::parse( "2022-03-14 15:09:26" );
	REQUIRE( idt4.has_value() );
	CHECK_EQ( idt4->year(), 2022 );
	CHECK_EQ( idt4->month(), 3 );
	CHECK_EQ( idt4->day(), 14 );
	CHECK_EQ( idt4->hour(), 15 );
	CHECK_EQ( idt4->minute(), 9 );
	CHECK_EQ( idt4->second(), 26 );
	CHECK_EQ( idt4->dayOfWeek(), 1 );  // Monday
	CHECK_EQ( idt4->dayOfYear(), 73 ); // 73rd day of the year
	CHECK_EQ( idt4->isLeapYear(), false );
	CHECK_EQ( idt4->str(), "2022-03-14 15:09:26.000" );
	CHECK_EQ( idt4->iso(), "2022-03-14T15:09:26.000Z" );
	CHECK_EQ( idt4->utc(), "2022-03-14 15:09:26.000" );

	CHECK_EQ( idt4->tz(), 0 );
	idt4->tz( "+01:00" );
	CHECK_EQ( idt4->tz(), 60 );
	CHECK_EQ( idt4->str(), "2022-03-14 16:09:26.000" );
	CHECK_EQ( idt4->iso(), "2022-03-14T16:09:26.000+01:00" );
	CHECK_EQ( idt4->utc(), "2022-03-14 15:09:26.000" );
	CHECK_EQ( idt4->strftime("%Y-%m-%d"), "2022-03-14" );
	CHECK_EQ( idt4->strftime("%H:%M:%S"), "16:09:26" );

	*idt4 += IDU::days( 1 );
	CHECK_EQ( idt4->day(), 15 );
	CHECK_EQ( idt4->str(), "2022-03-15 16:09:26.000" );
	CHECK_EQ( idt4->iso(), "2022-03-15T16:09:26.000+01:00" );
	CHECK_EQ( idt4->utc(), "2022-03-15 15:09:26.000" );

	*idt4 -= IDU::hours( 2 );
	*idt4 -= IDU::mins( 8 );
	*idt4 -= IDU::secs( 26 );
	CHECK_EQ( idt4->hour(), 14 );
	CHECK_EQ( idt4->str(), "2022-03-15 14:01:00.000" );
	CHECK_EQ( idt4->iso(), "2022-03-15T14:01:00.000+01:00" );
	CHECK_EQ( idt4->utc(), "2022-03-15 13:01:00.000" );


	// Test ISO 8601 constructor and timezone handling
	IDT idt_iso1( "2021-08-17T21:09:27Z" );
	CHECK_EQ( idt_iso1.year(), 2021 );
	CHECK_EQ( idt_iso1.month(), 8 );
	CHECK_EQ( idt_iso1.day(), 17 );
	CHECK_EQ( idt_iso1.hour(), 21 );
	CHECK_EQ( idt_iso1.minute(), 9 );
	CHECK_EQ( idt_iso1.second(), 27 );
	CHECK_EQ( idt_iso1.millisecond(), 0 );
	CHECK_EQ( idt_iso1.tz(), 0 );
	CHECK_EQ( idt_iso1.str(), "2021-08-17 21:09:27.000" );
	CHECK_EQ( idt_iso1.iso(), "2021-08-17T21:09:27.000Z" );
	CHECK_EQ( idt_iso1.dayOfYear(), 229 ); // August 17 is the 229th day of the year
	CHECK_EQ( idt_iso1.dayOfWeek(), 2 );   // August 17, 2021 was a Tuesday (2)

	IDT idt_iso2( "2021-08-17T21:09:27.123+08:00" );
	CHECK_EQ( idt_iso2.year(), 2021 );
	CHECK_EQ( idt_iso2.month(), 8 );
	CHECK_EQ( idt_iso2.day(), 17 );
	CHECK_EQ( idt_iso2.hour(), 21 );
	CHECK_EQ( idt_iso2.minute(), 9 );
	CHECK_EQ( idt_iso2.second(), 27 );
	CHECK_EQ( idt_iso2.millisecond(), 123 );
	CHECK_EQ( idt_iso2.tz(), 480 );
	CHECK_EQ( idt_iso2.str(), "2021-08-17 21:09:27.123" );
	CHECK_EQ( idt_iso2.str("YYYY-MM-DD HH:mm:ss.SSS"), "2021-08-17 21:09:27.123" );
	CHECK_EQ( idt_iso2.iso(), "2021-08-17T21:09:27.123+08:00" );
	CHECK_EQ( idt_iso2.utc(), "2021-08-17 13:09:27.123" );
	CHECK_EQ( idt_iso2.utc("YYYY-MM-DD HH:mm:ss.SSS"), "2021-08-17 13:09:27.123" );
	CHECK_EQ( idt_iso2.dayOfYear(), 229 ); // August 17 is the 229th day of the year
	CHECK_EQ( idt_iso2.dayOfWeek(), 2 );   // August 17, 2021 was a Tuesday (2)



	IDT idt_iso3( "2021-08-17T21:09:27-05:30" );
	CHECK_EQ( idt_iso3.tz(), -330 );
	CHECK_EQ( idt_iso3.str(), "2021-08-17 21:09:27.000" );
	CHECK_EQ( idt_iso3.iso(), "2021-08-17T21:09:27.000-05:30" );


	// Test edge cases
	IDT idt_iso4( "2021-12-31T23:59:59.999Z" );
	CHECK_EQ( idt_iso4.tz(), 0 );
	CHECK_EQ( idt_iso4.zoneStr(), "UTC" );
	CHECK_EQ( idt_iso4.str(), "2021-12-31 23:59:59.999" );
	CHECK_EQ( idt_iso4.iso(), "2021-12-31T23:59:59.999Z" );
	CHECK_EQ( idt_iso4.dayOfYear(), 365 ); // December 31 is the 365th day of the year
	CHECK_EQ( idt_iso4.dayOfWeek(), 5 );   // December 31, 2021 was a Friday (5)

	IDT idt_iso5( "2021-01-01T00:00:00Z" );
	CHECK_EQ( idt_iso5.tz(), 0 );
	CHECK_EQ( idt_iso5.zoneStr(), "UTC" );
	CHECK_EQ( idt_iso5.str(), "2021-01-01 00:00:00.000" );
	CHECK_EQ( idt_iso5.iso(), "2021-01-01T00:00:00.000Z" );
	CHECK_EQ( idt_iso5.dayOfYear(), 1 ); // January 1 is the 1st day of the year
	CHECK_EQ( idt_iso5.dayOfWeek(), 5 ); // January 1, 2021 was a Friday (5)

	// Test timezone change
	IDT idt6( 1629590461000, "+00:00" );
	CHECK_EQ( idt6.iso(), "2021-08-22T00:01:01.000Z" );
	CHECK_EQ( idt6.unixtime(), 1629590461000 );
	CHECK_EQ( idt6.dayOfYear(), 234 ); // August 22 is the 234th day of the year
	CHECK_EQ( idt6.dayOfWeek(), 0 );   // August 22, 2021 was a Sunday (0)


	idt6.tz( "+01:00" );
	CHECK_EQ( idt6.tz(), 60 );
	CHECK_EQ( idt6.str(), "2021-08-22 01:01:01.000" );
	CHECK_EQ( idt6.iso(), "2021-08-22T01:01:01.000+01:00" );
	CHECK_EQ( idt6.utc(), "2021-08-22 00:01:01.000" );
	CHECK_EQ( idt6.day(), 22 );
	CHECK_EQ( idt6.hour(), 01 );


	auto idt7 = IDT::parse( "2022-03-14 15:09:26" );
	REQUIRE( idt7.has_value() );
	CHECK_EQ( idt7->str("YYYY-MM-DD HH:mm:ss"), "2022-03-14 15:09:26" );

	auto idt8 = IDT::parse( idt7->str() );
	CHECK_EQ( idt8->str("YYYY-MM-DD HH:mm:ss"), "2022-03-14 15:09:26" );
}

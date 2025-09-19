/*
 *  ReplayGainAnalysis Error Reporting 
 *     Handles error reporting for mp3gain in either standalone or DLL form.
 */

#include "rg_error.h"

extern int gSuccess;

void DoError( char * localerrstr, MMRESULT localerrnum )
{
    gSuccess = 0;
	fprintf(stdout, "%s", localerrstr);
}

void DoUnkError( char * localerrstr)
{
	DoError( localerrstr, MP3GAIN_UNSPECIFED_ERROR );
}
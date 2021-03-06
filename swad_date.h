// swad_date.h: dates

#ifndef _SWAD_DAT
#define _SWAD_DAT
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2017 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#include <stdbool.h>		// For boolean type
#include <stdio.h>		// For FILE *
#include <time.h>

#include "swad_constant.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

#define Dat_SECONDS_IN_ONE_MONTH (30UL * 24UL * 60UL * 60UL)

#define Dat_MAX_BYTES_TIME_ZONE 256

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

#define Dat_LENGTH_YYYYMMDD (4 + 2 + 2)
struct Date
  {
   unsigned Day;
   unsigned Month;
   unsigned Year;
   unsigned Week;
   char YYYYMMDD[Dat_LENGTH_YYYYMMDD + 1];
  };
struct Time
  {
   unsigned Hour;
   unsigned Minute;
   unsigned Second;
  };
struct Hour
  {
   unsigned Hour;
   unsigned Minute;
  };
struct DateTime
  {
   struct Date Date;
   struct Time Time;
   char YYYYMMDDHHMMSS[4 + 2 + 2 + 2 + 2 + 2 + 1];
  };

#define Dat_NUM_TIME_STATUS 3
typedef enum
  {
   Dat_PAST    = 0,
   Dat_PRESENT = 1,
   Dat_FUTURE  = 2,
  } Dat_TimeStatus_t;

#define Dat_NUM_START_END_TIME 2
typedef enum
  {
   Dat_START_TIME = 0,
   Dat_END_TIME   = 1,
  } Dat_StartEndTime_t;

#define Dat_NUM_FORM_SECONDS 2
typedef enum
  {
   Dat_FORM_SECONDS_OFF,
   Dat_FORM_SECONDS_ON,
  } Dat_FormSeconds;

typedef enum
  {
   Dat_HMS_DO_NOT_SET = 0,
   Dat_HMS_TO_000000  = 1,
   Dat_HMS_TO_235959  = 2,
  } Dat_SetHMS;

/***** Date format *****/
#define Dat_NUM_OPTIONS_FORMAT 3
typedef enum
  {
   Dat_FORMAT_YYYY_MM_DD	= 0,	// ISO 8601, default
   Dat_FORMAT_DD_MONTH_YYYY	= 1,
   Dat_FORMAT_MONTH_DD_YYYY	= 2,
  } Dat_Format_t;	// Do not change these numbers because they are used in database
#define Dat_FORMAT_DEFAULT Dat_FORMAT_YYYY_MM_DD

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Dat_PutBoxToSelectDateFormat (void);

void Dat_PutSpanDateFormat (Dat_Format_t Format);
void Dat_PutScriptDateFormat (Dat_Format_t Format);

void Dat_ChangeDateFormat (void);
Dat_Format_t Dat_GetDateFormatFromStr (const char *Str);

void Dat_GetStartExecutionTimeUTC (void);
void Dat_GetAndConvertCurrentDateTime (void);

time_t Dat_GetUNIXTimeFromStr (const char *Str);
bool Dat_GetDateFromYYYYMMDD (struct Date *Date,const char *YYYYMMDDString);

void Dat_ShowClientLocalTime (void);

struct tm *Dat_GetLocalTimeFromClock (const time_t *timep);
void Dat_ConvDateToDateStr (struct Date *Date,char StrDate[Cns_MAX_BYTES_DATE + 1]);

void Dat_PutFormStartEndClientLocalDateTimesWithYesterdayToday (bool SetHMS000000To235959);
void Dat_PutFormStartEndClientLocalDateTimes (time_t TimeUTC[2],
                                              Dat_FormSeconds FormSeconds);

void Dat_WriteFormClientLocalDateTimeFromTimeUTC (const char *Id,
                                                  const char *ParamName,
                                                  time_t TimeUTC,
                                                  unsigned FirstYear,
                                                  unsigned LastYear,
                                                  Dat_FormSeconds FormSeconds,
                                                  Dat_SetHMS SetHMS,
                                                  bool SubmitFormOnChange);
time_t Dat_GetTimeUTCFromForm (const char *ParamName);

void Dat_PutHiddenParBrowserTZDiff (void);
void Dat_GetBrowserTimeZone (char BrowserTimeZone[Dat_MAX_BYTES_TIME_ZONE + 1]);

void Dat_WriteFormDate (unsigned FirstYear,unsigned LastYear,
	                const char *Id,
		        struct Date *DateSelected,
                        bool SubmitFormOnChange,bool Disabled);
void Dat_GetDateFromForm (const char *ParamNameDay,const char *ParamNameMonth,const char *ParamNameYear,
                          unsigned *Day,unsigned *Month,unsigned *Year);

void Dat_GetIniEndDatesFromForm (void);

void Dat_WriteRFC822DateFromTM (FILE *File,struct tm *tm);

void Dat_GetDateBefore (struct Date *Date,struct Date *PrecedingDate);
void Dat_GetDateAfter (struct Date *Date,struct Date *SubsequentDate);
void Dat_GetWeekBefore (struct Date *Date,struct Date *PrecedingDate);
void Dat_GetMonthBefore (struct Date *Date,struct Date *PrecedingDate);
unsigned Dat_GetNumDaysBetweenDates (struct Date *DateIni,struct Date *DateEnd);
unsigned Dat_GetNumWeeksBetweenDates (struct Date *DateIni,struct Date *DateEnd);
unsigned Dat_GetNumMonthsBetweenDates (struct Date *DateIni,struct Date *DateEnd);
unsigned Dat_GetNumDaysInYear (unsigned Year);
unsigned Dat_GetNumDaysFebruary (unsigned Year);
bool Dat_GetIfLeapYear (unsigned Year);
unsigned Dat_GetNumWeeksInYear (unsigned Year);
unsigned Dat_GetDayOfWeek (unsigned Year,unsigned Month,unsigned Day);
unsigned Dat_GetDayOfYear (struct Date *Date);
void Dat_CalculateWeekOfYear (struct Date *Date);
void Dat_AssignDate (struct Date *DateDst,struct Date *DateSrc);

void Dat_WriteScriptMonths (void);

#endif

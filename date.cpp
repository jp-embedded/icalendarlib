/*************************************************************************
 ** Copyright (C) 2024 Jan Pedersen <jp@jp-embedded.com>
 ** 
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 ** 
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** 
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "date.h"

string Date::Format() const {
  char Temp[10 + 1 + 6 + 1 + 6 + 1];
  sprintf(Temp, "%.4d/%.2d/%.2d", Data[YEAR], Data[MONTH], Data[DAY]);
  if (WithTime == true)
    sprintf(Temp + 10, " %.2d:%.2d", Data[HOUR], Data[MINUTE]);
  if (tzid.size() > 0)
    return string(Temp) + ' ' + tzid;
  return string(Temp);
}

Date::operator string() const {
  char Temp[8 + 1 + 6 + 6 + 6 + 1];
  sprintf(Temp, "%.4d%.2d%.2d", Data[YEAR], Data[MONTH], Data[DAY]);
  if (WithTime == true)
    sprintf(Temp + 8, "T%.2d%.2d%.2d", Data[HOUR], Data[MINUTE], Data[SECOND]);
  return string(Temp);
}

Date &Date::operator=(const string &Text) {
  if (Text.length() >= 8) {
    sscanf(Text.c_str(), "%4hd%2hd%2hd", &Data[YEAR], &Data[MONTH], &Data[DAY]);

    if (Text.length() >= 15) {
      sscanf(Text.c_str() + 9, "%2hd%2hd%2hd", &Data[HOUR], &Data[MINUTE],
             &Data[SECOND]);
      WithTime = true;
    } else {
      Data[HOUR] = Data[MINUTE] = Data[SECOND] = 0;
      WithTime = false;
    }
  } else
    Data[YEAR] = Data[MONTH] = Data[DAY] = 0;
  return *this;
}

// difference *this - OtherDate
unsigned long Date::Difference(Date &OtherDate, TimeUnit Unit,
                               bool RoundUp) const {
  unsigned long Result = 0;

  switch (Unit) {
  case YEAR:
    Result = Data[YEAR] - OtherDate.Data[YEAR];
    break;

  case MONTH:
    Result = (Data[YEAR] - OtherDate.Data[YEAR]) * 12 + Data[MONTH] -
             OtherDate.Data[MONTH];
    break;

  case DAY:
    if (OtherDate.Data[YEAR] == Data[YEAR]) {
      if (OtherDate.Data[MONTH] == Data[MONTH]) {
        Result = Data[DAY] - OtherDate.Data[DAY];
      } else {
        Result = (OtherDate.DaysInMonth() - OtherDate.Data[DAY]) + Data[DAY];

        for (char i = OtherDate.Data[MONTH] + 1; i < Data[MONTH]; ++i)
          Result += OtherDate.DaysInMonth(i);
      }
    } else {
      // number of days in the starting year
      Result = OtherDate.DaysInMonth() - OtherDate.Data[DAY];
      for (char i = OtherDate.Data[MONTH] + 1; i <= 12; ++i)
        Result += OtherDate.DaysInMonth(i);

      // number of days in years between the starting and ending years
      for (short i = OtherDate.Data[YEAR] + 1; i < Data[YEAR]; ++i)
        Result += IsLeapYear(i) ? 366 : 365;

      // number of days in the ending year
      for (char i = 1; i < Data[MONTH]; ++i)
        Result += DaysInMonth(i);
      Result += Data[DAY];
    }

    break;

  case HOUR:
    Result = Difference(OtherDate, DAY, false) * 24 + Data[HOUR] -
             OtherDate.Data[HOUR];
    break;

  case MINUTE:
    Result = Difference(OtherDate, HOUR, false) * 60 + Data[MINUTE] -
             OtherDate.Data[MINUTE];
    break;

  case SECOND:
    Result = Difference(OtherDate, MINUTE, false) * 60 + Data[SECOND] -
             OtherDate.Data[SECOND];
    break;

  case WEEK:
    Result = (Difference(OtherDate, DAY, false) + 6) / 7;
    break;
  }

  // Example hour results
  // 22:01 - 20:51 = 1:10 =>  Result = 2, Compare = -1, round-up = 2
  // 22:51 - 20:01 = 2:50 =>  Result = 2, Compare =  1, round-up = 3  (+1)
  // 22:31 - 20:30 = 2:01 =>  Result = 2, Compare =  1, round-up = 3  (+1)
  // 22:30 - 20:30 = 2:00 =>  Result = 2, Compare =  0, round-up = 2

  // Example week results. WEEK is not rounded below, but in case above
  // mon 30 - fri 29 =  3d = 0w =>  Result = 1, Compare = -1, round-up = 1
  // fri 30 - mon 29 = 11d = 1w =>  Result = 1, Compare =  1, round-up = 2  (+1)
  // wed 30 - tue 29 =  8d = 1w =>  Result = 1, Compare =  1, round-up = 2  (+1)
  // wed 30 - wed 29 =  7d = 1w =>  Result = 1, Compare =  0, round-up = 1

  if (RoundUp == true && Unit < SECOND &&
      Compare(OtherDate, (TimeUnit)(Unit + 1)) > 0)
    ++Result;
  return Result;
}

void Date::SetToNow() {
  time_t Timestamp;
  tm *CurrentTime;

  time(&Timestamp);
  CurrentTime = localtime(&Timestamp);

  Data[YEAR] = CurrentTime->tm_year + 1900;
  Data[MONTH] = CurrentTime->tm_mon + 1;
  Data[DAY] = CurrentTime->tm_mday;
  Data[HOUR] = CurrentTime->tm_hour;
  Data[MINUTE] = CurrentTime->tm_min;
  Data[SECOND] = CurrentTime->tm_sec;

  WithTime = true;
}

Date::DatePart &Date::DatePart::operator+=(short Value) {
  switch (Part) {
  case YEAR:
    BaseDate.Data[YEAR] += Value;
    break;

  case MONTH:
    BaseDate.Data[MONTH] += Value;
    BaseDate[YEAR] += (BaseDate.Data[MONTH] - 1) / 12;
    BaseDate.Data[MONTH] = (BaseDate.Data[MONTH] - 1) % 12 + 1;
    break;

  case DAY:
    BaseDate.Data[DAY] += Value;
    while (BaseDate.Data[DAY] > BaseDate.DaysInMonth()) {
      BaseDate.Data[DAY] -= BaseDate.DaysInMonth();
      BaseDate[MONTH] += 1;
    }
    break;

  case HOUR:
    BaseDate.Data[HOUR] += Value;
    BaseDate[DAY] += BaseDate.Data[HOUR] / 24;
    BaseDate.Data[HOUR] %= 24;
    break;

  case MINUTE:
    BaseDate.Data[MINUTE] += Value;
    BaseDate[HOUR] += BaseDate.Data[MINUTE] / 60;
    BaseDate.Data[MINUTE] %= 60;
    break;

  case SECOND:
    BaseDate.Data[SECOND] += Value;
    BaseDate[MINUTE] += BaseDate.Data[SECOND] / 60;
    BaseDate.Data[SECOND] %= 60;
    break;

  case WEEK:
    BaseDate[DAY] += Value * 7;
    break;
  }
  return *this;
}

Date::DatePart &Date::DatePart::operator-=(short Value) {
  switch (Part) {
  case YEAR:
    BaseDate.Data[YEAR] -= Value;
    break;

  case MONTH:
    BaseDate.Data[MONTH] -= Value;
    if (BaseDate.Data[MONTH] < 1) {
      BaseDate[YEAR] -= -(BaseDate.Data[MONTH] - 12) / 12;
      BaseDate.Data[MONTH] = 12 + BaseDate.Data[MONTH] % 12;
    }
    break;

  case DAY:
    // todo: won't work for large Value
    BaseDate.Data[DAY] -= Value;
    while (BaseDate.Data[DAY] < 1) {
      BaseDate[MONTH] -= 1;
      BaseDate.Data[DAY] += BaseDate.DaysInMonth();
    }
    break;

  case HOUR:
    // todo: won't work for large Value
    BaseDate.Data[HOUR] -= Value;
    if (BaseDate.Data[HOUR] < 0) {
      BaseDate[DAY] -= -(BaseDate.Data[HOUR] - 24) / 24;
      BaseDate.Data[HOUR] = 24 + BaseDate.Data[HOUR] % 24;
    }
    break;

  case MINUTE:
    // todo: won't work for large Value
    BaseDate.Data[MINUTE] -= Value;
    if (BaseDate.Data[MINUTE] < 0) {
      BaseDate[HOUR] -= -(BaseDate.Data[MINUTE] - 60) / 60;
      BaseDate.Data[MINUTE] = 60 + BaseDate.Data[MINUTE] % 60;
    }
    break;

  case SECOND:
    // todo: won't work for large Value
    BaseDate.Data[SECOND] -= Value;
    if (BaseDate.Data[SECOND] < 0) {
      BaseDate[MINUTE] -= -(BaseDate.Data[SECOND] - 60) / 60;
      BaseDate.Data[SECOND] = 60 + BaseDate.Data[SECOND] % 60;
    }
    break;

  case WEEK:
    BaseDate[DAY] -= Value * 7;
    break;
  }
  return *this;
}

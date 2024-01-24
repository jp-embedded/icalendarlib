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

#include "types.h"

Recurrence::operator string() const {
  char Temp[6];
  string Text = "FREQ=";
  switch (Freq) {
  case YEAR:
    Text += "YEARLY";
    break;
  case MONTH:
    Text += "MONTHLY";
    break;
  case DAY:
    Text += "DAILY";
    break;
  case HOUR:
    Text += "HOURLY";
    break;
  case MINUTE:
    Text += "MINUTELY";
    break;
  case SECOND:
    Text += "SECONDLY";
    break;
  case WEEK:
    Text += "WEEKLY";
    break;
  }
  Text += ";INTERVAL=";
  sprintf(Temp, "%d", Interval);
  Text += Temp;
  if (Count > 0) {
    Text += ";COUNT=";
    sprintf(Temp, "%d", Count);
    Text += Temp;
  } else if (!Until.IsEmpty()) {
    Text += ";UNTIL=";
    Text += Until;
  }

  return Text;
}

AlarmTrigger &AlarmTrigger::operator=(const string &Text) {
  char UnitChar;
  // 1 because always at least 'P' before the value
  short i = 1;

  if (Text[0] == '-')
    Before = true;

  while (!isdigit(Text[i]))
    ++i;

  sscanf(Text.c_str() + i, "%hd%c", (short *)&Value, &UnitChar);

  switch (UnitChar) {
  case 'H':
    Unit = HOUR;
    break;
  case 'D':
    Unit = DAY;
    break;
  case 'W':
    Unit = WEEK;
    break;
  default:
    Unit = MINUTE;
    break;
  }

  return *this;
}

AlarmTrigger::operator string() const {
  string Text;
  char Temp[6];

  sprintf(Temp, "%d", Value);

  if (Before)
    Text = '-';
  Text += 'P';
  if (Unit != WEEK && Unit != DAY)
    Text += 'T';
  Text += Temp;
  switch (Unit) {
  case HOUR:
    Text += 'H';
    break;
  case DAY:
    Text += 'D';
    break;
  case WEEK:
    Text += 'W';
    break;
  default:
    Text += 'M';
    break;
  }

  return Text;
}

Alarm::operator string() const {
  string Text = "\r\nBEGIN:VALARM";
  Text += "\r\nTRIGGER:";
  Text += Trigger;
  Text += "\r\nACTION:";
  switch (Action) {
  case AUDIO:
    Text += "AUDIO";
    break;
  case DISPLAY:
    Text += "DISPLAY";
    break;
  case PROCEDURE:
    Text += "PROCEDURE";
    break;
  case EMAIL:
    Text += "EMAIL";
    break;
  }
  if (!Description.empty()) {
    Text += "\r\nDESCRIPTION:";
    Text += Description;
  }
  Text += "\r\nEND:VALARM";

  return Text;
}

Event::operator string() const {
  string Text = "BEGIN:VEVENT";
  Text += "\r\nUID:";
  Text += UID;
  Text += "\r\nSUMMARY:";

  Text += Summary;
  // some programs do not add this property, let's leave it when modifying
  if (!DtStamp.IsEmpty()) {
    Text += "\r\nDTSTAMP:";
    Text += DtStamp;
  }
  if (!Completed.IsEmpty()) {
    Text += "\r\nDTSTAMP:";
    Text += Completed;
  }
  Text += "\r\nDTSTART:";
  Text += DtStart;
  if (!DtEnd.IsEmpty()) {
    Text += "\r\nDTEND:";
    Text += DtEnd;
  }
  if (!Description.empty()) {
    Text += "\r\nDESCRIPTION:";
    Text += Description;
  }
  if (!Categories.empty()) {
    Text += "\r\nCATEGORIES:";
    Text += Categories;
  }
  if (!Status.empty()) {
    Text += "\r\nSTATUS:";
    Text += Status;
  }
  if (!RRule.IsEmpty()) {
    Text += "\r\nRRULE:";
    Text += RRule;
  }

  if (!Alarms->empty()) {
    for (list<Alarm>::const_iterator AlarmsIterator = Alarms->begin();
         AlarmsIterator != Alarms->end(); ++AlarmsIterator) {
      Text += *AlarmsIterator;
    }
  }

  Text += "\r\nEND:VEVENT\r\n";

  return Text;
}

bool Event::HasAlarm(const Date &From, const Date &To) {
  if (Alarms->empty())
    return false;

  bool HasAlarm = false;
  Date AlarmDate;

  for (list<Alarm>::iterator AlarmsIterator = Alarms->begin();
       AlarmsIterator != Alarms->end(); ++AlarmsIterator) {
    AlarmDate = DtStart;

    if ((*AlarmsIterator).Trigger.Before)
      AlarmDate[(*AlarmsIterator).Trigger.Unit] -=
          (*AlarmsIterator).Trigger.Value;
    else
      AlarmDate[(*AlarmsIterator).Trigger.Unit] +=
          (*AlarmsIterator).Trigger.Value;

    if (AlarmDate >= From && AlarmDate <= To) {
      HasAlarm = true;
      (*AlarmsIterator).Active = true;
    } else {
      (*AlarmsIterator).Active = false;
    }
  }

  return HasAlarm;
}

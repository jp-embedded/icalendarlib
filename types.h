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

#ifndef _TYPES_H
#define _TYPES_H

#include "date.h"
#include <list>
#include <string>
#include <vector>

using namespace std;

struct DeleteItem {
  template <typename T> void operator()(T *ptr) { delete ptr; }
};

// Custom data types

typedef enum { VCALENDAR, VEVENT, VALARM } Component;
typedef enum { DISPLAY = 0, PROCEDURE, AUDIO, EMAIL } AlarmAction;

struct Recurrence {
  Recurrence() : Freq(YEAR), Interval(0), Count(0), WeekStart(SU) {}
  operator string() const;
  bool IsEmpty() const { return (Interval == 0); }
  void Clear() { Interval = 0; }

  TimeUnit Freq;
  unsigned short Interval, Count;
  Date Until;
  Days WeekStart;
  vector<Days> ByDay;
};

struct AlarmTrigger {
  AlarmTrigger() : Before(true), Value(0), Unit(MINUTE) {}
  AlarmTrigger &operator=(const string &Text);
  operator string() const;

  bool Before;
  unsigned short Value;
  TimeUnit Unit;
};

struct Alarm {
  Alarm() : Active(false), Action(DISPLAY) {}
  operator string() const;
  void Clear() { Description.clear(); }

  bool Active;
  AlarmAction Action;
  AlarmTrigger Trigger;
  string Description;
};

struct Event {
  Event()
      : Alarms(new list<Alarm>), ExDates(new list<Date>), RecurrenceNo(0),
        BaseEvent(this) {}
  Event(const Event &Base)
      : UID(Base.UID), Summary(Base.Summary), Description(Base.Description),
        Categories(Base.Categories), Status(Base.Status),
        Completed(Base.Completed), DtStamp(Base.DtStamp), DtStart(Base.DtStart),
        DtEnd(Base.DtEnd), RRule(Base.RRule), Alarms(Base.Alarms),
        ExDates(Base.ExDates), RecurrenceNo(Base.RecurrenceNo) {
    BaseEvent =
        Base.BaseEvent == (Event *)&Base ? (Event *)&Base : Base.BaseEvent;
  }
  ~Event() {
    if (BaseEvent == this) {
      delete Alarms;
      delete ExDates;
    }
  }
  operator string() const;
  bool HasAlarm(const Date &From, const Date &To);

  string UID, Summary, Description, Categories, Status;
  Date Completed, DtStamp, DtStart, DtEnd;
  Recurrence RRule;
  list<Alarm> *Alarms;
  list<Date> *ExDates;
  unsigned short RecurrenceNo;
  Event *BaseEvent;
};

struct EventsCriteria {
  EventsCriteria() : AllEvents(false), IncludeRecurrent(true) {}

  Date From, To;
  bool AllEvents, IncludeRecurrent;
};

inline ostream &operator<<(ostream &stream, const Recurrence &RRule) {
  stream << RRule.operator string();
  return stream;
}

inline ostream &operator<<(ostream &stream, const Alarm &Alarm) {
  stream << Alarm.operator string();
  return stream;
}

inline ostream &operator<<(ostream &stream, const Event &Event) {
  stream << Event.operator string();
  return stream;
}

#endif // _TYPES_H

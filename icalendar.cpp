#include "icalendar.h"
#include <iostream>
#include <vector>

constexpr bool debug = false;

vector<Days> to_days(string str) {
  vector<Days> d;
  while (str.size() > 0) {
    size_t pos = str.find_first_of(',');
    string item = str.substr(0, pos);

    if (item == "SU")
      d.push_back(SU);
    if (item == "MO")
      d.push_back(MO);
    if (item == "TU")
      d.push_back(TU);
    if (item == "WE")
      d.push_back(WE);
    if (item == "TH")
      d.push_back(TH);
    if (item == "FR")
      d.push_back(FR);
    if (item == "SA")
      d.push_back(SA);
    if (pos != string::npos)
      ++pos;
    str.erase(0, pos);
  }
  return d;
}

void ICalendar::LoadFromFile() {
  string Line, NextLine;
  Component CurrentComponent = VCALENDAR, PrevComponent = VCALENDAR;
  Event *NewEvent = NULL;
  Alarm NewAlarm;
  // for getting some UIDs for events without them
  unsigned int NoUID = 0;
  fstream File;

  File.open(FileName, ios::in | ios::binary);

  if (!File.is_open()) {
    File.clear();
    File.open(FileName, ios::out | ios::binary | ios::trunc);
    File << "BEGIN:VCALENDAR\r\n";
    File << "VERSION:2.0\r\n";
    File << "PRODID:-//Juliusz Gonera//NONSGML remind.me//EN\r\n";
    File << "END:VCALENDAR\r\n";
    File.close();

    return;
  }

  getline(File, NextLine);

  while (!File.eof()) {
    Line = NextLine;
    // lines can be wrapped after 75 octets so we may have to unwrap them
    for (;;) {
      getline(File, NextLine);
      if (NextLine[0] != '\t' && NextLine[0] != ' ')
        break;
      Line += NextLine.substr(1);
    }

    switch (CurrentComponent) {
    case VCALENDAR:
      if (Line.find("BEGIN:VEVENT") == 0 || Line.find("BEGIN:VTODO") == 0) {
        NewEvent = new Event;
        CurrentComponent = VEVENT;
      }
      break;

    case VEVENT:
      if (Line.find("UID") == 0) {
        NewEvent->UID = GetProperty(Line);
      } else if (Line.find("SUMMARY") == 0) {
        NewEvent->Summary = GetProperty(Line);
      } else if (Line.find("COMPLETED") == 0) {
        NewEvent->Completed = GetProperty(Line);
      } else if (Line.find("DTSTAMP") == 0) {
        NewEvent->DtStamp = GetProperty(Line);
      } else if (Line.find("DTSTART") == 0) {
        NewEvent->DtStart = GetProperty(Line);
        NewEvent->DtStart.tzid = GetSubProperty(Line, "TZID");
      } else if (Line.find("DTEND") == 0) {
        NewEvent->DtEnd = GetProperty(Line);
        NewEvent->DtEnd.tzid = GetSubProperty(Line, "TZID");
      } else if (Line.find("DURATION") == 0) {
        NewEvent->DtEnd = NewEvent->DtStart;
        int count;
        char unit;
        // Should try to find a second or third count|unit pair
        if (2 == sscanf(GetProperty(Line).c_str(), "PT%u%c", &count, &unit))
          switch (unit) {
          case 'S':
            NewEvent->DtEnd[SECOND] += count;
            break;
          case 'M':
            NewEvent->DtEnd[MINUTE] += count;
            break;
          case 'H':
            NewEvent->DtEnd[HOUR] += count;
            break;
          case 'D':
            NewEvent->DtEnd[DAY] += count;
            break;
          case 'W':
            NewEvent->DtEnd[WEEK] += count;
            break;
          }
      } else if (Line.find("EXDATE") == 0) {
        Date d;
        d = GetProperty(Line);
        d.tzid = GetSubProperty(Line, "TZID");
        NewEvent->ExDates->push_back(d);
      } else if (Line.find("DESCRIPTION") == 0) {
        NewEvent->Description = GetProperty(Line);
      } else if (Line.find("CATEGORIES") == 0) {
        NewEvent->Categories = GetProperty(Line);
      } else if (Line.find("STATUS") == 0) {
        NewEvent->Status = GetProperty(Line);
      } else if (Line.find("RRULE") == 0) {
        NewEvent->RRule.Freq = ConvertFrequency(GetSubProperty(Line, "FREQ"));
        NewEvent->RRule.Interval =
            atoi(GetSubProperty(Line, "INTERVAL").c_str());
        if (NewEvent->RRule.Interval == 0)
          NewEvent->RRule.Interval = 1;
        NewEvent->RRule.Count = atoi(GetSubProperty(Line, "COUNT").c_str());
        NewEvent->RRule.Until = GetSubProperty(Line, "UNTIL");
        auto wkst = to_days(GetSubProperty(Line, "WKST"));
        if (wkst.size() == 1)
          NewEvent->RRule.WeekStart = wkst[0];
        NewEvent->RRule.ByDay = to_days(GetSubProperty(Line, "BYDAY"));
      } else if (Line.find("BEGIN:VALARM") == 0) {
        NewAlarm.Clear();
        PrevComponent = CurrentComponent;
        CurrentComponent = VALARM;
      } else if (Line.find("END:VEVENT") == 0 || Line.find("END:VTODO") == 0) {
        if (NewEvent->UID.empty())
          NewEvent->UID = NoUID++;

        Events.push_back(NewEvent);
        CurrentComponent = VCALENDAR;
      }
      break;

    case VALARM:
      if (Line.find("ACTION") == 0) {
        NewAlarm.Action = ConvertAlarmAction(GetProperty(Line));
      } else if (Line.find("TRIGGER") == 0) {
        NewAlarm.Trigger = GetProperty(Line);
      } else if (Line.find("DESCRIPTION") == 0) {
        NewAlarm.Description = GetProperty(Line);
      } else if (Line.find("END:VALARM") == 0) {
        NewEvent->Alarms->push_back(NewAlarm);
        CurrentComponent = PrevComponent;
      }
      break;
    }
  }

  File.close();
}

/*Event* ICalendar::GetEventByUID(char *UID) {
        for (list<Event *>::iterator Iterator = Events.begin(); Iterator !=
Events.end(); ++Iterator) { if ((*Iterator)->UID.find(UID) == 0) { return
*Iterator;
                }
        }

        return NULL;
}*/

void ICalendar::AddEvent(Event *NewEvent) {
  char Temp[16];
  string Line;
  streamoff Offset;

  NewEvent->DtStamp.SetToNow();
  NewEvent->UID = NewEvent->DtStamp;
  NewEvent->UID += '-';
  sprintf(Temp, "%d", rand());
  NewEvent->UID += Temp;

  Events.push_back(NewEvent);

  // for some reason tellg() modifies the get pointer under Windows if the file
  // is not opened in the binary mode (possibly because of UTF-8?)
  fstream File(FileName, ios::in | ios::out | ios::binary);

  do {
    Offset = File.tellg();
    getline(File, Line);
  } while (!File.eof() && Line.find("END:VCALENDAR") != 0);
  File.seekp(Offset, ios::beg);

  File << *NewEvent;
  File << "END:VCALENDAR\r\n";

  File.close();
}

void ICalendar::DeleteEvent(Event *DeletedEvent) {
  fstream File;
  string Data, Line, PartialData;
  unsigned int Length;
  bool Copy = true, Deleted = false;

  File.open(FileName, ios::in | ios::binary);
  File.seekg(0, ios::end);
  Length = File.tellg();
  File.seekg(0, ios::beg);

  // to avoid reallocating memory
  Data.reserve(Length);

  while (!File.eof()) {
    getline(File, Line);

    Length = Line.length();
    if (Length <= 1)
      continue;

    // getline() removes only '\n' from the end of the line (not '\r')
    FixLineEnd(Line, Length);

    if (Line.find("BEGIN:VEVENT") == 0 || Line.find("BEGIN:VTODO") == 0) {
      Copy = false;
      Deleted = false;
      PartialData = "";
    } else if (Line.find("UID:") == 0 && Line.find(DeletedEvent->UID) == 4) {
      Deleted = true;
    }

    if (Copy == true)
      Data += Line;
    else if (Deleted == false)
      PartialData += Line;

    if (Line.find("END:VEVENT") == 0 || Line.find("END:VTODO") == 0) {
      Copy = true;

      if (Deleted == false)
        Data += PartialData;
    }
  }

  File.close();
  File.clear();

  // again problems in non-binary mode - "\r\n" changed to "\r\r\n"
  File.open(FileName, ios::out | ios::binary | ios::trunc);
  File << Data;
  File.close();

  for (list<Event *>::iterator Iterator = Events.begin();
       Iterator != Events.end();) {
    if (*Iterator == DeletedEvent) {
      delete *Iterator;
      Events.erase(Iterator++);
    } else
      ++Iterator;
  }
}

void ICalendar::ModifyEvent(Event *ModifiedEvent) {
  fstream File;
  string Data, Line, PartialData;
  unsigned int Length;
  bool Copy = true, Modified = false;

  File.open(FileName, ios::in | ios::binary);
  File.seekg(0, ios::end);
  Length = File.tellg();
  File.seekg(0, ios::beg);

  // we will probably need at least such amount of memory
  Data.reserve(Length);

  while (!File.eof()) {
    getline(File, Line);

    Length = Line.length();
    if (Length <= 1)
      continue;

    // getline() removes only '\n' from the end of the line (not '\r')
    FixLineEnd(Line, Length);

    if (Line.find("BEGIN:VEVENT") == 0 || Line.find("BEGIN:VTODO") == 0) {
      Copy = false;
      Modified = false;
      PartialData = "";
    } else if (Line.find("UID:") == 0 && Line.find(ModifiedEvent->UID) == 4) {
      Modified = true;
      Data += *ModifiedEvent;
    }

    if (Copy == true)
      Data += Line;
    else if (Modified == false)
      PartialData += Line;

    if (Line.find("END:VEVENT") == 0 || Line.find("END:VTODO") == 0) {
      Copy = true;

      if (Modified == false)
        Data += PartialData;
    }
  }

  File.close();
  File.clear();

  // again problems in non-binary mode - "\r\n" changed to "\r\r\n"
  File.open(FileName, ios::out | ios::binary | ios::trunc);
  File << Data;
  File.close();
}

/// ICalendar::Query

Event *ICalendar::Query::GetNextEvent(bool WithAlarm) {
  bool exclude;
  Event *next;
  do {
    next = nullptr;
    static Event *RecurrentEvent = nullptr;
    /* not all events have DtEnd, but we need some DtEnd for various checks,
       so we will use this for temporary DtEnd derived from DtStart (following
       RFC 2445, 4.6.1) */
    Date DtEnd;
    unsigned long Difference;
    unsigned short Rest;

    if (RecurrentEvent != NULL) {

      if (!RecurrentEvent->RRule.ByDay
               .empty()) { // todo: what if freq is less than WEEK?
        while (true) {
          RecurrentEvent->DtStart[DAY] += 1; // next day
          if (RecurrentEvent->DtStart.WeekDay() ==
              RecurrentEvent->RRule.WeekStart) {
            // next interval
            ++RecurrentEvent->RecurrenceNo;
            RecurrentEvent->DtStart = RecurrentEvent->BaseEvent->DtStart;
            RecurrentEvent->DtStart[RecurrentEvent->RRule.Freq] +=
                RecurrentEvent->RecurrenceNo * RecurrentEvent->RRule.Interval;
            if (!RecurrentEvent->DtEnd.IsEmpty()) {
              RecurrentEvent->DtEnd = RecurrentEvent->BaseEvent->DtEnd;
              RecurrentEvent->DtEnd[RecurrentEvent->RRule.Freq] +=
                  RecurrentEvent->RecurrenceNo * RecurrentEvent->RRule.Interval;
            }
          }
          if (find(RecurrentEvent->RRule.ByDay.begin(),
                   RecurrentEvent->RRule.ByDay.end(),
                   RecurrentEvent->DtStart.WeekDay()) !=
              RecurrentEvent->RRule.ByDay.end()) {
            // next maching day found
            break;
          }
        }
      } else {
        RecurrentEvent->DtStart[RecurrentEvent->RRule.Freq] +=
            RecurrentEvent->RRule.Interval;
        if (!RecurrentEvent->DtEnd.IsEmpty())
          RecurrentEvent->DtEnd[RecurrentEvent->RRule.Freq] +=
              RecurrentEvent->RRule.Interval;
        ++RecurrentEvent->RecurrenceNo;
      }

      if (debug)
        std::cout << "Considering (R): " << RecurrentEvent->DtStart.Format()
                  << " " << RecurrentEvent->Summary;
      if ((!WithAlarm && RecurrentEvent->DtStart <= Criteria.To &&
           (RecurrentEvent->RRule.Until.IsEmpty() ||
            RecurrentEvent->RRule.Until >= RecurrentEvent->DtStart) &&
           (RecurrentEvent->RRule.Count == 0 ||
            RecurrentEvent->RecurrenceNo < RecurrentEvent->RRule.Count)) ||
          (WithAlarm && RecurrentEvent->HasAlarm(Criteria.From, Criteria.To))) {
        RecurrentEvents.push_back(new Event(*RecurrentEvent));
        next = RecurrentEvents.back();
        if (debug)
          std::cout << " [match]" << std::endl;
      } else {
        delete RecurrentEvent;
        RecurrentEvent = NULL;
        if (debug)
          std::cout << std::endl;
      }
    }

    if ((RecurrentEvent == nullptr) && (next == nullptr)) {
      for (; EventsIterator != Calendar->Events.end(); ++EventsIterator) {
        if ((*EventsIterator)->DtEnd.IsEmpty()) {
          DtEnd = (*EventsIterator)->DtStart;
          if ((*EventsIterator)->DtStart.WithTime == false)
            ++DtEnd[DAY];
        } else {
          DtEnd = (*EventsIterator)->DtEnd;
        }

        if (debug)
          std::cout << "Considering:     "
                    << (*EventsIterator)->DtStart.Format() << " "
                    << (*EventsIterator)->Summary;
        if (Criteria.AllEvents == true ||
            (!WithAlarm &&
             // DtEnd is non-inclusive (according to RFC 2445)
             (DtEnd > Criteria.From ||
              (*EventsIterator)->DtStart >= Criteria.From) &&
             (*EventsIterator)->DtStart <= Criteria.To) ||
            (WithAlarm &&
             (*EventsIterator)->HasAlarm(Criteria.From, Criteria.To))) {
          // Match
          if (Criteria.AllEvents == false &&
              Criteria.IncludeRecurrent == true &&
              (*EventsIterator)->RRule.IsEmpty() == false)
            RecurrentEvent = new Event(**EventsIterator);
          next = *(EventsIterator++);
          if (debug)
            std::cout << " [match]" << std::endl;
          break;

        } else if (
            // No Match - still needs to check if recurrent matches
            (*EventsIterator)->RRule.IsEmpty() == false &&
            (*EventsIterator)->DtStart < Criteria.From &&
            ((*EventsIterator)->RRule.Until.IsEmpty() ||
             (*EventsIterator)->RRule.Until >= Criteria.From) &&
            Criteria.IncludeRecurrent == true) {
          RecurrentEvent = new Event(**EventsIterator);

          Difference =
              Criteria.From.Difference(DtEnd, RecurrentEvent->RRule.Freq);
          Rest = Difference % RecurrentEvent->RRule.Interval;

          if (Rest != 0)
            Difference += RecurrentEvent->RRule.Interval - Rest;

          // if using byday, step one interval back. Need to check individual
          // days until next interval
          if (!(*EventsIterator)->RRule.ByDay.empty())
            Difference -= RecurrentEvent->RRule.Interval;

          RecurrentEvent->DtStart[RecurrentEvent->RRule.Freq] += Difference;
          DtEnd[RecurrentEvent->RRule.Freq] += Difference;
          RecurrentEvent->RecurrenceNo =
              Difference / RecurrentEvent->RRule.Interval;

          /*
          // DtEnd is non-inclusive (according to RFC 2445)
          while (DtEnd <= Criteria.From) {
             RecurrentEvent->DtStart[RecurrentEvent->RRule.Freq] +=
          RecurrentEvent->RRule.Interval; DtEnd[RecurrentEvent->RRule.Freq] +=
          RecurrentEvent->RRule.Interval;
             ++RecurrentEvent->RecurrenceNo;
          }
          */

          if ((!WithAlarm && RecurrentEvent->DtStart <= Criteria.To &&
               // < because DtStart counts as the first occurence
               (RecurrentEvent->RRule.Count == 0 ||
                RecurrentEvent->RecurrenceNo < RecurrentEvent->RRule.Count)) ||
              (WithAlarm &&
               RecurrentEvent->HasAlarm(Criteria.From, Criteria.To))) {
            ++EventsIterator;
            if (!RecurrentEvent->DtEnd.IsEmpty())
              RecurrentEvent->DtEnd = DtEnd;
            RecurrentEvents.push_back(new Event(*RecurrentEvent));
            next = RecurrentEvents.back();
            if (debug)
              std::cout << " [match]" << std::endl;
            break;
          }

          delete RecurrentEvent;
          RecurrentEvent = NULL;
        }
        if (debug)
          std::cout << std::endl;
      }
    }

    exclude =
        next && (std::find_if(next->ExDates->begin(), next->ExDates->end(),
                              [next](const Date &d) {
                                return d == next->DtStart;
                              }) != next->ExDates->end());
    if (debug) {
      if (exclude)
        std::cout << "Excluded:        " << next->DtStart.Format() << " "
                  << next->Summary << std::endl;
      else if (next != nullptr)
        std::cout << "Found:           " << next->DtStart.Format() << " "
                  << next->Summary << std::endl;
    }

  } while (exclude);
  return next;
}

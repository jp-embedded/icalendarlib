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

// test file

#include "icalendar.h"
#include <ctime>
#include <fstream>
#include <iostream>

using namespace std;

int main() {
  Date a, b;

  a = "20080627T140000";
  b = "20080626T100000";

  a[MINUTE] -= 65;

  cout << a << endl;
  cout << a.Difference(b, HOUR) << endl;

  ICalendar Calendar("calendar.ics");
  Event *CurrentEvent;
  ICalendar::Query SearchQuery(&Calendar);

  srand(time(NULL));

  SearchQuery.Criteria.From = "20100927T000000";
  SearchQuery.Criteria.To = "20100927T235959";
  // SearchQuery.Criteria.To[HOUR] = 23;
  // SearchQuery.Criteria.To[MINUTE] = 59;
  // SearchQuery.Criteria.To[SECOND] = 59;

  SearchQuery.ResetPosition();

  while ((CurrentEvent = SearchQuery.GetNextEvent(false)) != NULL) {
    cout << CurrentEvent->UID << endl;
    cout << CurrentEvent->DtStart.Format() << endl;
    cout << CurrentEvent->DtEnd.Format() << endl;
    cout << CurrentEvent->Summary << endl;
    cout << CurrentEvent->Categories << endl;
    cout << "\t" << CurrentEvent->RRule.Freq << endl;
    cout << "\t" << CurrentEvent->RRule.Interval << endl;
    cout << "\t" << CurrentEvent->RRule.Until << endl << endl;

    // CurrentEvent->BaseEvent->Description = "aasfjanfkjahsf";
    // Calendar.ModifyEvent(CurrentEvent->BaseEvent);
  }

  Event *NewEvent = new Event;
  NewEvent->Summary = "test";
  a.SetToNow();
  NewEvent->DtStart = a;
  Calendar.AddEvent(NewEvent);

  return 0;
}

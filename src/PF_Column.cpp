// =====================================================================================
// 
//       Filename:  PF_Column.cpp
// 
//    Description:  implementation of class which implements Point & Figure column data
// 
//        Version:  1.0
//        Created:  2021-07-26 09:36 AM 
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================


	/* This file is part of PF_CollectData. */

	/* PF_CollectData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* PF_CollectData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with PF_CollectData.  If not, see <http://www.gnu.org/licenses/>. */

//-----------------------------------------------------------------------------
// 
// Logic for how to construct a column given incoming data comes from the
// excellent book "The Definitive Guide to Point and Figure" by
// Jeremy du Plessis.
// This book is written by an engineer/programmer so the alogorithm descriptions
// can be readily used to write code.  However, the implementation along with 
// responsibility for any errors is mine.
//
//-----------------------------------------------------------------------------

#include <exception>
#include <memory>

#include <fmt/format.h>

#include "Boxes.h"
#include "DDecQuad.h"
#include "PF_Column.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Column::PF_Column(Boxes* boxes, int reversal_boxes,
            Direction direction, DprDecimal::DDecQuad top, DprDecimal::DDecQuad bottom)
    : boxes_{boxes}, reversal_boxes_{reversal_boxes}, top_{top}, bottom_{bottom}, direction_{direction}
{
}  // -----  end of method PF_Column::PF_Column  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Column
//      Method:  PF_Column
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Column::PF_Column (Boxes* boxes, const Json::Value& new_data)
    : boxes_{boxes}
{
    this->FromJSON(new_data);
}  // -----  end of method PF_Column::PF_Column  (constructor)  ----- 

PF_Column PF_Column::MakeReversalColumn (Direction direction, const DprDecimal::DDecQuad& value,
        tpt the_time)
{
    auto new_column = PF_Column{boxes_, reversal_boxes_, direction, value, value};
    new_column.time_span_ = {the_time, the_time};
    return new_column;
}		// -----  end of method PF_Column::MakeReversalColumn  ----- 

bool PF_Column::operator== (const PF_Column& rhs) const
{
    return rhs.reversal_boxes_ == reversal_boxes_ && rhs.direction_ == direction_
        && rhs.top_ == top_ && rhs.bottom_ == bottom_ && rhs.had_reversal_ == had_reversal_;
}		// -----  end of method PF_Column::operator==  ----- 

PF_Column::AddResult PF_Column::AddValue (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    if (top_ == -1 && bottom_ == -1)
    {
        // OK, first time here for this column.

        return StartColumn(new_value, the_time);
    }

    // OK, we've got a value but may not yet have a direction.

    if (direction_ == Direction::e_unknown)
    {
        return TryToFindDirection(new_value, the_time);
    }

    // If we're here, we have direction. We can either continue in 
    // that direction, ignore the value or reverse our direction 
    // in which case, we start a new column (unless this is 
    // a 1-box reversal and we can revers in place)

    if (direction_ == Direction::e_up)
    {
        return TryToExtendUp(new_value, the_time);
    }
    return TryToExtendDown(new_value, the_time);
}		// -----  end of method PF_Column::AddValue  ----- 

PF_Column::AddResult PF_Column::StartColumn (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    // As this is the first entry in the column, just set fields 
    // to the input value rounded down to the nearest box value.

    top_= boxes_->FindBox(new_value);
    bottom_ = top_;
    time_span_ = {the_time, the_time};

    return {Status::e_accepted, std::nullopt};
}		// -----  end of method PF_Column::StartColumn  ----- 


PF_Column::AddResult PF_Column::TryToFindDirection (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    // NOTE: Since a new value may gap up or down, we could 
    // have multiple boxes to fill in. 

    // we can compare to either value since they 
    // are both the same at this point.

    Boxes::Box possible_value = boxes_->FindBox(new_value);

    if (possible_value > top_)
    {
        direction_ = Direction::e_up;
        top_ = possible_value;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }
    if (possible_value < bottom_)
    {
        direction_ = Direction::e_down;
        bottom_ = possible_value;
        time_span_.second = the_time;
        return {Status::e_accepted, std::nullopt};
    }

    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToFindDirection  ----- 

PF_Column::AddResult PF_Column::TryToExtendUp (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    // if we are going to extend the column up, then we need to move up by at least 1 box.

    Boxes::Box possible_new_top = boxes_->FindNextBox(top_);
    if (new_value >= possible_new_top)
    {
        // OK, up we go...

        while (possible_new_top <= new_value)
        {
            top_ = possible_new_top;
            possible_new_top = boxes_->FindNextBox(top_);
        }

        return {Status::e_accepted, std::nullopt};
    }

    // look for a reversal down 

    Boxes::Box possible_new_column_top = boxes_->FindPrevBox(top_);

    if (new_value <= possible_new_column_top)
    {
        // look for 1-step back reversal.

        if (reversal_boxes_ == 1)
        {
            if (bottom_ < top_)
            {
                // there are at least 2 boxes in column so 
                // can't do 1-box reversal 

                time_span_.second = the_time;
                // can't do it as box is occupied.
                return {Status::e_reversal, MakeReversalColumn(Direction::e_down, possible_new_column_top, the_time)};
            }
            // OK, down we go in column reversal...

            // for clarity 
            auto possible_new_column_bottom = possible_new_column_top;
            while (possible_new_column_bottom <= new_value)
            {
                bottom_ = possible_new_column_bottom;
                possible_new_column_bottom = boxes_->FindPrevBox(bottom_);
            }

//            bottom_ = possible_new_column_top;
            had_reversal_ = true;
            direction_ = Direction::e_down;
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        // multi-box reversal, let's see if we can do it.
        // we've already gone down a box above, so terminate the loop 
        // at 1.

        for (auto x = reversal_boxes_; x > 1; --x) 
        {
            possible_new_column_top = boxes_->FindPrevBox(possible_new_column_top);
        }

        if (new_value <= possible_new_column_top)
        {
            time_span_.second = the_time;
            return {Status::e_reversal, MakeReversalColumn(Direction::e_down, boxes_->FindPrevBox(top_), the_time)};
        }
    }

//    Boxes::Box possible_box = boxes_->FindBox(new_value);
//
//    if (possible_box > top_)
//    {
//        top_ = possible_box;
//        time_span_.second = the_time;
//        return {Status::e_accepted, std::nullopt};
//    }
//    // look for a reversal 
//
//    // if the new value is in the 'middle' of a box ( > than lower bound)
//    // then we need to use the next box up as our test value since we need 
//    // to be at least a whole box down.
//
//    if (new_value > possible_box)
//    {
//        // ok, we need to move up a box 
//
//        possible_box = boxes_->FindNextBox(possible_box);
//    }
//
//    Boxes::Box reversal_box = top_;
//    for (auto x = reversal_boxes_; x > 0; --x) 
//    {
//        reversal_box = boxes_->FindPrevBox(reversal_box);
//    }
//    if (possible_box <= reversal_box)
//    {
//        // look for one-step-back reversal first.
//
//        if (reversal_boxes_ == 1)
//        {
//            if (bottom_ < top_)
//            {
//                // there are at least 2 boxes in column so 
//                // can't do 1-box reversal 
//
//                time_span_.second = the_time;
//                // can't do it as box is occupied.
//                return {Status::e_reversal, MakeReversalColumn(Direction::e_down, reversal_box, the_time)};
//            }
//            bottom_ = possible_box;
//            had_reversal_ = true;
//            direction_ = Direction::e_down;
//            time_span_.second = the_time;
//            return {Status::e_accepted, std::nullopt};
//        }
//        time_span_.second = the_time;
//        return {Status::e_reversal, MakeReversalColumn(Direction::e_down, boxes_->FindPrevBox(top_), the_time)};
//    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Chart::TryToExtendUp  ----- 


PF_Column::AddResult PF_Column::TryToExtendDown (const DprDecimal::DDecQuad& new_value, tpt the_time)
{
    // if we are going to extend the column down, then we need to move down by at least 1 box.

    Boxes::Box possible_new_bottom = boxes_->FindPrevBox(bottom_);
    if (new_value <= possible_new_bottom)
    {
        // OK, down we go...

        while (possible_new_bottom >= new_value)
        {
            bottom_ = possible_new_bottom;
            possible_new_bottom = boxes_->FindPrevBox(bottom_);
        }

        return {Status::e_accepted, std::nullopt};
    }

    // look for a reversal up 

    Boxes::Box possible_new_column_bottom = boxes_->FindNextBox(bottom_);

    if (new_value >= possible_new_column_bottom)
    {
        // look for 1-step back reversal.

        if (reversal_boxes_ == 1)
        {
            if (bottom_ < top_)
            {
                // there are at least 2 boxes in column so 
                // can't do 1-box reversal 

                time_span_.second = the_time;
                // can't do it as box is occupied.
                return {Status::e_reversal, MakeReversalColumn(Direction::e_up, possible_new_column_bottom, the_time)};
            }
            // OK, up we go in column reversal...

            // for clarity 
            auto possible_new_column_top = possible_new_column_bottom;
            while (possible_new_column_top <= new_value)
            {
                top_ = possible_new_column_top;
                possible_new_column_top = boxes_->FindNextBox(top_);
            }

//            bottom_ = possible_new_column_top;
            had_reversal_ = true;
            direction_ = Direction::e_up;
            time_span_.second = the_time;
            return {Status::e_accepted, std::nullopt};
        }
        // multi-box reversal, let's see if we can do it.
        // we've already gone up a box above, so terminate the loop 
        // at 1.

        for (auto x = reversal_boxes_; x > 1; --x) 
        {
            possible_new_column_bottom = boxes_->FindNextBox(possible_new_column_bottom);
        }

        if (new_value >= possible_new_column_bottom)
        {
            time_span_.second = the_time;
            return {Status::e_reversal, MakeReversalColumn(Direction::e_up, boxes_->FindNextBox(bottom_), the_time)};
        }
    }

//    Boxes::Box possible_box = boxes_->FindBox(new_value);
//
//    // to extend down a box, the new value must be <= the box 
//    // value so let's make sure it is.
//
//    if (new_value > possible_box)
//    {
//        // ok, we need to move up a box 
//
//        possible_box = boxes_->FindNextBox(possible_box);
//    }
//
//    if (possible_box < bottom_)
//    {
//        bottom_ = possible_box;
//        time_span_.second = the_time;
//        return {Status::e_accepted, std::nullopt};
//    }
//
//    // look for a reversal 
//
//    // if we might be moving up, then reset this value 
//
//    possible_box = boxes_->FindBox(new_value);
//    Boxes::Box reversal_box = bottom_;
//    for (auto x = reversal_boxes_; x > 0; --x) 
//    {
//        reversal_box = boxes_->FindNextBox(reversal_box);
//    }
//    if (possible_box >= reversal_box)
//    {
//        // look for one-step-back reversal first.
//
//        if (reversal_boxes_ == 1)
//        {
//            if (top_ > bottom_)
//            {
//                // there are at least 2 boxes in column so 
//                // can't do 1-box reversal
//
//                time_span_.second = the_time;
//                // can't do it as box is occupied.
//                return {Status::e_reversal, MakeReversalColumn(Direction::e_up, reversal_box, the_time)};
//            }
//            top_ = possible_box;
//            had_reversal_ = true;
//            direction_ = Direction::e_up;
//            time_span_.second = the_time;
//            return {Status::e_accepted, std::nullopt};
//        }
//        time_span_.second = the_time;
//        return {Status::e_reversal, MakeReversalColumn(Direction::e_up, boxes_->FindNextBox(bottom_), the_time)};
//    }
    return {Status::e_ignored, std::nullopt};
}		// -----  end of method PF_Column::TryToExtendDown  ----- 

Json::Value PF_Column::ToJSON () const
{
    Json::Value result;

    result["start_at"] = time_span_.first.time_since_epoch().count();
    result["last_entry"] = time_span_.second.time_since_epoch().count();

    result["reversal_boxes"] = reversal_boxes_;
    result["top"] = top_.ToStr();
    result["bottom"] = bottom_.ToStr();

    switch(direction_)
    {
        using enum Direction;
        case e_unknown:
            result["direction"] = "unknown";
            break;

        case e_down:
            result["direction"] = "down";
            break;

        case e_up:
            result["direction"] = "up";
            break;
    };

    result["had_reversal"] = had_reversal_;
    return result;
}		// -----  end of method PF_Column::ToJSON  ----- 

void PF_Column::FromJSON (const Json::Value& new_data)
{
    time_span_.first = tpt{std::chrono::nanoseconds{new_data["start_at"].asInt64()}};
    time_span_.second = tpt{std::chrono::nanoseconds{new_data["last_entry"].asInt64()}};

    reversal_boxes_ = new_data["reversal_boxes"].asInt();
    top_ = DprDecimal::DDecQuad{new_data["top"].asString()};
    bottom_ = DprDecimal::DDecQuad{new_data["bottom"].asString()};

    const auto direction = new_data["direction"].asString();
    if (direction == "up")
    {
        direction_ = Direction::e_up;
    }
    else if (direction == "down")
    {
        direction_ = Direction::e_down;
    }
    else if (direction == "unknown")
    {
        direction_ = Direction::e_unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid direction provided: {}. Must be 'up', 'down', 'unknown'.", direction)};
    }

    had_reversal_ = new_data["had_reversal"].asBool();
    
}		// -----  end of method PF_Column::FromJSON  ----- 


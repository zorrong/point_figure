// =====================================================================================
//
//       Filename:  PF_Signals.h
//
//    Description:  Place for code related to finding trading 'signals'
//                  in PF_Chart data.
//
//        Version:  1.0
//        Created:  09/21/2022 09:26:12 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================


#ifndef  PF_SIGNALS_INC
#define  PF_SIGNALS_INC

#include <optional>
#include <vector>

#include <date/date.h>
#include <date/tz.h>

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <json/json.h>

#include "DDecQuad.h"
#include "utilities.h"

class PF_Chart;

enum class PF_SignalCategory {e_PF_Unknown, e_PF_Buy, e_PF_Sell};
enum class PF_SignalType {e_PF_Unknown, e_DoubleTop_Buy, e_DoubleBottom_Sell, e_TripleTop_Buy, e_TripleBottom_Sell};

struct PF_Signal
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Unknown;
    PF_SignalType signal_type_ = PF_SignalType::e_PF_Unknown;
    date::utc_time<date::utc_clock::duration> tpt_ = {};
    int32_t column_number_ = -1;
    DprDecimal::DDecQuad signal_price_ = -1;
    DprDecimal::DDecQuad box_ = -1;
};

using PF_SignalList = std::vector<PF_Signal>;

[[nodiscard]] Json::Value PF_SignalToJSON(const PF_Signal& signal);
[[nodiscard]] PF_Signal PF_SignalFromJSON(const Json::Value& new_data);

// here are some signals we can look for.

struct PF_DoubleTopBuy
{
    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time);
};

struct PF_TripleTopBuy
{
    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time);
};

// custom formatter 

template <> struct fmt::formatter<PF_Signal>: formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Signal& signal, fmt::format_context& ctx)
    {
        std::string s;
        std::string sig_type;
        
        switch (signal.signal_type_)
        {
            using enum PF_SignalType;
            case e_PF_Unknown:
                sig_type = "Unknown";
                break;
            case e_DoubleTop_Buy:
                sig_type = "DblTop Buy";
                break;
            case e_DoubleBottom_Sell:
                sig_type = "DblBtm Sell";
                break;
            case e_TripleTop_Buy:
                sig_type = "TripleTop Buy";
                break;
            case e_TripleBottom_Sell:
                sig_type = "TripleBtm Sell";
                break;
        }
        fmt::format_to(std::back_inserter(s), "category: {}. type: {}. time: {:%F %X}. col: {}. price {} box: {}.\n",
           (signal.signal_category_ == PF_SignalCategory::e_PF_Buy ? "Buy" : signal.signal_category_ == PF_SignalCategory::e_PF_Sell ? "Sell" : "Unknown"),
           sig_type,
           signal.tpt_,
           signal.column_number_,
           signal.signal_price_,
           signal.box_
       );

        return formatter<std::string>::format(s, ctx);
    }
};

#endif   // ----- #ifndef PF_SIGNALS_INC  ----- 
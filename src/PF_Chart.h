// =====================================================================================
// 
//       Filename:  p_f_data.h
// 
//    Description:  class to manage Point & Figure data for a symbol.
// 
//        Version:  2.0
//        Created:  2021-07-29 08:29 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================

// =====================================================================================
//        Class:  PF_Chart
//  Description:  class to manage Point & Figure data for a symbol.
// =====================================================================================

#ifndef  PF_CHART_INC
#define  PF_CHART_INC


#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <date/date.h>

namespace fs = std::filesystem;

//using namespace std::chrono_literals;

#include "PF_Column.h"


class PF_Chart
{
    using tpt = std::chrono::time_point<std::chrono::steady_clock>;

public:

    using Y_Limits = std::pair<int32_t, int32_t>;

    // make it look like a range

    using const_iterator = std::vector<PF_Column>::const_iterator;

    // ====================  LIFECYCLE     =======================================
    PF_Chart () = default;                             // constructor
    PF_Chart(const std::string& symbol, std::int32_t boxsize, int32_t reversal_boxes);

    // ====================  ACCESSORS     =======================================

    void ExportData(std::ostream* output_data);
    [[nodiscard]] PF_Column::Direction GetCurrentDirection() const { return current_direction_; }

    [[nodiscard]] std::size_t GetNumberOfColumns() const { return columns_.size(); }

    [[nodiscard]] Y_Limits GetYLimits() const { return {y_min_, y_max_}; }

    [[nodiscard]] const_iterator begin() const { return columns_.begin(); }
    [[nodiscard]] const_iterator end() const { return columns_.end(); }

    void ConstructChartAndWriteToFile(fs::path output_filename) const;

    // ====================  MUTATORS      =======================================
    
    template<typename T>
    void LoadData(std::istream* input_data);

    // ====================  OPERATORS     =======================================

    const PF_Column& operator[](size_t which) const { return columns_[which]; }

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================

    std::vector<PF_Column> columns_;
    std::unique_ptr<PF_Column> current_column_;

    std::string symbol_;

    tpt mFirstDate;			//	earliest entry for symbol
    tpt mLastChangeDate;		//	date of last change to data
    tpt mLastCheckedDate;	//	last time checked to see if update needed

    int32_t boxsize_;
    int32_t reversal_boxes_;
    int32_t y_min_ = std::numeric_limits<int32_t>::max();
    int32_t y_max_ = std::numeric_limits<int32_t>::min();

    PF_Column::Direction current_direction_;

}; // -----  end of class PF_Chart  -----

template<typename T>
void PF_Chart::LoadData (std::istream* input_data)
{
    // for now, just assume its numbers.

    current_column_ = std::make_unique<PF_Column>(boxsize_, reversal_boxes_);

    while ( ! input_data->eof())
    {
        T price;
        *input_data >> price;
        if (input_data->fail())
        {
            continue;
        }
//        std::cout << price << '\n';
        auto [status, new_col] = current_column_->AddValue(DprDecimal::DDecDouble(price));

        if (current_column_->GetTop() > y_max_)
        {
            y_max_ = current_column_->GetTop();
        }
        if (current_column_->GetBottom() < y_min_)
        {
            y_min_ = current_column_->GetBottom();
        }

        if (status == PF_Column::Status::e_reversal)
        {
            auto* save_col = current_column_.get();         // non-owning access
            columns_.push_back(*save_col);
            current_column_ = std::move(new_col.value());

            // now continue on processing the value.
            
            status = current_column_->AddValue(DprDecimal::DDecDouble(price)).first;
            current_direction_ = current_column_->GetDirection();
        }
    }

    // make sure we keep the last column we were working on 

    if (current_column_->GetTop() > y_max_)
    {
        y_max_ = current_column_->GetTop();
    }
    if (current_column_->GetBottom() < y_min_)
    {
        y_min_ = current_column_->GetBottom();
    }

    columns_.push_back(*current_column_);

    PF_Column* col = current_column_.release();
    delete col;
}
#endif   // ----- #ifndef PF_CHART_INC  ----- 


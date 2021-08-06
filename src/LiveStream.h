// =====================================================================================
//
//       Filename:  LiveStream.h
//
//    Description:  class to live stream ticker updatas 
//
//        Version:  1.0
//        Created:  08/06/2021 09:26:57 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef  LIVESTREAM_INC
#define  LIVESTREAM_INC

#include <string>
#include <vector>

#include "DDecDouble.h"

// =====================================================================================
//        Class:  LiveStream
//  Description:  live stream ticker updates -- look like a generator 
// =====================================================================================
#include <vector>
class LiveStream
{
public:


    // ====================  LIFECYCLE     ======================================= 
    LiveStream () = default;                             // constructor 
    LiveStream (const std::string& host, const std::string& port, const std::string& prefix,
            const std::string& symbols);

    // ====================  ACCESSORS     ======================================= 

    // ====================  MUTATORS      ======================================= 

    // ====================  OPERATORS     ======================================= 

protected:
    // ====================  METHODS       ======================================= 

    // ====================  DATA MEMBERS  ======================================= 

private:
    // ====================  METHODS       ======================================= 

    // ====================  DATA MEMBERS  ======================================= 

    std::vector<std::string> symbol_list_;
    std::string host_;
    std::string port_;
    std::string websocket_prefix_;

}; // -----  end of class LiveStream  ----- 

#endif   // ----- #ifndef LIVESTREAM_INC  ----- 

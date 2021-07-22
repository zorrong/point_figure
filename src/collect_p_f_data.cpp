// =====================================================================================
// 
//       Filename:  collect_p_f_data.cpp
// 
//    Description:  This program will compute Point & Figure data for the
//    				given input file.
// 
//        Version:  2.0
//        Created:  2021-07-20 11:50 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company: 
// 
// =====================================================================================

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <fstream>

#include <date/tz.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
//#include <decDouble.h>

#include "collect_p_f_data.h"
#include "p_f_data.h"
#include "aLine.h"
//#include "DDecimal_16.h"
//#include "DDecimal_32.h"


bool CMyApp::had_signal_ = false;

// utility to convert a date::year_month_day to a string
// using Howard Hinnant's date library

inline std::string LocalDateTimeAsString(std::chrono::system_clock::time_point a_date_time)
{
    auto t = date::make_zoned(date::current_zone(), a_date_time);
    std::string ts = date::format("%a, %b %d, %Y at %I:%M:%S %p %Z", t);
    return ts;
}

//--------------------------------------------------------------------------------------
//       Class:  CMyApp
//      Method:  CMyApp
// Description:  constructor
//--------------------------------------------------------------------------------------
CMyApp::CMyApp (int argc, char* argv[])
    : mArgc{argc}, mArgv{argv},
	mSource{source::unknown}, mDestination{destination::unknown}, mMode{mode::unknown}, mInterval{interval::unknown}, 
	mScale{scale::unknown}, mInputIsPath{false}, mOutputIsPath{false} 

{
}  // -----  end of method CMyApp::CMyApp  (constructor)  -----

bool CMyApp::Startup ()
{
	return true ;
}		// -----  end of method CMyApp::Do_StartUp  -----


bool CMyApp::CheckArgs ()
{
	//	let's get our input and output set up
	
	;//dfail_if_(! mVariableMap.count("symbol"), "Symbol must be specified.");
	mSymbol = mVariableMap["symbol"].as<std::string>();

	if(mVariableMap.count("file") == 0)
	{
		mSource = source::stdin;
		mInputIsPath = false;
	}
	else
	{
		std::string temp = mVariableMap["file"].as<std::string>();
		if (temp == "-")
		{
			mSource = source::stdin;
			mInputIsPath = false;
		}
		else
		{
			mSource = source::file;
			mInputPath = temp;
			;//dfail_if_(! fs::exists(mInputPath), "Can't find input file: ", mInputPath.c_str());
		}
	}

	//	if not specified, assume we are doing a 'load' operation for symbol.
	
	if (mVariableMap.count("mode") == 0)
	{
		mMode = mode::load;
	}
	else
	{
		std::string temp = mVariableMap["mode"].as<std::string>();
		if (temp == "load")
			mMode = mode::load;
		else if (temp == "update")
			mMode = mode::update;
		else
			;//dfail_msg("Unkown 'mode': ", temp.c_str());
	}

	//	if not specified, assume we are sending output to stdout
	
	if (mVariableMap.count("destination") == 0)
	{
		mDestination = destination::stdout;
		mOutputIsPath = false;
	}
	else
	{
		std::string temp = mVariableMap["destination"].as<std::string>();
		if (temp == "file")
		{
			mDestination = destination::file;
			;//dfail_if_(! mVariableMap.count("output"), "Output destination of 'file' specified but no 'output' name provided.");
			std::string temp = mVariableMap["output"].as<std::string>();
			if (temp == "-")
			{
				mOutputIsPath = false;
				mDestination = destination::stdout;
			}
			else
			{
				mOutputPath = temp;
				mOutputIsPath = true;
			}
		}
		else if (temp == "DB")
		{
			mDestination = destination::DB;
			;//dfail_if_(! mVariableMap.count("output"), "Output destination of 'DB' specified but no 'output' table name provided.");
			std::string temp = mVariableMap["output"].as<std::string>();
			;//dfail_if_(temp == "-", "Invalid DB table name specified: '-'.");
			mDBName = temp;
		}
		else
			;//dfail_msg("Invalid destination type specified. Must be: 'file' or 'DB'.");

	}

	;//dfail_if_(! mVariableMap.count("boxsize"), "'Boxsize must be specified.");
	DprDecimal::DDecDouble boxsize = mVariableMap["boxsize"].as<DprDecimal::DDecDouble>();
	mBoxSize = boxsize;

	if (mVariableMap.count("reversal") == 0)
		mReversalBoxes = 1;
	else
		mReversalBoxes = mVariableMap["reversal"].as<int>();

	if (mVariableMap.count("scale") == 0)
		mScale = scale::arithmetic;
	else
	{
		std::string temp = mVariableMap["reversal"].as<std::string>();
		if (temp == "arithmetic")
			mScale = scale::arithmetic;
		else if (temp == "log")
			mScale = scale::log;
		else
			;//dfail_msg("Scale must be either 'arithmetic' or 'log'.");
	}
	return true ;
}		// -----  end of method CMyApp::Do_CheckArgs  -----

void CMyApp::SetupProgramOptions ()
{
	mNewOptions->add_options()
		("help",											"produce help message")
		("symbol,s",			po::value<std::string>(),	"name of symbol we are processing data for")
		("file,f",				po::value<std::string>(),	"name of file containing data for symbol. Default is stdin")
		("mode,m",				po::value<std::string>(),	"mode: either 'load' new data or 'update' existing data. Default is 'load'")
		("output,o",			po::value<std::string>(),	"output file name")
		("destination,d",		po::value<std::string>(),	"send data to file or DB. Default is 'stdout'.")
		("boxsize,b",			po::value<DprDecimal::DDecDouble>(),	"box step size. 'n', 'm.n'")
		("reversal,r",			po::value<int>(),			"reversal size in number of boxes. Default is 1")
		("scale",				po::value<std::string>(),	"'arithmetic', 'log'. Default is 'arithmetic'")
		("interval,i",			po::value<std::string>(),	"'eod', 'tic', '1sec', '5sec', '1min', '5min', etc. Default is 'tic'")
		;

	return ;
}		// -----  end of method CMyApp::Do_SetupProgramOptions  -----


void CMyApp::ParseProgramOptions (const std::vector<std::string>& tokens)
{
    if (tokens.empty())
    {
	    auto options = po::parse_command_line(mArgc, mArgv, *mNewOptions);
        po::store(options, mVariableMap);
        if (this->mArgc == 1 ||	mVariableMap.count("help") == 1)
        {
            std::cout << *mNewOptions << "\n";
            throw std::runtime_error("\nExiting after 'help'.");
        }
    }
    else
    {
        auto options = po::command_line_parser(tokens).options(*mNewOptions).run();
        po::store(options, mVariableMap);
        if (mVariableMap.count("help") == 1)
        {
            std::cout << *mNewOptions << "\n";
            throw std::runtime_error("\nExiting after 'help'.");
        }
    }
	po::notify(mVariableMap);    
}		/* -----  end of method ExtractorApp::ParseProgramOptions  ----- */


std::tuple<int, int, int> CMyApp::Run()
{
	// open a stream on the specified input source.
	
	std::istream* theInput{nullptr};
	std::ifstream iFile;

	if (mSource == source::stdin)
		theInput = &std::cin;
	else if (mSource == source::file)
	{
		iFile.open(mInputPath.string(), std::ios_base::in | std::ios_base::binary);
		;//dfail_if_(! iFile.is_open(), "Unable to open input file: ", mInputPath.c_str());
		theInput = &iFile;
	}
	else
		;//dfail_msg("Unspecified input.");

	std::istream_iterator<aLine> itor{*theInput};
	std::istream_iterator<aLine> itor_end;

	std::ostream* theOutput{nullptr};
	std::ofstream oFile;

	if (mDestination == destination::stdout)
		theOutput = &std::cout;
	else
	{
		oFile.open(mOutputPath.string(), std::ios::out | std::ios::binary);
		;//dfail_if_(! oFile.is_open(), "Unable to open output file: ", mOutputPath.c_str());
		theOutput = &oFile;
	}

	std::ostream_iterator<aLine> otor{*theOutput, "\n"};

	std::copy(itor, itor_end, otor);

	// sampel code using a lambda
	/* std:transform(itor, itor_end, otor, */
	/* 			[] (const aLine& data) {DDecimal<16> aa(data.lineData); aLine bb; bb.lineData = aa.ToStr(); return bb; }); */


	//	play with decimal support in c++11
	
	return {} ;
}		// -----  end of method CMyApp::Do_Run  -----

void CMyApp::Shutdown ()
{
    spdlog::info(fmt::format("\n\n*** End run {} ***\n", LocalDateTimeAsString(std::chrono::system_clock::now())));
}       // -----  end of method ExtractorApp::Shutdown  -----

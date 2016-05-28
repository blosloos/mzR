//
// $Id: cpp_cli_utilities.hpp 6141 2014-05-05 21:03:47Z chambm $
//
//
// Original author: Matt Chambers <matt.chambers .@. vanderbilt.edu>
//
// Copyright 2010 Vanderbilt University - Nashville, TN 37232
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#ifndef _CPP_CLI_UTILITIES_HPP_
#define _CPP_CLI_UTILITIES_HPP_


#include <gcroot.h>
#include <vcclr.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include "automation_vector.h"


namespace pwiz {
namespace util {


inline std::string ToStdString(System::String^ source)
{
    if (System::String::IsNullOrEmpty(source))
        return std::string();

    System::Text::Encoding^ encoding = System::Text::Encoding::UTF8;
    array<System::Byte>^ encodedBytes = encoding->GetBytes(source);

    std::string target("", encodedBytes->Length);
    char* buffer = &target[0];
    unsigned char* unsignedBuffer = reinterpret_cast<unsigned char*>(buffer);
    System::Runtime::InteropServices::Marshal::Copy(encodedBytes, 0, (System::IntPtr) unsignedBuffer, encodedBytes->Length);
	return target;
}


inline System::String^ ToSystemString(const std::string& source, bool utf8=true)
{
    if (utf8)
    {
        System::Text::Encoding^ encoding = System::Text::Encoding::UTF8;
        int length = source.length();
        array<System::Byte>^ buffer = gcnew array<System::Byte>(length);
        System::Runtime::InteropServices::Marshal::Copy((System::IntPtr) const_cast<char*>(source.c_str()), buffer, 0, length);
        return encoding->GetString(buffer);
    }
    else
        return gcnew System::String(source.c_str());
}


template<typename managed_value_type, typename native_value_type>
void ToStdVector(cli::array<managed_value_type>^ managedArray, std::vector<native_value_type>& stdVector)
{
    stdVector.clear();
    if (managedArray->Length > 0)
    {
        cli::pin_ptr<managed_value_type> pin = &managedArray[0];
        native_value_type* begin = (native_value_type*) pin;
        stdVector.assign(begin, begin + managedArray->Length);
    }
}


template<typename managed_value_type, typename native_value_type>
void ToStdVector(System::Collections::Generic::IList<managed_value_type>^ managedList, std::vector<native_value_type>& stdVector)
{
    stdVector.clear();
    if (managedList->Count > 0)
    {
        stdVector.reserve(managedList->Count);
        for (size_t i = 0, end = managedList->Count; i < end; ++i)
            stdVector.push_back((native_value_type) managedList[i]);
    }
}


/// wraps a managed array in an automation_vector to enable direct access from unmanaged code
template<typename managed_value_type, typename native_value_type>
void ToAutomationVector(cli::array<managed_value_type>^ managedArray, automation_vector<native_value_type>& automationArray)
{
    VARIANT v;
    ::VariantInit(&v);
    System::IntPtr vPtr = (System::IntPtr) &v;
    System::Runtime::InteropServices::Marshal::GetNativeVariantForObject((System::Object^) managedArray, vPtr);
    automationArray.attach(v);
}


} // namespace util
} // namespace pwiz


/// forwards managed exception to unmanaged code;
/// prepends function with a single level of scope,
/// e.g. "Reader::read()" instead of "pwiz::data::msdata::Reader::read()"
#define CATCH_AND_FORWARD \
    catch (std::exception&) {throw;} \
    catch (System::Exception^ e) \
    { \
        std::vector<boost::iterator_range<std::string::const_iterator> > tokens; \
        std::string function(__FUNCTION__); \
        boost::algorithm::split(tokens, function, boost::is_any_of(":"), boost::algorithm::token_compress_on); \
        std::string what("["); \
        if (tokens.size() > 1) \
        { \
            boost::range::copy(*(tokens.rbegin()+1), std::back_inserter(what)); \
            what += "::"; \
            if (boost::range::equal(*(tokens.rbegin()+1), *tokens.rbegin())) \
                what += "ctor"; \
            else if (tokens.rbegin()->front() == '~') \
                what += "dtor"; \
            else \
                boost::range::copy(*tokens.rbegin(), std::back_inserter(what)); \
        } \
        else if (tokens.size() > 0) \
            boost::range::copy(*tokens.rbegin(), std::back_inserter(what)); \
        what += "] "; \
        what += pwiz::util::ToStdString(e->Message); \
        throw std::runtime_error(what); \
    }


#endif // _CPP_CLI_UTILITIES_HPP_

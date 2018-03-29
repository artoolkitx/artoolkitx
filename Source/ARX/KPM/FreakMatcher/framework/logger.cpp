//
//  logger.cpp
//  artoolkitX
//
//  This file is part of artoolkitX.
//
//  artoolkitX is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  artoolkitX is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#include "logger.h"
//#include "filesystem_utils.h"

#include <iostream>
#include <sstream>
#include <stdarg.h>

using namespace vision;



BackendSinkFilter::BackendSinkFilter() {}

BackendSinkFilter::~BackendSinkFilter() {}

void BackendSinkFilter::write(const std::string& str) {
    std::cerr << str << std::endl;
}

FrontendSinkFilter::FrontendSinkFilter(BackendSinkFilterPtr& backendFilter)
: mPriorityMask(LOGGER_DISABLE)
, mBackendSinkFilter(backendFilter) {}

FrontendSinkFilter::~FrontendSinkFilter() {}

void FrontendSinkFilter::write(LoggerPriorityLevel level, const std::string& str) {
    if(allow(level)) {
        mBackendSinkFilter->write(str);
    }
}

Logger::Logger() {}

Logger::~Logger() {}

void Logger::write(LoggerPriorityLevel level, const std::string& str) {
    for(size_t i = 0; i < mFrontendSinkFilters.size(); i++) {
        mFrontendSinkFilters[i]->write(level, str);
    }
}

void Logger::write(LoggerPriorityLevel level, const char* fmt, ...) {
    va_list arg_list;
    va_start(arg_list, fmt);
    write(level, detail::create_formatted_string(std::string(fmt), arg_list));
    va_end(arg_list);
}

void Logger::addSinkFilter(FrontendSinkFilterPtr& f) {
    mFrontendSinkFilters.push_back(f);
}

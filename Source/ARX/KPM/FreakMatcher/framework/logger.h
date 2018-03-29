//
//  logger.h
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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "error.h"
#include "date_time.h"
#include <fstream>

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// DEFINES to enable/disable specific logger priorities
#define ENABLE_LOGGER
#define ENABLE_FATAL
#define ENABLE_ERROR
#define ENABLE_WARNING
#define ENABLE_INFO
#define ENABLE_DEBUG
#define ENABLE_TRACE

// STRING name of the logger priority
#define LOGGER_FATAL_MESSAGE    " FATAL "
#define LOGGER_ERROR_MESSAGE    " ERROR "
#define LOGGER_WARNING_MESSAGE  "WARNING"
#define LOGGER_INFO_MESSAGE     " INFO  "
#define LOGGER_DEBUG_MESSAGE    " DEBUG "
#define LOGGER_TRACE_MESSAGE    " TRACE "

// Format of the log message
#define LOGGER_FORMAT_SHORT(FMT, PRIORITY) "[%s] [%s] : " FMT, PRIORITY, vision::get_pretty_time().c_str()
#define LOGGER_FORMAT_LONG(FMT, PRIORITY) "[%s] [%s] [%s] : " FMT, PRIORITY, vision::get_pretty_time().c_str(), __PRETTY_FUNCTION__
#define LOGGER_FORMAT(FMT, PRIORITY) LOGGER_FORMAT_LONG(FMT, PRIORITY)

namespace vision {
    
    namespace detail {
        
        /**
         * Converts a C-style function formatted argument list into a C++ STRING.
         */
        inline std::string create_formatted_string(const std::string& fmt, va_list arg_list) {
            const size_t MAX_BUFFER_SIZE = 2048;
            char buffer[MAX_BUFFER_SIZE];
            vsnprintf(buffer, MAX_BUFFER_SIZE, fmt.c_str(), arg_list);
            return std::string(buffer);
        }
        
    } // detail
    
    enum LoggerPriorityLevel {
        LOGGER_DISABLE     = 0,
        LOGGER_FATAL       = 1,
        LOGGER_ERROR       = 2,
        LOGGER_WARNING     = 4,
        LOGGER_INFO        = 8,
        LOGGER_DEBUG       = 16,
        LOGGER_TRACE       = 32
    }; // Levels
    
    /**
     * Back-end filters are the filters that log messages to the console, file or network. The default
     * back-end filter logs to the standard error stream.
     */
    class BackendSinkFilter {
    public:
        
        BackendSinkFilter();
        virtual ~BackendSinkFilter();
    
        /**
         * Write a message to the sink filter.
         */
        virtual void write(const std::string& str);
    };
    
    typedef std::shared_ptr<BackendSinkFilter> BackendSinkFilterPtr;

    /**
     * The front-end filter "filters" messages based on the priority mask before forwarding to the 
     * corresponding back-end filter.
     */
    class FrontendSinkFilter {
    public:
        
        FrontendSinkFilter(BackendSinkFilterPtr& backendFilter);
        virtual ~FrontendSinkFilter();
        
        /**
         * Set the priority mask of the logger.
         */
        void setPriority(int mask) {
            mPriorityMask = mask;
        }
        
        /**
         * Get the priority mask of the logger.
         */
        int priority() const {
            return mPriorityMask;
        }
        
        /**
         * Write a message to the sink filter.
         */
        virtual void write(LoggerPriorityLevel level, const std::string& str);
        
        /**
         * Check if the message with LEVEL should be logged.
         */
        bool allow(LoggerPriorityLevel level) const {
            return (mPriorityMask & (int)level) != 0;
        }
        
    private:
        
        // Logger priority mask
        int mPriorityMask;
        
        // Back-end sink filter
        BackendSinkFilterPtr mBackendSinkFilter;
    };

    typedef std::shared_ptr<FrontendSinkFilter> FrontendSinkFilterPtr;
    
    class Logger {
    public:
    
        /**
         * Get an instance of the logger.
         */
        static Logger& getInstance() {
            static Logger logger;
            return logger;
        }
        
        /**
         * Write a message to the logger.
         */
        void write(LoggerPriorityLevel level, const std::string& str);
        void write(LoggerPriorityLevel level, const char* fmt, ...);
    
        /**
         * Add a front-end sink filter.
         */
        void addSinkFilter(FrontendSinkFilterPtr& f);
        
    private:
        
        // Vector of front-end sink filters
        std::vector<FrontendSinkFilterPtr> mFrontendSinkFilters;
        
        Logger();
        ~Logger();
        
    }; // Logger


} // vision

#if defined(ENABLE_LOGGER) && defined(ENABLE_FATAL)
#define LOG_FATAL(FMT, ...) \
vision::Logger::getInstance().write(vision::LOGGER_FATAL, LOGGER_FORMAT(FMT, LOGGER_FATAL_MESSAGE), ##__VA_ARGS__);
#else
#define LOG_FATAL(FMT, ...)
#endif

#if defined(ENABLE_LOGGER) && defined(ENABLE_ERROR)
#define LOG_ERROR(FMT, ...) \
vision::Logger::getInstance().write(vision::LOGGER_ERROR, LOGGER_FORMAT(FMT, LOGGER_ERROR_MESSAGE), ##__VA_ARGS__);
#else
#define LOG_ERROR(FMT, ...)
#endif

#if defined(ENABLE_LOGGER) && defined(ENABLE_WARNING)
#define LOG_WARNING(FMT, ...) \
vision::Logger::getInstance().write(vision::LOGGER_WARNING, LOGGER_FORMAT(FMT, LOGGER_WARNING_MESSAGE), ##__VA_ARGS__);
#else
#define LOG_WARNING(FMT, ...)
#endif

#if defined(ENABLE_LOGGER) && defined(ENABLE_INFO)
#define LOG_INFO(FMT, ...) \
vision::Logger::getInstance().write(vision::LOGGER_INFO, LOGGER_FORMAT(FMT, LOGGER_INFO_MESSAGE), ##__VA_ARGS__);
#else
#define LOG_INFO(FMT, ...)
#endif

#if defined(ENABLE_LOGGER) && defined(ENABLE_DEBUG)
#define LOG_DEBUG(FMT, ...) \
vision::Logger::getInstance().write(vision::LOGGER_DEBUG, LOGGER_FORMAT(FMT, LOGGER_DEBUG_MESSAGE), ##__VA_ARGS__);
#else
#define LOG_DEBUG(FMT, ...)
#endif

#if defined(ENABLE_LOGGER) && defined(ENABLE_TRACE)
#define LOG_TRACE(FMT, ...) \
vision::Logger::getInstance().write(vision::LOGGER_TRACE, LOGGER_FORMAT(FMT, LOGGER_TRACE_MESSAGE), ##__VA_ARGS__);
#else
#define LOG_TRACE(FMT, ...)
#endif
/*
 *  thread_sub_winrt.cpp
 *  artoolkitX
 *
 *  This file is part of artoolkitX.
 *
 *  artoolkitX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  artoolkitX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with artoolkitX.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2018 Realmax, Inc.
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2014-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

//
// File notes:
// Since we use WinRT types in this file, it must be compiled with /ZW compiler option (Visual
// Studio setting "Consume Windows Runtime extension:Yes").
// This will generate linker warning 4264, which can be safely ignored, as we are not exposing any WinRT API
// in the public interface. To do this, add /ignore:4264 to the linker "Additional options" setting.
// See http://msdn.microsoft.com/en-us/library/windows/apps/hh771041.aspx for more info.
//

#include "thread_sub_winrt.h"

#ifdef _WINRT

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;

int arCreateDetachedThreadWinRT(void *(*start_routine)(THREAD_HANDLE_T*), THREAD_HANDLE_T*flag)
{
        auto workItemHandler = ref new WorkItemHandler([=](IAsyncAction^)
        {
            // Run the user callback.
            try
            {
                start_routine(flag);
            }
            catch (...) { }
            
        }, CallbackContext::Any);

        ThreadPool::RunAsync(workItemHandler, WorkItemPriority::Normal, WorkItemOptions::TimeSliced);

		return 0;
}

#endif // !_WINRT

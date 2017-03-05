/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
*
*  Use of this source code is governed by a MIT-style license that can
*  be found in the License.html file in the root of the source tree.
*/

//---------------------------------------------------------------------------
// std includes
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <windows.h>
using namespace std;

//---------------------------------------------------------------------------
// MediaInfo includes
#include "MediaInfoDLL/MediaInfoDLL.h"
#include "MediaInfo/MediaInfo_Events.h"
using namespace MediaInfoDLL;

//---------------------------------------------------------------------------
// User handler
struct UserHandle_struct
{
    ostream &Out;

    UserHandle_struct(ostream &Out_)
        : Out(Out_)
    {}
};

//---------------------------------------------------------------------------
// Event - Global_AttachedFile
void Global_AttachedFile_0(struct MediaInfo_Event_Global_AttachedFile_0 *Event, struct UserHandle_struct* UserHandle)
{
    UserHandle->Out << "ID=" << Event->StreamIDs[0] << ", Size=" << Event->Content_Size;
    if (Event->Name && Event->Name[0])
        UserHandle->Out << ", Name=" << Event->Name;
    if (Event->MimeType && Event->MimeType[0])
        UserHandle->Out << ", MimeType=" << Event->MimeType;
    if (Event->Description && Event->Description[0])
        UserHandle->Out << ", Description=" << Event->Description;
    UserHandle->Out << endl;
}

//---------------------------------------------------------------------------
// Callback function
void __stdcall Event_CallBackFunction(unsigned char* Data_Content, size_t Data_Size, void* UserHandle_Void)
{
    struct UserHandle_struct*           UserHandle = (struct UserHandle_struct*)UserHandle_Void;
    struct MediaInfo_Event_Generic*     Event_Generic = (struct MediaInfo_Event_Generic*) Data_Content;
    unsigned char ParserID = (unsigned char)((Event_Generic->EventCode & 0xFF000000) >> 24);
    unsigned short EventID = (unsigned short)((Event_Generic->EventCode & 0x00FFFF00) >> 8);
    unsigned char EventVersion = (unsigned char)(Event_Generic->EventCode & 0x000000FF);

    switch (EventID)
    {
        case MediaInfo_Event_Global_AttachedFile:               if (EventVersion == 0) Global_AttachedFile_0((struct MediaInfo_Event_Global_AttachedFile_0*)Data_Content, UserHandle); break;
        default:;
    }
}

//---------------------------------------------------------------------------
// Main function
int main(int argc, char* argv[])
{
    // Defaults
    int InputFileName_pos = -1;
    char* LogFile = NULL;

    // Checking command line parameters
    for (int i = 1; i < argc; i++)
    {
             if (!strcmp(argv[i], "-h"))
        {
            InputFileName_pos = -1;
            break;
        }
        else if (!strcmp(argv[i], "-l") && i + 1 < argc)
        {
            i++;
            LogFile = argv[i];
        }
        else if (InputFileName_pos == -1)
            InputFileName_pos = i;
        else
        {
            cerr << "Invalid arguments" << endl;
            return 1;
        }
    }
    if (InputFileName_pos == -1)
    {
        cerr <<
            "Usage: " << argv[0] << " [-l LogFile] InputFileName\n"
            "-l log file instead of standard output" << endl;
        return 1;
    }

    // Finding the library
    if (MediaInfoDLL_Load() == (size_t)-1)
    {
        cerr << "MediaInfo library not found" << endl;
        return 1;
    }

    // Configuring log file
    ofstream LogFileStream;
    if (LogFile)
    {
        LogFileStream.open(LogFile, ios::ate);
        if (!LogFileStream.is_open())
        {
            cerr << LogFile << " can not be open" << endl;
            return 1;
        }
    }
    UserHandle_struct UserHandle(*(LogFile ? &LogFileStream : &cout));
    
    // MediaInfo instance
    MediaInfo MI;

    // MediaInfo config - we set the callback for MediaInfo events
    ostringstream Event_CallBackFunction_Text;
    Event_CallBackFunction_Text << "CallBack=memory://" << (MediaInfo_int64u)Event_CallBackFunction << ";UserHandler=memory://" << (MediaInfo_int64u)&UserHandle;
    MI.Option("File_Event_CallBackFunction", Event_CallBackFunction_Text.str());
    
    // Open
    MI.Open(argv[InputFileName_pos]);

    return 0;
}


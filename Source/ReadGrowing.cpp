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
// Generic output prefix
void OutputPrefix(ostream &Out)
{
    SYSTEMTIME Time;
    GetLocalTime(&Time);

    Out << dec << setw(2) << setfill('0') << Time.wHour << ':' << setw(2) << Time.wMinute << ':' << setw(2) << Time.wSecond << '.' << setfill('0') << setw(3) << Time.wMilliseconds << ", ";
}

//---------------------------------------------------------------------------
// Event - General_Start
void General_Start_0(struct MediaInfo_Event_General_Start_0* Event, struct UserHandle_struct* UserHandle)
{
    OutputPrefix(UserHandle->Out);
    UserHandle->Out << "(Re)starting parsing, ";
    if (Event->Stream_Size == (MediaInfo_int64u)-1)
        UserHandle->Out << "unknown size";
    else
        UserHandle->Out << "file size is " << dec << Event->Stream_Size << " (0x" << hex << Event->Stream_Size << ") bytes";
    UserHandle->Out << endl;
}

//---------------------------------------------------------------------------
// Event - General_End
void General_End_0(struct MediaInfo_Event_General_End_0* Event, struct UserHandle_struct* UserHandle)
{
    OutputPrefix(UserHandle->Out);
    UserHandle->Out << "Ending parsing, ";
    if (Event->Stream_Size == (MediaInfo_int64u)-1)
        UserHandle->Out << "unknown size";
    else
        UserHandle->Out << "file size is " << dec << Event->Stream_Size << " (0x" << hex << Event->Stream_Size << ") bytes, ";
    UserHandle->Out << dec << Event->Stream_Bytes_Analyzed << " (0x" << hex << Event->Stream_Bytes_Analyzed << ") bytes analyzed" << endl;
}

//---------------------------------------------------------------------------
// Event - General_WaitForMoreData_Start
void General_WaitForMoreData_Start_0(struct MediaInfo_Event_General_WaitForMoreData_Start_0* Event, struct UserHandle_struct* UserHandle)
{
    OutputPrefix(UserHandle->Out);
    UserHandle->Out << "Waiting for more data during " << dec << Event->Duration_Max << " seconds" << endl;
}

//---------------------------------------------------------------------------
// Event - General_WaitForMoreData_End
void General_WaitForMoreData_End_0(struct MediaInfo_Event_General_WaitForMoreData_End_0* Event, struct UserHandle_struct* UserHandle)
{
    OutputPrefix(UserHandle->Out);
    UserHandle->Out << "Do not wait anymore for more data, " << dec << Event->Duration_Actual << " seconds elapsed, ";
    if (Event->Flags & 1 << 0) //Flag at bit 0 says if giving up
        UserHandle->Out << "giving up";
    else
        UserHandle->Out << "continuing";
    UserHandle->Out << endl;
}

//---------------------------------------------------------------------------
// Event - Global_Demux
void Global_Demux_4(struct MediaInfo_Event_Global_Demux_4 *Event, struct UserHandle_struct* UserHandle)
{
    OutputPrefix(UserHandle->Out);
    UserHandle->Out << "ID=" << setfill('0') << setw(4) << Event->StreamIDs[0] << ", Offset=" << setfill('0') << setw(10) << hex << Event->StreamOffset << ", Frame#=" << setw(4) << dec << Event->FrameNumber << ", DTS=" << setw(10) << setprecision(3) << fixed << Event->DTS / 1000000.0 << " demuxed";
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
        case MediaInfo_Event_General_Start:                     if (EventVersion == 0) General_Start_0((struct MediaInfo_Event_General_Start_0*)Data_Content, UserHandle); break;
        case MediaInfo_Event_General_End:                       if (EventVersion == 0) General_End_0((struct MediaInfo_Event_General_End_0*)Data_Content, UserHandle); break;
        case MediaInfo_Event_General_WaitForMoreData_Start:     if (EventVersion == 0) General_WaitForMoreData_Start_0((struct MediaInfo_Event_General_WaitForMoreData_Start_0*)Data_Content, UserHandle); break;
        case MediaInfo_Event_General_WaitForMoreData_End:       if (EventVersion == 0) General_WaitForMoreData_End_0((struct MediaInfo_Event_General_WaitForMoreData_End_0*)Data_Content, UserHandle); break;
        case MediaInfo_Event_Global_Demux:                      if (EventVersion == 4) Global_Demux_4((struct MediaInfo_Event_Global_Demux_4*)Data_Content, UserHandle); break;
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
    
    // In case the file creation script is launched at the same time, we let it the time for creating the file
    Sleep(1000);

    // MediaInfo instance
    MediaInfo MI;

    // MediaInfo config - we want the whole stream
    MI.Option("ParseSpeed", "1.0");

    // MediaInfo config - demuxing 1 frame per demux packet, only raw streams (not intermediate layers)
    MI.Option("Demux", "container");
    MI.Option("File_Demux_Unpacketize", "1");

    // MediaInfo config - we want to do some stuff after each packet is demuxed
    MI.Option("File_NextPacket", "1");
    
    // MediaInfo config - we set the callback for MediaInfo events
    ostringstream Event_CallBackFunction_Text;
    Event_CallBackFunction_Text << "CallBack=memory://" << (MediaInfo_int64u)Event_CallBackFunction << ";UserHandler=memory://" << (MediaInfo_int64u)&UserHandle;
    MI.Option("File_Event_CallBackFunction", Event_CallBackFunction_Text.str());
    
    // MediaInfo config - we want to test growing files even if there is no indicator of growing in the file, during 10 seconds
    MI.Option("File_GrowingFile_Force", "1");
    MI.Option("File_GrowingFile_Delay", "10");

    // Open
    MI.Open(argv[InputFileName_pos]);

    // Loop
    while (MI.Open_NextPacket() & 0x100) // bit 8 means that a packet was found
    {
        SYSTEMTIME Time;
        GetLocalTime(&Time);
        UserHandle.Out << ", back to main loop at " << setw(2) << Time.wHour << ':' << setw(2) << Time.wMinute << ':' << setw(2) << Time.wSecond << '.' << setfill('0') << setw(3) << Time.wMilliseconds << endl;
    }

    // Ending
    OutputPrefix(UserHandle.Out);
    UserHandle.Out << "Finished, file size is " << MI.Get(Stream_General, 0, "FileSize") << ", file duration is " << MI.Get(Stream_General, 0, "Duration/String") << endl;
   
    return 0;
}


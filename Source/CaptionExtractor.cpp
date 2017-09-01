/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
*
*  Use of this source code is governed by a MIT-style license that can
*  be found in the License.html file in the root of the source tree.
*/

//---------------------------------------------------------------------------
// std includes
#include <iostream>
#include <iomanip>
#include <map>
#include <fstream>
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <locale>
#include <codecvt>
#include <windows.h>
using namespace std;

//---------------------------------------------------------------------------
// MediaInfo includes
#include "MediaInfoDLL/MediaInfoDLL.h"
#include "MediaInfo/MediaInfo_Events.h"
using namespace MediaInfoDLL;

//---------------------------------------------------------------------------
// User handler (common data between the calling function and the callback function)
struct UserHandle_struct
{
    ostream& Out;
    map<string, ofstream> Outs;
    const char* FileName;

    UserHandle_struct(ostream &Out_, const char* FileName_)
        : Out(Out_)
        , FileName(FileName_)
    {}
};

//---------------------------------------------------------------------------
// ID build
string ID_FromEvent(struct MediaInfo_Event_Generic *Event)
{
    //ID is the list of unique identifier per analysed layer in the file
    //Example: for 608 caption in CDP in Ancillary data in MXF file referenced by another MXF file, IDs are the following ones and with this order:
    // - Track ID from the master MXF
    // - Track ID from the reference MXF
    // - Ancillary data DID+SDID (here, the DID+SDID reserved for CDP transport in Ancillary data)
    // - CDP order (0 for 608 field 1, 1 for 608 field 2, 2 for 708)
    // - Service order (for 608 field 1: 0 for CC1, 1 for CC2, 2 for T1, 3 for T2)

    ostringstream t;
    for (size_t i = 0; i < Event->StreamIDs_Size; i++)
    {
        t << hex << setw(Event->StreamIDs_Width[i]) << setfill('0') << Event->StreamIDs[i];
        if (i + 1 < Event->StreamIDs_Size)
            t << "-";
    }

    return t.str();
};

//---------------------------------------------------------------------------
// Timestamp build
std::string MediaInfo_TimeStamp(MediaInfo_int64u Value)
{
    // TimeStamp is in nanosecond, we transform it to HH:MM:SS.mmm format
    // Empty if not available

    if (Value == (MediaInfo_int64u)-1)
        return string();

    long long Value2 = Value / 1000000; //Value is in nanoseconds
    MediaInfo_int64u HH = Value2 / 1000 / 60 / 60;
    MediaInfo_int64u MM = Value2 / 1000 / 60 - ((HH * 60));
    MediaInfo_int64u SS = Value2 / 1000 - ((HH * 60 + MM) * 60);
    MediaInfo_int64u MS = Value2 - ((HH * 60 + MM) * 60 + SS) * 1000;
    std::string Date;
    if (HH>999)
        Date.append("X");
    else if (HH>99)
        Date.append(1, (char)('0' + (HH / 100)));
    Date.append(1, (char)('0' + (HH / 10)));
    Date.append(1, '0' + (HH % 10));
    Date.append(1, ':');
    Date.append(1, (char)('0' + (MM / 10)));
    Date.append(1, (char)('0' + (MM % 10)));
    Date.append(1, ':');
    Date.append(1, (char)('0' + (SS / 10)));
    Date.append(1, (char)('0' + (SS % 10)));
    Date.append(1, '.');
    Date.append(1, (char)('0' + (MS / 100)));
    Date.append(1, (char)('0' + ((MS / 10) % 10)));
    Date.append(1, (char)('0' + (MS % 10)));
    return Date;
}

//---------------------------------------------------------------------------
// Select Output (dump to sidecar files or displayed to std::cout)
ostream& Out_Select(struct MediaInfo_Event_Generic *Event, struct UserHandle_struct* UserHandle, const char* Name)
{
    if (!UserHandle->FileName)
    {
        // Displayed to std::cout
        UserHandle->Out << ID_FromEvent(Event) << "\n";
        UserHandle->Out << Name << "\n";
        return UserHandle->Out;
    }

    //Build file name based on ID suite
    string ID=ID_FromEvent(Event);
    map<string, ofstream>::iterator Out = UserHandle->Outs.find(ID);
    if (Out != UserHandle->Outs.end())
        return Out->second; // File already exists, returning its handle

    UserHandle->Outs[ID].open(string(UserHandle->FileName) + "." + ID + "." + Name + ".txt");

    return UserHandle->Outs[ID];
};

//---------------------------------------------------------------------------
// Event - Eia608_CC_Content
void Eia608_CC_Content_0(struct MediaInfo_Event_Eia608_CC_Content_0 *Event, struct UserHandle_struct* UserHandle)
{
    ostream& Out = Out_Select((MediaInfo_Event_Generic*)Event, UserHandle, "Eia608_CC_Content");
    Out << "PTS=" << MediaInfo_TimeStamp(Event->PTS) << ", Field=" << (int)Event->Field << ", Service=" << (int)Event->Service << "\n";
    Out << "**********************************\n";
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    for (size_t i = 0; i<15; i++)
        Out << "*" << utf8_conv.to_bytes(Event->Row_Values[i]) << "*\n";
    Out << "**********************************\n";
    Out << endl;
}

//---------------------------------------------------------------------------
// Event - Eia708_CC_Content
void DtvccCaption_Content_Minimal_0(struct MediaInfo_Event_DtvccCaption_Content_Minimal_0* Event, struct UserHandle_struct* UserHandle)
{
    ostream& Out = Out_Select((MediaInfo_Event_Generic*)Event, UserHandle, "DtvccCaption_Content_Minimal");
    Out << "PTS=" << MediaInfo_TimeStamp(Event->PTS) << ", Service=" << (int)Event->Service << "\n";
    const wchar_t* t = wcschr(Event->Row_Values[0], '\0');
    if (!t)
        return;
    size_t Size=2+size_t(t-Event->Row_Values[0]);
    for (size_t i = 0; i<Size; i++)
        Out << '*';
    Out << '\n';
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    for (size_t i = 0; i<15; i++)
        Out << "*" << utf8_conv.to_bytes(Event->Row_Values[i]) << "*\n";
    for (size_t i = 0; i<Size; i++)
        Out << '*';
    Out << '\n';
    Out << endl;
}

//---------------------------------------------------------------------------
// Callback function - EIA-608 specific calls
void __stdcall Event_CallBackFunction_Eia608(struct MediaInfo_Event_Generic *Event_Generic, struct UserHandle_struct* UserHandle)
{
    unsigned short                      EventID         = (unsigned short)((Event_Generic->EventCode & 0x00FFFF00) >> 8);
    unsigned char                       EventVersion    = (unsigned char)  (Event_Generic->EventCode & 0x000000FF);

    // Send events to the corresponding event parser
    switch (EventID)
    {
        case MediaInfo_Event_Eia608_CC_Content:             if (EventVersion == 0) Eia608_CC_Content_0((struct MediaInfo_Event_Eia608_CC_Content_0*)Event_Generic, UserHandle); break;
        default:;
    }
}

//---------------------------------------------------------------------------
// Callback function - DTVCC Caption specific calls
void __stdcall Event_CallBackFunction_DtvccCaption(struct MediaInfo_Event_Generic *Event_Generic, struct UserHandle_struct* UserHandle)
{
    unsigned short                      EventID         = (unsigned short)((Event_Generic->EventCode & 0x00FFFF00) >> 8);
    unsigned char                       EventVersion    = (unsigned char)  (Event_Generic->EventCode & 0x000000FF);

    // Send events to the corresponding event parser
    switch (EventID)
    {
        case MediaInfo_Event_Eia608_CC_Content:             if (EventVersion == 0) DtvccCaption_Content_Minimal_0((struct MediaInfo_Event_DtvccCaption_Content_Minimal_0*)Event_Generic, UserHandle); break;
        default:;
    }
}

//---------------------------------------------------------------------------
// Callback function
void __stdcall Event_CallBackFunction(unsigned char* Data_Content, size_t Data_Size, void* UserHandle_Void)
{
    struct UserHandle_struct           *UserHandle      = (struct UserHandle_struct*)UserHandle_Void;
    struct MediaInfo_Event_Generic     *Event_Generic   = (struct MediaInfo_Event_Generic*)Data_Content;
    unsigned char                       ParserID        = (unsigned char) ((Event_Generic->EventCode & 0xFF000000) >> 24);

    // Send specific events to the corresponding event parser
    switch (ParserID)
    {
        case MediaInfo_Parser_Eia608 :                      Event_CallBackFunction_Eia608(Event_Generic, UserHandle); break;
        case MediaInfo_Parser_DtvccCaption :                Event_CallBackFunction_DtvccCaption(Event_Generic, UserHandle); break;
        default:;
    }
}

//---------------------------------------------------------------------------
// Main function
int main(int argc, char* argv[])
{
    // Defaults
    int InputFileName_pos = -1;
    bool Dump = false;

    // Checking command line parameters
    for (int i = 1; i < argc; i++)
    {
             if (!strcmp(argv[i], "-h"))
        {
            InputFileName_pos = -1;
            break;
        }
        else if (!strcmp(argv[i], "-d"))
            Dump = true;
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
            "Usage: " << argv[0] << " [-d] InputFileName\n"
            "-d dump to files (in current dir) instead of standard output" << endl;
        return 1;
    }

    // Finding the library
    if (MediaInfoDLL_Load() == (size_t)-1)
    {
        cerr << "MediaInfo library not found" << endl;
        return 1;
    }

    UserHandle_struct UserHandle(cout, Dump?argv[InputFileName_pos]:NULL);
    
    // MediaInfo instance
    MediaInfo MI;

    // MediaInfo config - we set the callback for MediaInfo events
    ostringstream Event_CallBackFunction_Text;
    Event_CallBackFunction_Text << "CallBack=memory://" << (MediaInfo_int64u)Event_CallBackFunction << ";UserHandler=memory://" << (MediaInfo_int64u)&UserHandle;
    MI.Option("File_Event_CallBackFunction", Event_CallBackFunction_Text.str());

    // MediaInfo config - we want the whole stream
    MI.Option("ParseSpeed", "1.0");

    // Open
    MI.Open(argv[InputFileName_pos]);

    return 0;
}


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
// Generic output prefix
void OutputPrefix(ostream &Out)
{
    SYSTEMTIME Time;
    GetLocalTime(&Time);

    Out << dec << setw(2) << setfill('0') << Time.wHour << ':' << setw(2) << Time.wMinute << ':' << setw(2) << Time.wSecond << '.' << setfill('0') << setw(3) << Time.wMilliseconds << ", ";
}

//---------------------------------------------------------------------------
// Long output
void LongOutput(ostream &Out, LARGE_INTEGER &Offset)
{
    SYSTEMTIME Time;
    GetLocalTime(&Time);

    OutputPrefix(Out);
    Out << "Offset=" << setfill('0') << setw(10) << hex << Offset.QuadPart << endl;
}

//---------------------------------------------------------------------------
// Main function
int main(int argc, char* argv[])
{
    // Defaults
    int InputFileName_pos = -1;
    int OutputFileName_pos = -1;
    DWORD Buffer_Size_Max = -1; // Based on file size
    bool Short = false;
    DWORD Delay_Value = 1000; // 1000 ms
    char* LogFile = NULL;
    LARGE_INTEGER Offset_IgnoreSleep = { 0 };

    // Checking command line parameters
    for (int i = 1; i < argc; i++)
    {
             if (!strcmp(argv[i], "-b") && i + 1 < argc)
        {
            i++;
            Buffer_Size_Max = atoi(argv[i]);
        }
        else if (!strcmp(argv[i], "-i") && i + 1 < argc)
        {
            i++;
            Offset_IgnoreSleep.LowPart = atoi(argv[i]);
        }
        else if (!strcmp(argv[i], "-h"))
        {
            OutputFileName_pos = -1;
            break;
        }
        else if (!strcmp(argv[i], "-l") && i + 1 < argc)
        {
            i++;
            LogFile = argv[i];
        }
        else if (!strcmp(argv[i], "-s"))
        {
            Short = true;
        }
        else if (!strcmp(argv[i], "-t") && i + 1 < argc)
        {
            i++;
            Delay_Value = (DWORD)(atof(argv[i]) * 1000);
        }
        else if (InputFileName_pos == -1)
            InputFileName_pos = i;
        else if (OutputFileName_pos == -1)
            OutputFileName_pos = i;
        else
        {
            cout << "Invalid arguments" << endl;
            return 1;
        }
    }
    if (OutputFileName_pos == -1 || !Buffer_Size_Max || !Delay_Value)
    {
        cout <<
            "Usage: " << argv[0] << " [-b Bytes] [-t Seconds] [-s] [-l LogFile] InputFileName OutputFileName\n"
            "-b count of bytes to read at each iteration\n"
            "-t count of seconds to wait between each iteration\n"
            "-i count of bytes ignoring -t value (\"burst\")\n"
            "-s short form of log (only a period)"
            "-l log file instead of standard output" << endl;
        return 1;
    }

    // Input file
    HANDLE Input = CreateFile(argv[InputFileName_pos], FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (Input == INVALID_HANDLE_VALUE)
    {
        cout << "InputFileName " << argv[InputFileName_pos] << " can not be open for reading" << endl;
        return 1;
    }

    // Output file
    DeleteFile(argv[OutputFileName_pos]);
    HANDLE Output = CreateFile(argv[OutputFileName_pos], FILE_WRITE_DATA, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL);
    if (Output == INVALID_HANDLE_VALUE)
    {
        cout << "OutputFileName " << argv[OutputFileName_pos] << " can not be open for writing (File exists?)" << endl;
        return 1;
    }

    // Configuring log file
    ofstream LogFileStream;
    if (LogFile)
    {
        LogFileStream.open(LogFile, ios::ate);
        if (!LogFileStream.is_open())
        {
            cout << LogFile << " can not be open" << endl;
            return 1;
        }
    }
    ostream &Out = *(LogFile ? &LogFileStream : &cout);

    // Getting input size
    LARGE_INTEGER Size = { 0 };
    LARGE_INTEGER Offset = { 0 };
    if (!GetFileSizeEx(Input, &Size))
        return 1;
    if (Buffer_Size_Max == (DWORD)-1)
    {
        if (Size.HighPart) //More than 4 GiB
            Buffer_Size_Max = 0x1000000; // 16 MiB
        else if (Size.LowPart >= 0x10000000) // More than 256 MiB
            Buffer_Size_Max = 0x100000; // 1 MiB
        else
            Buffer_Size_Max = 0x10000; // 64 KiB
    }

    // Init
    DWORD Buffer_Size;
    LPVOID Buffer = new unsigned char[Buffer_Size_Max];
    if (!Short)
        LongOutput(Out, Offset);

    // Reading then writing then small pause
    while (ReadFile(Input, Buffer, Buffer_Size_Max, &Buffer_Size, NULL) && Buffer_Size)
    {
        DWORD Buffer_Size_Written;
        if (!WriteFile(Output, Buffer, Buffer_Size, &Buffer_Size_Written, NULL))
            return 1;
        Offset.QuadPart += Buffer_Size_Written;

        if (Short)
            Out << '.';
        else
            LongOutput(Out, Offset);

        if (Offset.QuadPart>Offset_IgnoreSleep.QuadPart)
            Sleep(Delay_Value); //Wait
    }

    CloseHandle(Output);

    // Ending
    delete[] Buffer;

    return 0;
}


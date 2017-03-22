/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
*
*  Use of this source code is governed by a MIT-style license that can
*  be found in the License.html file in the root of the source tree.
*/

//---------------------------------------------------------------------------
// std includes
#include <iostream>
#include <sstream>
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
void LongOutput(ostream &Out, LARGE_INTEGER &Offset, int Pos, int Count)
{
    SYSTEMTIME Time;
    GetLocalTime(&Time);

    OutputPrefix(Out);
    Out << "Offset=" << setfill('0') << setw(10) << hex << Offset.QuadPart;
    if (Count>1 && Offset.QuadPart)
        Out << dec << " (File " << Pos + 1 << "/" << Count << ")";
    Out << endl;
}

//---------------------------------------------------------------------------
// Main function
int main(int argc, char* argv[])
{
    // Defaults
    int InputFileName_pos = -1;
    int OutputFileName_pos = -1;
    int ReplaceInputFileName_pos = -1;
    DWORD Buffer_Size_Max = -1; // Based on file size
    bool Short = false;
    DWORD Delay_Value = 1000; // 1000 ms
    char* LogFile = NULL;
    int Count = 1;
    bool SequenceOfFiles = false;
    bool OpenWaitClose = false;
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
        else if (!strcmp(argv[i], "-c") && i + 1 < argc)
        {
            i++;
            Count = atoi(argv[i]);
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
        else if (!strcmp(argv[i], "-o"))
        {
            OpenWaitClose = true;
        }
        else if (!strcmp(argv[i], "-q"))
        {
            SequenceOfFiles = true;
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
        else if (!strcmp(argv[i], "-w") && i + 1 < argc)
        {
            i++;
            ReplaceInputFileName_pos = i;
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
            "Usage: " << argv[0] << " [-b Bytes] [-t Seconds] [-i Bytes] [-c Count] [-q] [-w ReplaceInputFileName] [-s] [-l LogFile] InputFileName OutputFileName\n"
            "-b count of bytes to read at each iteration\n"
            "-t count of seconds to wait between each iteration\n"
            "-i count of bytes ignoring -t value (\"burst\")\n"
            "-c count of repetitions of InputFileName\n"
            "-q create a sequence of files instead of an unique file\n"
            "-w write begin of the output file with new content\n"
            "-o close the file before waiting then open it again, in append mode\n"
            "-s short form of log (only a period)"
            "-l log file instead of standard output" << endl;
        return 1;
    }
    if (SequenceOfFiles && Count <= 1)
    {
        cout <<
            "-c value must be >1 when -q is used" << endl;
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
    HANDLE Output = INVALID_HANDLE_VALUE;
    string OutputFileName(argv[OutputFileName_pos]);
    size_t OutputFileName_DotPos = OutputFileName.rfind('.');
    if (OutputFileName_DotPos == string::npos)
        OutputFileName_DotPos = OutputFileName.size();
    stringstream SequenceOfFiles_NumberWidth_Temp;
    SequenceOfFiles_NumberWidth_Temp << Count - 1;
    size_t SequenceOfFiles_NumberWidth = SequenceOfFiles_NumberWidth_Temp.str().size();

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
        LongOutput(Out, Offset, 0, Count);

    // Deleting previous files
    for (int Pos=0; Pos<Count; Pos++)
    {
        if (SequenceOfFiles)
        {
            stringstream Count_Stream;
            Count_Stream << setfill('0') << setw(SequenceOfFiles_NumberWidth) << Pos;
            OutputFileName.insert(OutputFileName_DotPos, Count_Stream.str());
        }

        DeleteFile(OutputFileName.c_str());

        if (SequenceOfFiles)
        {
            OutputFileName.erase(OutputFileName_DotPos, SequenceOfFiles_NumberWidth);
        }
    }

    // Reading then writing then small pause
    for (int Pos = 0;;)
    {
        // Output file
        if (Output == INVALID_HANDLE_VALUE)
        {
            if (SequenceOfFiles)
            {
                stringstream Count_Stream;
                Count_Stream << setfill('0') << setw(SequenceOfFiles_NumberWidth) << Pos;
                OutputFileName.insert(OutputFileName_DotPos, Count_Stream.str());
            }

            DeleteFile(OutputFileName.c_str());
            Output = CreateFile(OutputFileName.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL);
            if (Output == INVALID_HANDLE_VALUE)
            {
                cout << "OutputFileName " << OutputFileName << " can not be open for writing (File exists?)" << endl;
                return 1;
            }

            if (SequenceOfFiles)
            {
                OutputFileName.erase(OutputFileName_DotPos, SequenceOfFiles_NumberWidth);
            }
        }

        while (ReadFile(Input, Buffer, Buffer_Size_Max, &Buffer_Size, NULL) && Buffer_Size)
        {
            DWORD Buffer_Size_Written;
            if (!WriteFile(Output, Buffer, Buffer_Size, &Buffer_Size_Written, NULL))
                return 1;
            Offset.QuadPart += Buffer_Size_Written;

            if (Short)
                Out << '.';
            else
                LongOutput(Out, Offset, Pos, Count);

            if (Offset.QuadPart>Offset_IgnoreSleep.QuadPart)
            {
                if (OpenWaitClose)
                    CloseHandle(Output);

                Sleep(Delay_Value); //Wait

                if (OpenWaitClose)
                {
                    Output = CreateFile(OutputFileName.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                    if (Output == INVALID_HANDLE_VALUE)
                    {
                        cout << "OutputFileName " << OutputFileName << " can not be open for appending (File exists?)" << endl;
                        return 1;
                    }
                }
            }
        }

        // Next file
        Pos++;

        // Output file
        if ((SequenceOfFiles || Pos >= Count) && ReplaceInputFileName_pos == -1)
        {
            CloseHandle(Output);
            Output = INVALID_HANDLE_VALUE;
        }

        // Check the count of loops
        if (Pos>=Count)
            break;

        // Seek to 0
        LARGE_INTEGER GoTo;
        GoTo.QuadPart=0;
        BOOL i = SetFilePointerEx(Input, GoTo, NULL, SEEK_SET);
    }

    // Replace begin of a file
    if (ReplaceInputFileName_pos != -1)
    {
        LARGE_INTEGER GoTo;
        GoTo.QuadPart = 0;
        if (!SetFilePointerEx(Output, GoTo, NULL, SEEK_SET))
        {
            cout << "IntpuFileName " << OutputFileName << " can not be restarted" << endl;
            return 1;
        }

        HANDLE Input2 = CreateFile(argv[ReplaceInputFileName_pos], FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (Input2 == INVALID_HANDLE_VALUE)
        {
            cout << "InputFileName " << argv[ReplaceInputFileName_pos] << " can not be open for reading" << endl;
            return 1;
        }

        while (ReadFile(Input2, Buffer, Buffer_Size_Max, &Buffer_Size, NULL) && Buffer_Size)
        {
            DWORD Buffer_Size_Written;
            if (!WriteFile(Output, Buffer, Buffer_Size, &Buffer_Size_Written, NULL))
                return 1;
        }

        CloseHandle(Input2);
        CloseHandle(Output);
    }

    // Ending
    CloseHandle(Input);
    delete[] Buffer;

    return 0;
}


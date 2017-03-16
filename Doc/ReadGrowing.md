# ReadGrowing

An example for analyzing files during their creation (the input file is growing), in order to finish the full analysis few seconds after the end of the file creation.  

Notes: 

- this is useful mainly when `--ParseSpeed=1` is used (e.g. for full demux or conformance testing) as file analysis may be long.
- this is usable only with files which can be analyzed in sequential order (e.g. TS files, but no MOV file with header at the end of the file).
- growing file test is disabled in case another method is usable for knowing if a file is growing (e.g. MXF has information about muxing not finished so MediaInfo prefers to rely on the MXF partition status change in order to avoid a delay at the end of the parsing)
- only for files accessible by file API (no HTTP, HTTPS, FTP...)

(Visual C++ only) In case you don't have any other method for creating a growing file, `MediaInfoLib_Demos_ReadGrowing` Visual C++ solution will execute both the `ReadGrowing_CreateGrowing` and `ReadGrowing` projects and simulate a growing file while analyzing it.  
You need to add the input file name and the output file name for the command argument of `ReadGrowing_CreateGrowing` project and the input file name for the command argument of `ReadGrowing` project (same as the previously provided output file name).  

MediaInfo features used:

- Use of some logging events
- Full scan of the file
- Demux of coded frames
- Returns after the demux of each packet
- Callback function setup
- Growing file test activation

Options:

- `-l`: set a log file instead of standard output 

Example of result:
```
20:06:43.513, (Re)starting parsing, file size is 131072 (0x20000) bytes
20:06:43.959, ID=0481, Offset=0000000372, Frame#=0000, DTS=000999.989 demuxed, back to main loop at 20:06:43.961
20:06:43.961, ID=0481, Offset=0000006213, Frame#=0001, DTS=001039.989 demuxed, back to main loop at 20:06:43.963
20:06:43.964, ID=0481, Offset=000000c3cc, Frame#=0002, DTS=001079.989 demuxed, back to main loop at 20:06:43.966
20:06:43.967, Waiting for more data during 10.000 seconds
20:06:44.968, Do not wait anymore for more data, 1.000 seconds elapsed, continuing
20:06:44.970, (Re)starting parsing, file size is 196608 (0x30000) bytes
20:06:44.972, ID=0481, Offset=0000012443, Frame#=0003, DTS=001119.989 demuxed, back to main loop at 20:06:44.974
20:06:44.975, ID=0481, Offset=000001844d, Frame#=0004, DTS=001159.989 demuxed, back to main loop at 20:06:44.976
20:06:44.977, ID=0481, Offset=000001e072, Frame#=0005, DTS=001199.989 demuxed, back to main loop at 20:06:44.979
20:06:44.979, Waiting for more data during 10.000 seconds
20:06:54.985, Do not wait anymore for more data, 10.000 seconds elapsed, giving up
20:06:54.987, ID=0481, Offset=0000024621, Frame#=0006, DTS=001239.989 demuxed, back to main loop at 20:06:54.989
20:06:54.989, ID=0481, Offset=000002a85f, Frame#=0007, DTS=001279.989 demuxed, back to main loop at 20:06:54.991
20:06:55.162, ID=0481, Offset=000002fd56, Frame#=0008, DTS=001319.989 demuxed, back to main loop at 20:06:55.164
20:06:55.184, Ending parsing, file size is 196608 (0x30000) bytes, 218268 (0x3549c) bytes analyzed
20:06:55.967, Finished, file size is 196608, file duration is 305ms

```

Meaning that the file was 131072 byte long when MediaInfo parsing started, 3 frames are demuxed, then MediaInfo waits; 1 second later MediaInfo detects that the file size changed, reads again the file size and now it is 196608 byte long, MediaInfo continues to demux; after having hit the end of the file, MediaInfo waits again; after 10 seconds, MediaInfo gives up and ends the parsing, parsing the last (potentially) incomplete frames and showing the duration of the final file.

# ReadGrowing_CreateGrowing

Companion for `ReadGrowing` example in case you don't have any other method for creating a growing file, taking a source file as input and outputs a growing file, with some bytes written every second.

Use it through ReadGrowing project.

Options:

- `-b`: count of bytes to read at each iteration 
- `-t`: count of seconds to wait between each iteration 
- `-i`: count of bytes ignoring -t value ("burst") 
- `-c`: count of repetitions of InputFileName 
- `-q`: create a sequence of files instead of an unique file
- `-w`: write begin of the output file with new content
- `-s`: short form of log (only a period) 
- `-l`: file instead of standard output 

# Use cases

The following cases have been tested with the `ReadGrowing_CreateGrowing` program and MediaInfo library, below is information about if MediaInfo waits depending if it is without or with `--File_GrowingFile_Force=1` option:

|kind of file(s)                                                | without       | with         | Remarks      
|:--------------------------------------------------------------|---------------|--------------|--------------
| MPEG-TS/MPEG-PS file                                          | Maybe         | Yes          |
| sequence of MPEG-TS files (e.g. from HLS)                     | No [FIXME]    | No [FIXME]   |
| sequence of image (e.g. JPEG 2000) files                      | No [FIXME]    | No [FIXME]   |
| closed/complete MXF file                                      | Maybe         | Yes          |
| open/incomplete MXF file changed to closed/complete           | Maybe         | Maybe        |
| open/incomplete MXF file                                      | Yes           | Yes          |

`Maybe` means that if `--ParseSpeed=1` (full parsing) is used, there is one unique (no delay wiaitng for something) test after the end of the parsing, and the file is considered as growing if the file size (for 1 file) or file count (for sequences) changed between the begining and the end of the parsing.

- MPEG-TS file:
in the case of `--File_GrowingFile_Force=1` option, MediaInfo gives up after a time out waiting for file size change of the file,  
else MediaInfo immediately stops when the end of file is reached.  
`ReadGrowing_CreateGrowing ..\..\..\Sample\2s.ts ..\..\..\Result\2s.ts`  
`ReadGrowing ..\..\..\Result\2s.ts`  

- sequence of MPEG-TS files (e.g. from HLS):  
in the case of `--File_GrowingFile_Force=1` option, MediaInfo gives up after a time out waiting for new file presence or file size change of the last file,  
else MediaInfo immediately stops when the end of the last detected file is reached
`ReadGrowing_CreateGrowing ..\..\..\Sample\2s.ts ..\..\..\Result\Sequence.ts -c 100 -q -i 10000000`  
`ReadGrowing ..\..\..\Result\Sequence00.ts`  
[FIXME] Currently deactivated in MediaInfo due to unexpeted behavior, expected to be `Maybe` and `Yes`
Note: `-c 100 -q` is used in order to simulate an HLS muxer creating file one per one.  
Note: `-i 10000000` is used in order to create quickly the 30 first MPEG-TS files (300 KB each) because MediaInfo detects the sequence only if there is more than 24 files at startup.  

- sequence of image (e.g. JPEG 2000) files:  
in the case of `--File_GrowingFile_Force=1` option, MediaInfo gives up after a time out waiting for new file presence or file size change of the last file,  
else MediaInfo immediately stops when the end of the last detected file is reached
`ReadGrowing_CreateGrowing ..\..\..\Sample\PAL.jpg ..\..\..\Result\Sequence.jpg -t 0.040 -c 100 -q -i 1000000`  
`ReadGrowing ..\..\..\Result\Sequence00.jpg`  
[FIXME] Currently deactivated in MediaInfo due to unexpeted behavior, expected to be `Maybe` and `Yes`
Note: `-c 100 -q` is used in order to simulate an sequence of images being written on disc (either file copy or file creation) one per one.  
Note: `-i 1000000` is used in order to create quickly the 30 first JPEG files (30 KB each) because MediaInfo detects the sequence only if there is more than 24 files at startup.  

- closed/complete MXF file:
in the case of `--File_GrowingFile_Force=1` option, MediaInfo gives up after a time out waiting for file size change of the last file,  
else MediaInfo immediately stops when the end of file is reached.  
`ReadGrowing_CreateGrowing ..\..\..\Sample\2s.mxf ..\..\..\Result\2s.mxf`  
`ReadGrowing ..\..\..\Result\2s.mxf`  

- open/incomplete MXF file changed to closed/complete at the end of the muxing:  
MediaInfo considers it as a file being muxed and immediately stops when the end of file is reached, when the file changes to closed/complete.  
`--File_GrowingFile_Force=1` option is ignored as the file header changed (so demonstrating that this is a muxing and not a file copy) so is not useful.  
`ReadGrowing_CreateGrowing ..\..\..\Sample\2s_Open.mxf ..\..\..\Result\2s_OpenThenClosed.mxf -w ..\..\..\Sample\2s_Open_ClosedHeader.mxf`  
`ReadGrowing ..\..\..\Result\2s_OpenThenClosed.mxf`  
Note: `-w` is used in order to simulate an MXF muxer (open/incomplete during muxing, changed to closed/complete at the end of muxing).  

- open/incomplete MXF file growing then stops without change to closed/complete (muxer crash or file copy):  
MediaInfo considers it as a file being muxed and waits automaticaly for new content, MediaInfo gives up after a time out waiting for file size change of the file.  
Behavior is like if `--File_GrowingFile_Force=1` option is set, even if it is not set.  
`ReadGrowing_CreateGrowing ..\..\..\Sample\2s_Open.mxf ..\..\..\Result\2s_Open.mxf`  
`ReadGrowing ..\..\..\Result\2s_Open.mxf`  
Note: General duration is empty because only focused on the MXF header partition. [FIXME] Peek duration form MXF footer and/or count of frames.  



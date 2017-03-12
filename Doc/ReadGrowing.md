# ReadGrowing

An example for analyzing files during their creation (the input file is growing), in order to finish the full analysis few seconds after the end of the file creation.

Note: this is usable only with files which can be analyzed in sequential order (e.g. TS files, but no MOV file with header at the end of the file).

Note: growing file test is disabled in case another method is usable for knowing if a file is growing (e.g. MXF has information about muxing not finished so MediAInfo prefers to rely on the MXf partition status in order to avoid a delay at the end of the parsing)

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
- `-s`: short form of log (only a period) 
- `-l`: file instead of standard output 
